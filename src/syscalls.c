#include "syscalls.h"
#include "pcb.h"
#include "kernel.h"
#include "scheduler.h"

#include <ykernel.h>

int kernel_Nop (int a, int b, int c, int d) {
  /**
   * No-op syscall. 
   */
  return 0; 
}

int kernel_Fork (void) {
  /**
   * Clone the current process.
   * - parent returns child's pid
   * - child returns 0
   *
   * Pseudocode:
   *  pcb_t *child;
   *  if pcb_clone(g_current_process, &child) == ERROR:
   *    return ERROR;
   *
   *  if g_current_process == child:
   *    return 0;
   *
   *  return child->pid; 
   */

  return 0; 
}

int kernel_Exec (char *filename, char **argv) {
  /**
   * Replaces the current process's program
   *
   * Pseudocode:
   * if validate_filename(filename) == ERROR:
   *  return ERROR;
   *
   * if validate_arguments(argv) == ERROR:
   *  return ERROR;
   *
   * if pcb_load_program(g_current_process, filename, argv) == ERROR:
   *  return ERROR;
   *
   * return 0; 
   */
  return ERROR; 
}

void kernel_Exit (int status) {
  /**
   * Terminates the current process; 
   */

  // terminate the current running process
  pcb_terminate(g_current_process, status);

  // should not reach here.
  Halt(); 

  return ERROR;
}

int kernel_Wait (int *status_ptr) {
  /**
   * Collect's an exited child's pid and status.
   * Cases:
   * - Zombie child: pop it from zombie list, copy its pid and status, destroy it's PCB.
   * - No children: return ERROR;
   * - No zombies but live children: wait until child exits.
   *
   * Pseudocode:
   * if status_ptr != NULL:
   *  if validate_user_buffer(status_ptr, sizeof(int), PROT_READ | PROT_WRITE) == ERROR:
   *    return ERROR;
   *
   * while true:
   *  if g_current_process->zombies != NULL:
   *    zombie = g_current_process->zombies;
   *    g_current_process->zombies = zombie->next_zombie;
   *
   *    pid = zombie->pid;
   *    status = zombie->exit_status
   *
   *    pcb_destroy(zombie);
   *
   *    if status_ptr != NULL:
   *      *status_ptr = status;
   *    return pid;
   *  if g_current_process->children == NULL:
   *    return ERROR;
   *
   *  g_current_process->state = BLOCKED;
   *  g_current_process->waiting_on = WAIT_CHILD;
   *  schedule(); 
   *  // when wake up, loop back and check zombies
   */

  return ERROR; 
}

int kernel_GetPid (void) {
  return g_current_process->pid; 
}
int kernel_Brk (void *address){
  /**
   * Change the user heap break.
   * - greater than or equal to the bottom of the heap
   * - less than or equal to the bottom of stack minus one page
   *
   * Pseudocode:
   * new_brk = UP_TO_PAGE(address)
   * old_brk = UP_TO_PAGE(g_current_process->ubrk)
   *
   * if new_brk < g_current_process->udata_end;
   *  return ERROR;
   *
   * if new_brk >= g_current_process->ustack_low - PAGESIZE:
   *  return ERROR;
   *
   * if new_brk > old_brk:
   *    // Grow
   *    needed = (new_brk - old_brk) >> PAGESHIFT;
   *    if frames_available() < needed:
   *      return ERROR;
   *
   *    for (byte_addr = old_brk; vpn < new_brk - PAGESIZE; byte_addr + PAGESIZE)
   *      vpn   = byte_addr >> PAGESHIFT
   *      r1_index = vpn - (VMEM_1_BASE >> PAGESHIFT)
   *      frame = frame_alloc()
   *      g_current_process->region_1[r1_index].valid = 1
   *      g_current_process->region_1[r1_index].prot = PROT_READ | PROT_WRITE
   *      g_current_process->region_1[r1_index].pfn = frame
   * else if new_brk < old_brk:
   *    // Shrink
   *    for (byte_addr = new_brk; vpn < old_brk - PAGESIZE; byte_addr + PAGESIZE)
   *      vpn   = byte_addr >> PAGESHIFT
   *      r1_index = vpn - (VMEM_1_BASE >> PAGESHIFT)
   *      if g_current_process->region_1[r1_idx].valid:
   *        frame_free(g_current_process->region_1[r1_idx].pfn)
   *        g_current_process->region_1[r1_idx].valid = 0
   *    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1)
   *
   * g_current_process->ubrk = new_brk;
   * return 0;
   */
  return ERROR; 
}
int kernel_Delay (int ticks){
  /**
   * Block for `ticks` clock interrupts
   *
   * Pseudocode:
   * if ticks < 0:
   *  return ERROR;
   *
   * if ticks == 0:
   *  return 0;
   *
   * g_current_process->state = BLOCKED;
   * g_current_process->waiting_on = WAIT_DELAY;
   * g_current_process->wait_arg = ticks;
   *
   * // Put the process into the queue of delayed processes
   * delay_enqueue(g_current_process);
   *
   * schedule();
   * 
   * return 0; 
   */

  return ERROR; 
}
int kernel_TtyRead (int tty_id, void *buf, int len){
  /**
   * Read a line of input from terminal.
   *
   * If input is available, return immediately. Otherwise block until TRAP_TTY_RECEIVE handler.
   * Returns the number of bytes copied. If greater than len, wait for the next read.
   *
   * Pseudocode:
   * if tty_id < 0 or tty_id >= NUM_TERMINALS:
   *  return ERROR;
   *
   * if len < 0:
   *  return ERROR;
   * if len == 0:
   *  return 0;
   *
   * if validate_user_buffer(buf, len, PROT_WRITE) == ERROR:
   *  return ERROR
   *
   * while true:
   *  if tty_has_input(tty_id):
   *    n = tty_consume_input(tty_id, buf, len)
   *    return n
   *
   *  g_current_process->state = BLOCKED;
   *  g_current_process->waiting_on = WAIT_TTY_READ; 
   *  g_current_process->wait_arg   = tty_id
   *  
   *  schedule()
   *  // When wake up, recheck the terminal
   * 
   */
  return ERROR; 
}
int kernel_TtyWrite (int tty_id, void *buf, int len){
  /**
   * Write len bytes from buffer to terminal
   * Blocks until all bytes have been written. May require multiple TtyTransmit calls.
   *
   * Pseudocode:
   * if tty_id < 0 or tty_id >= NUM_TERMINALS:
   *    return ERROR;
   *
   * if len < 0:
   *    return ERROR;
   *
   * if len == 0:
   *    return 0;
   *
   * if validate_user_buffer(buf, len, PROT_READ) == ERROR:
   *  return ERROR
   *
   * // kernel-side buffer
   * kbuf = malloc(len)
   * if kbuf == NULL:
   *  return ERROR
   * memcpy(kbuf, buf, len);
   *
   * tty_write_start(tty_id, kbuf, len, g_current_process)
   *
   * g_current_process->state = BLOCKED;
   * g_current_process->waiting_on = WAIT_TTY_WRITE;
   * g_current_process->wait_arg = tty_id
   *
   * schedule();
   *
   * return len; 
   */
  return ERROR; 
}

int kernel_ReadSector (int, void *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_WriteSector (int, void *){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_PipeInit (int *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_PipeRead (int, void *, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_PipeWrite (int, void *, int){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_SemInit (int *, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_SemUp (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_SemDown (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_LockInit (int *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_Acquire (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_Release (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarInit (int *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarWait (int, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarSignal (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarBroadcast (int){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_Reclaim (int){
  // UNIMPLEMENTED
  return ERROR; 
}