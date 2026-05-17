#include <ykernel.h>

#include "pcb.h"
#include "frame.h"
#include "kernel.h"

static void add_child(pcb_t *parent, pcb_t *child) {
  /**
   * Add child to parent's children list
   * Pseudocode: 
   * child->parent = parent
   * child->next_sibling = parent->children
   * parent_childrent = child
   */
}

static void remove_child(pcb_t *parent, pcb_t *child) {
  /**
   * Remove child from parent's children list
   * Pseudocode: 
   *    walk parent->children list, find previous node whose next_sibling == child
   *    if child is head: parent->children = child->next_sibling
   *    else: prev->next_sibling = child->next_sibling
   *    child->next_sibling = NULL
   */
}

static void orphan_children(pcb_t *parent) {
  /**
   * When a process exits, its children become orphans
   * Orphan's continue normally, but we dont keep their PID when they die
   * Pseudocode: 
   *  for child in parent->children: 
   *    if child->state == ZOMBIE 
   *      pcb_destroy(child)
   *    else
   *      child->parent = NULL
   *  parent->children = NULL
   */
}

static void attach_zombie(pcb_t *parent, pcb_t *child) {
  /**
   * Move a dead child to its parent's zombie list. 
   * Pseudocode: 
   *  remove_child(parent, child)
   *  child->next_zombie = parent->zombies
   *  parent->zombies = child
   *  child->state = ZOMBIE
   */
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

  p->state = RUNNABLE;
  p->waiting_on = WAIT_NONE; 

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
   * 
   * Pseudocode: 
   * 
   * p->exit_status = status
   * if p is init process: 
   *  Halt()
   * 
   * // Free Region 1 frames
   * for (vpn = 0; vpn < MAX_PT_LEN; vpn++) 
   *    if p->region_1[vpn].valid
   *        frame_free(p->region_1[vpn].pfn)
   *        p->region_1[vpn].valid = 0
   * free(p->region_1)
   * p->region_1 = NULL
   * 
   * orphan_children(p)
   * 
   * // free any zombies
   * while p->zombies != NULL 
   *    zombie = p->zombies
   *    p->zombies = zombie->next_zombie
   *    pcb_destroy(zombie)
   * 
   * // Become parent's zombie
   * if p->parent != NULL and p->parent is alive: 
   *    attach_zombie(p->parent, p)
   *    
   *    // Enqueue parent if waiting on a child
   *    if p->parent->state == BLOCKED and p->parent->waiting_on == WAIT_CHILD: 
   *      p->parent->state = RUNNABLE
   *      p->parent->waiting_on = WAIT_NONE
   *      ready_enqueue(p->parent)
   * else: 
   *    pcb_destroy(p)
   * 
   * // Current process is either zombie or freed
   * // Schedule some other process
   * g_current_process = NULL
   * schedule()
   * 
   * // Should not return here
   */
}

void pcb_destroy(pcb_t *p) {
  /**
   * Fully release a PCB's resources
   * Assumes Region 1 frames and page table have already been freed.
   * Frees kernel stack frames, retires the PID, and frees the PCB struct
   *
   * Pseudocode:
   * if p == NULL return
   *
   * for (i = 0; i < KSTACK_NPG; i++)
   *    if p->kstack[i].valid: 
   *      frame_free(p->kstack[i].pfn)
   *      p->kstack[i].valid = 0
   * 
   * helper_retire_pid(p->pid); 
   * free(p)
   */
}

int pcb_clone(pcb_t *parent, pcb_t **child_out) {
  /**
   * Duplicate the current process's state into a new PCB
   * Allocates new PCB, copies Region 1 page by page
   * copies the user context, sets up parent-child pointers, and uses KCCopy to clone the
   * kernel context and stack
   *
   * On return, *child_out is set to a new PCB
   *
   * Pseudocode:
   * child = pcb_create()
   * if child == NULL return ERROR
   *
   * child->uctx = parent->uctx
   * child->ubrk = parent->ubrk
   * child->ustack_low = parent->ustack_low
   *
   * add_child(parent, child)
   *
   * // Copy Region 1 page-by-page
   * scratch_vpn = (KERNEL_STACK_BASE >> PAGESHIFT) - 1
   * for (vpn = 0; vpn < MAX_PT_LEN; vpn++)
   *    if not parent.region_1[vpn].valid:
   *      child.region_1[vpn].valid = 0
   *      continue
   *
   *    new_frame = frame_alloc()
   *    if new_frame == -1:
   *      for (v = 0; v < vpn; v++)
   *        if child.region_1[v].valid
   *          frame_free(child->region_1[v].pfn)
   *      remove_child(parent, child)
   *      pcb_destroy(child)
   *      return ERROR
   *    
   *    region_0_pt[scratch_vpn].valid = 1; 
   *    region_0_pt[scratch_vpn].prot  = PROT_READ | PROT_WRITE
   *    region_0_pt[scratch_vpn].pfn   = new_frame
   *    WriteRegister(REG_TLB_FLUSH, scratch_vpn << PAGESHIFT)
   * 
   *    src = VMEM_1_BASE + (vpn << PAGESIZE)
   *    dst = scratch_vpn << PAGE_SHIFT
   *    memcpy(dst, src, PAGESIZE)
   * 
   *    region_0_pt[scratch_vpn].valid = 0
   *    WriteRegister(REG_TLB_FLUSH, scratch_vpn << PAGESHIFT)
   *    
   *    // Install in child's page table.
   *    child->region_1[vpn].valid = 1
   *    child->region_1[vpn].prot  = parent->region_1[vpn].prot 
   *    child->region_1[vpn].pfn   = new_frame
   * 
   *  // Now copy kernel context and stack
   *  return_code = KernelContextSwitch(KCCopy, (void *)child, NULL)
   *  if return_code != 0: 
   *      for (v = 0; v < vpn; v++)
   *        if child.region_1[v].valid
   *          frame_free(child->region_1[v].pfn)
   *      remove_child(parent, child)
   *      pcb_destroy(child)
   *      return ERROR
   * 
   *  // Both parent and child execute past KernelContextSwitch
   *  // The child sits in the ready queue
   *  if g_current_process == parent: 
   *    // We are the parent
   *    child->uctx.regs[0] = 0
   *    child->state = RUNNABLE
   *    ready_enqueue(child)
   *    *child_out = child
   *    return 0
   *  else: 
   *    // We are the child
   *    *child_out = child
   *    return 0
   */

  return 0;
}

int pcb_load_program(pcb_t *p, char *filename, char **argv) {
  /**
   * Replace current process's program
   * Frees the current Region 1 frames, then calls LoadProgram to setup the new executable's
   * text, data, stack and initial UserContext
   * If LoadProgram fails, terminate the process
   * Returns 0 on success.
   *
   * Pseudocode:
   * // Free all of Region 1
   * for (vpn = 0; vpn < MAX_PT_LEN; vpn++) 
   *  if p->region_1[vpn].valid: 
   *    frame_free(p->region_1[vpn].pfn)
   *    p->region_1[vpn].valid = 0
   * 
   * WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1)
   * 
   * return_code = LoadProgram(filename, argv, p)
   * if return_code != 0: 
   *  pcb_terminate(p, ERROR)
   * 
   * p->ubrk = (data_pg1 + data_npg) << PAGESHIFT  + VMEM_1_BASE
   * p->ustack_low = (MAX_PT_LEN - stack_npg) << PAGESHIFT + VMEM_1_BASE
   * 
   * return 0
   */

  return 0;
}