#include <ykernel.h>

#include "load_program.h"
#include "pcb.h"
#include "frame.h"
#include "kernel.h"
#include "scheduler.h"

static void add_child(pcb_t *parent, pcb_t *child) {
  /**
   * Add child to parent's children list
   */

  child->parent = parent; 
  child->next_sibling = parent->children;
  parent->children = child; 
}

static void remove_child(pcb_t *parent, pcb_t *child) {
  /**
   * Remove child from parent's children list
   */

  if (child == parent->children) {
    parent->children = child->next_sibling;
    child->next_sibling = NULL;
    return; 
  }

  pcb_t *prev = parent->children; 
  while (prev != NULL && prev->next_sibling != child) {
    prev = prev->next_sibling; 
  }

  if (prev != NULL) {
    prev->next_sibling = child->next_sibling;
  } else {
    TracePrintf(0, "remove_child: failed to find child (pid=%d) for parent (pid=%d)\n", child->pid, parent->pid); 
  }

  child->next_sibling = NULL; 
}

static void orphan_children(pcb_t *parent) {
  /**
   * When a process exits, its children become orphans
   * Orphan's continue normally, but we dont keep their PID when they die
   */

  pcb_t *child = parent->children; 
  while (child != NULL) {
    child->parent = NULL;

    child = child->next_sibling;
  }

  parent->children = NULL;
}

static void attach_zombie(pcb_t *parent, pcb_t *child) {
  /**
   * Move a dead child to its parent's zombie list. 
   */

  remove_child(parent, child); 
  child->next_zombie = parent->zombies;
  parent->zombies = child;
  child->state = ZOMBIE; 
}

pcb_t *pcb_create(void) {
  /**
   * Allocate a blank PCB with resources
   * 
   * Allocates: PCB struct, Region 1 page table, kernel stack frames
   * Registers pid with helper_new_pid
   * Does not load a program
   * 
   * New PCB on success, NULL otherwise
   */
  pcb_t *p = malloc(sizeof(pcb_t)); 
  if (p == NULL) {
    return NULL; 
  }

  p->region_1 = malloc(MAX_PT_LEN * sizeof(pte_t)); 
  if (p->region_1 == NULL) {
    free(p); 
    return NULL;
  }

  for (int i = 0; i < MAX_PT_LEN; i++) {
    p->region_1[i].valid = 0;
    p->region_1[i].prot = 0;
    p->region_1[i].pfn = 0; 
  }

  for (int i = 0; i < KSTACK_NPG; i++) {
    int frame = frame_alloc(); 
    if (frame == -1) {
      for (int j = 0; j < i; j++) {
        frame_free(p->kstack[j].pfn);
      }

      free(p->region_1);
      free(p);

      return NULL; 
    }

    p->kstack[i].pfn = frame;
    p->kstack[i].valid = 1;
  };

  p->pid = helper_new_pid(p->region_1); 

  p->ubrk = 0;
  p->ustack_low = 0;
  p->udata_end = 0; 

  p->state = RUNNABLE;
  p->waiting_on = WAIT_NONE; 
  p->wait_arg = 0; 

  p->parent = NULL;
  p->children = NULL;
  p->next_sibling = NULL;
  p->zombies = NULL;
  p->next_zombie = NULL;

  p->exit_status = 0;

  p->next = NULL;

  return p; 
}

void pcb_terminate(pcb_t *p, int status) {
  /**
   * End of life of the process. 
   * Frees Region 1 resources, handles the children and sibling state and schedule other process. 
   * Does not return to the caller since other process is scheduled. 
   * If p is the init process, halt.
   */

  p->exit_status = status; 
  //TODO: if p is init process, Halt

  // Free Region 1 frames
  for (int vpn = 0; vpn < MAX_PT_LEN; vpn++) {
    if (p->region_1[vpn].valid == 1) {
      frame_free(p->region_1[vpn].pfn);
      p->region_1[vpn].valid = 0;
    } 
  }

  free(p->region_1); 
  p->region_1 = NULL;

  // Cut ties with children
  orphan_children(p);

  // Free any zombies
  pcb_t *zombie;
  while (p->zombies != NULL) {
    zombie = p->zombies;
    p->zombies = p->zombies->next_zombie;
    pcb_destroy(zombie);
  }

  // Become parent's zombie if exists
  if (p->parent != NULL && p->parent->state != ZOMBIE) {
    attach_zombie(p->parent, p); 

    if (p->parent->state == BLOCKED && p->parent->waiting_on == WAIT_CHILD) {
      p->parent->state = RUNNABLE;
      p->parent->waiting_on = WAIT_NONE;
      ready_enqueue(p->parent); 
    }
  } else {
    pcb_destroy(p); 
  }

  // Current process is either zombie or freed
  // Schedule some other process
  g_current_process = NULL; 
  schedule(); 

  TracePrintf(0, "pcb_terminate: unexpected return from schedule\n"); 
  Halt(); 
}

