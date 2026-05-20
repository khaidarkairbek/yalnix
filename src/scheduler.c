#include "scheduler.h"

/**
 * The scheduler state 
 */
pcb_t *g_current_process = NULL;
pcb_t *g_idle_process = NULL;

/* Ready Queue State */
static pcb_t *ready_head = NULL;
static pcb_t *ready_tail = NULL;

/* Ready Queue Operations */
void ready_enqueue(pcb_t *p) {
  /*
   * Add p to the tail of the ready queue.
   * Caller is responsible for setting p->state = RUNNABLE
   * Do not enqueue idle process - idle process is a fallback when the queue is empty
   */

  if (p == g_idle_process)
    return;

  p->next = NULL; 
  if (ready_tail == NULL) {
    ready_tail = ready_head = p; 
  } else {
    ready_tail->next = p;
    ready_tail = p;
  };
}
pcb_t *ready_dequeue(void) {
  /**
   * Remove and return the head of the ready queue.
   * If empty returns NULL
   */
  if (ready_head == NULL) {
    return NULL;
  }

  pcb_t *head = ready_head;
  ready_head = ready_head->next; 
  if (ready_head == NULL) {
    ready_tail = NULL; 
  }

  head->next = NULL;
  return head;
}
int ready_empty(void) {
  /**
   * Returns 1 if ready queue is empty.
   */
  return ready_head == NULL; 
}

/* High-Level Scheduling Operations*/
void schedule(void) {
  /**
   * Pick the next process to run and switch to it. (Round-Robin Policy)
   * - TRAP_CLOCK handler
   * - Syscall that blocks
   * - Process exit
   */

  if (g_current_process->state == RUNNING && g_current_process != g_idle_process) {
    g_current_process->state = RUNNABLE;
    ready_enqueue(g_current_process); 
  }

  pcb_t *next = ready_dequeue(); 
  if (next == NULL) {
    next = g_idle_process; 
  }

  if (next == g_current_process) {
    g_current_process->state = RUNNING;
    return; 
  }

  next->state = RUNNING;
  context_switch(next); 
}
void context_switch(pcb_t *next) {
  /**
   * Switch from current process to next. Assumes the caller has
   * updated states (current state is not RUNNING, next state is RUNNING)
   */

  if (next == g_current_process)
    return;

  pcb_t *prev = g_current_process;
  // Install Next's Region 1 Page Table
  WriteRegister(REG_PTBR1, next->region_1);
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1); 

  // KCSwitch handles the kernel stack remapping and kctx swap
  // Updates current_process internally
  if (KernelContextSwitch(KCSwitch, (void *) prev, (void *) next) != 0) {
    TracePrintf(0, "context_switch: failed KernelContextSwitch\n");
    Halt();
  }

  // When we reach here, we are back to running as prev, after some future switch
}

KernelContext *KCSwitch (KernelContext *kc_in, void *curr_pcb, void *next_pcb) {
  /**
   * Called by KernelContextSwitch on a special context outside of kernel. Saves the current kernel
   * context into current process pcb, remaps the kernal stack to next's frames and returns the pointer to
   * the next's kernel context.
   *
   * When returns and the framework sets next->kctx:
   * - the pc loaded points somewhere in KernelContextSwitch's return path, which
   *   will return into next's context_switch caller
   * - the sp loaded points into [KERNEL_STACK_BASE, KERNEL_STACK_LIMIT)
   *   which now reads from next's frames due to remapping
   * - so execution resumes from next's point of view
   */

  pcb_t *curr = (pcb_t *)curr_pcb;
  pcb_t *next = (pcb_t *)next_pcb;

  // Save curr kernel context
  curr->kctx = *kc_in; 
  // Remap kernel stack pages
  int kstack_base_vpn = KERNEL_STACK_BASE >> PAGESHIFT; 
  for (int i = 0; i < KSTACK_NPG; i++) {
    g_region_0_pt[kstack_base_vpn + i].valid = 1;
    g_region_0_pt[kstack_base_vpn + i].prot = PROT_READ | PROT_WRITE;
    g_region_0_pt[kstack_base_vpn + i].pfn = next->kstack[i].pfn; 
  }

  // Flush kernel stack TLB
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK);

  g_current_process = next; 

  return &next->kctx; 
}

KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb, void *) {
  /**
   * Used during forking the current process.
   *
   * Copies the current kernel context and kernel stack contents into a newly created PCB.
   * Unlike KCSwitch, KCCopy does NOT switch execution - the caller continues executing afterwards.
   * The new PCB ends up in a state such that, when some future switch dispatches it, it will resume
   * after the KernelContextSwitch call with its own kernel stack.
   */

  pcb_t *new = (pcb_t *)new_pcb;
  new->kctx = *kc_in; 

  // Copy current kernel stack frames into new kernel stack frames
  // We will use scratch page just below the current kernel stack

  int scratch_vpn = (KERNEL_STACK_BASE >> PAGESHIFT) - 1;
  for (int i = 0; i < KSTACK_NPG; i++) {
    // Temporarily map into the scratch page
    g_region_0_pt[scratch_vpn].valid = 1;
    g_region_0_pt[scratch_vpn].prot = PROT_READ | PROT_WRITE; 
    g_region_0_pt[scratch_vpn].pfn = new->kstack[i].pfn;
    WriteRegister(REG_TLB_FLUSH, scratch_vpn << PAGESHIFT);

    // Copy from current kernel stack to new kernel stack
    int src = KERNEL_STACK_BASE + i * PAGESIZE;
    int dst = scratch_vpn << PAGESHIFT;
    memcpy(dst, src, PAGESIZE);

    g_region_0_pt[scratch_vpn].valid = 0;
    WriteRegister(REG_TLB_FLUSH, scratch_vpn << PAGESHIFT); 
  }

  return kc_in;
}