void pcb_destroy(pcb_t *p) {
  /**
   * Fully release a PCB's resources
   * Assumes Region 1 frames and page table have already been freed.
   * Frees kernel stack frames, retires the PID, and frees the PCB struct
   */

  if (p == NULL) return; 

  for (int i = 0; i < KSTACK_NPG; i++) {
    if (p->kstack[i].valid == 1) {
      frame_free(p->kstack[i].pfn);
      p->kstack[i].valid = 0; 
    }
  }

  helper_retire_pid(p->pid);
  free(p); 
}

int pcb_clone(pcb_t *parent, pcb_t **child_out) {
  /**
   * Duplicate the current process's state into a new PCB
   * Allocates new PCB, copies Region 1 page by page
   * copies the user context, sets up parent-child pointers, and uses KCCopy to clone the
   * kernel context and stack
   *
   * On return, *child_out is set to a new PCB
   */

  pcb_t *child = pcb_create(); 
  if (child == NULL) return ERROR;

  child->uctx = parent->uctx;
  child->ubrk = parent->ubrk;
  child->ustack_low = parent->ustack_low;
  child->udata_end = parent->udata_end;

  add_child(parent, child); 

  // Copy Region 1 page by page
  int scratch_vpn = (KERNEL_STACK_BASE >> PAGESHIFT) - 1;
  for (int vpn = 0; vpn < MAX_PT_LEN; vpn++) {
    if (parent->region_1[vpn].valid == 0) {
      child->region_1[vpn].valid = 0;
      continue; 
    }

    int new_frame = frame_alloc(); 
    if (new_frame == -1) {
      // unwind
      for (int v = 0; v < vpn; v++) {
        if (child->region_1[v].valid == 1) {
          frame_free(child->region_1[v].pfn); 
        }
      }

      remove_child(parent, child); 
      pcb_destroy(child);
      return ERROR; 
    }

    g_region_0_pt[scratch_vpn].valid = 1;
    g_region_0_pt[scratch_vpn].prot = PROT_READ | PROT_WRITE;
    g_region_0_pt[scratch_vpn].pfn = new_frame;
    WriteRegister(REG_TLB_FLUSH, scratch_vpn << PAGESHIFT);
    
    void *src = VMEM_1_BASE + (vpn << PAGESIZE);
    void *dst = scratch_vpn << PAGESHIFT;
    memcpy(dst, src, PAGESIZE);

    g_region_0_pt[scratch_vpn].valid = 0; 
    WriteRegister(REG_TLB_FLUSH, scratch_vpn << PAGESHIFT); 

    // Install in child's page table.
    child->region_1[vpn].valid = 1; 
    child->region_1[vpn].prot = parent->region_1[vpn].prot;
    child->region_1[vpn].pfn = new_frame; 
  }

  // Now copy kernel context and stack
  if (KernelContextSwitch(KCCopy, (void *)child, NULL) != 0) {
    for (int vpn = 0; vpn < MAX_PT_LEN; vpn++) {
      if (child->region_1[vpn].valid == 1) {
        frame_free(child->region_1[vpn].pfn); 
      }
    }

    remove_child(parent, child); 
    pcb_destroy(child); 
    return ERROR; 
  }

  // Both parent and chidld execute past KernelContextSwitch
  // The child sits in the ready queue
  if (g_current_process == parent) {
    // We are the parent
    child->state = RUNNABLE; 
    ready_enqueue(child);
    *child_out = child;
    return SUCCESS;
  } else {
    // We are the child
    *child_out = child;
    return SUCCESS; 
  }
}

int pcb_load_program(pcb_t *p, char *filename, char **argv) {
  /**
   * Replace current process's program
   * Frees the current Region 1 frames, then calls LoadProgram to setup the new executable's
   * text, data, stack and initial UserContext
   * If LoadProgram fails, terminate the process
   * Returns 0 on success.
   */

  // Free all of Region 1
  for (int vpn = 0; vpn < MAX_PT_LEN; vpn++) {
    if (p->region_1[vpn].valid == 1) {
      frame_free(p->region_1[vpn].pfn);
      p->region_1[vpn].valid = 0; 
    }
  }

  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1); 

  if (LoadProgram(filename, argv, p) != 0) {
    pcb_terminate(p, ERROR); 
  }

  return SUCCESS;
}