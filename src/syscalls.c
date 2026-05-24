#include "syscalls.h"
#include "pcb.h"
#include "kernel.h"
#include "scheduler.h"
#include "validate.h"
#include "frame.h"
#include "tty.h"

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
   */

  pcb_t *child; 
  if (pcb_clone(g_current_process, &child) == ERROR)
    return ERROR;

  if (g_current_process == child)
    return 0;

  return child->pid;
}

int kernel_Exec (char *filename, char **argv) {
  /**
   * Replaces the current process's program
   */

  /* Validate filename */
  if (validate_user_string(filename) == ERROR) {
    TracePrintf(1, "kernel_Exec: validate_user_string failed\n"); 
    return ERROR;
  }

  /* Validate argv array */
  if (argv == NULL)
    return ERROR; 

  if (validate_user_buffer(argv, sizeof(char *), PROT_READ) == ERROR) {
    TracePrintf(1, "kernel_Exec: validate_user_buffer failed\n");
    return ERROR; 
  }

  int i;
  for (i = 0;; i++) {
    if (i > MAX_ARGS)
      return ERROR; 

    if (validate_user_buffer(&argv[i], sizeof(char *), PROT_READ) == ERROR) {
      return ERROR; 
    }

    if (argv[i] == NULL) break;
    if (validate_user_string(argv[i]) == ERROR) return ERROR;
  }

  TracePrintf(1, "Exec: pid=%d filename=%s\n", g_current_process->pid, filename);

  return pcb_load_program(g_current_process, filename, argv);
}

void kernel_Exit (int status) {
  /**
   * Terminates the current process; 
   */

  // terminate the current running process
  pcb_terminate(g_current_process, status);

  // should not reach here.
  Halt(); 
}

int kernel_Wait (int *status_ptr) {
  /**
   * Collect's an exited child's pid and status.
   * Cases:
   * - Zombie child: pop it from zombie list, copy its pid and status, destroy it's PCB.
   * - No children: return ERROR;
   * - No zombies but live children: wait until child exits.
   */

  if (status_ptr != NULL && validate_user_buffer(status_ptr, sizeof(int), PROT_READ | PROT_WRITE) == ERROR) {
    return ERROR;
  }

  for (;;){
    if (g_current_process->zombies != NULL) {
      pcb_t *zombie = g_current_process->zombies;
      g_current_process->zombies = zombie->next_zombie; 

      int pid = zombie->pid;
      int status = zombie->exit_status; 

      pcb_destroy(zombie); 

      if (status_ptr != NULL) {
        *status_ptr = status; 
      }

      return pid; 
    }

    if (g_current_process->children == NULL) {
      return ERROR; 
    }

    g_current_process->state = BLOCKED;
    g_current_process->waiting_on = WAIT_CHILD; 
    schedule(); 
    // when wake up, loop back and check zombies
  }

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
   */

  int new_brk = UP_TO_PAGE(address); 
  int old_brk = UP_TO_PAGE(g_current_process->ubrk); 

  if (new_brk < g_current_process->udata_end){
    return ERROR; 
  }

  if (new_brk >= g_current_process->ustack_low - PAGESIZE) {
    return ERROR; 
  }

  if (new_brk > old_brk) {
    // Grow
    int needed_npg = (new_brk - old_brk) >> PAGESHIFT; 
    if (frames_available() < needed_npg) {
      return ERROR; 
    }

    for (int byte_addr = old_brk; byte_addr < new_brk - PAGESIZE; byte_addr += PAGESIZE) {
      int r1_index = (byte_addr >> PAGESHIFT) - (VMEM_1_BASE >> PAGESHIFT);
      int new_frame = frame_alloc(); 
      g_current_process->region_1[r1_index].valid = 1; 
      g_current_process->region_1[r1_index].prot = PROT_READ | PROT_WRITE;
      g_current_process->region_1[r1_index].pfn = new_frame; 
    }
  } else if (new_brk < old_brk) {
    // Shrink
    for (int byte_addr = new_brk; byte_addr < old_brk - PAGESIZE; byte_addr += PAGESIZE) {
      int r1_index = (byte_addr >> PAGESHIFT) - (VMEM_1_BASE >> PAGESHIFT);
      if (g_current_process->region_1[r1_index].valid == 1) {
        frame_free(g_current_process->region_1[r1_index].pfn);
        g_current_process->region_1[r1_index].valid = 0;
      }
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
  }

  g_current_process->ubrk = new_brk; 
  return SUCCESS;
}
int kernel_Delay (int ticks){
  /**
   * Block for `ticks` clock interrupts
   */

  if (ticks < 0)
    return ERROR; 
  
  if (ticks == 0)
    return SUCCESS;

  g_current_process->state = BLOCKED;
  g_current_process->waiting_on = WAIT_DELAY;
  g_current_process->wait_arg = ticks; 

  // Put the process into the queue of delayed processes
  delay_enqueue(g_current_process); 

  schedule(); 
  
  return SUCCESS; 
}
int kernel_TtyRead (int tty_id, void *buf, int len){
  /**
   * Read a line of input from terminal.
   *
   * If input is available, return immediately. Otherwise block until TRAP_TTY_RECEIVE handler.
   * Returns the number of bytes copied. If greater than len, wait for the next read.
   */

  if (tty_id < 0 || tty_id >= NUM_TERMINALS) {
    return ERROR; 
  }

  if (len < 0) {
    return ERROR; 
  }

  if (len == 0) {
    return SUCCESS; 
  }

  if (validate_user_buffer(buf, len, PROT_WRITE) == ERROR) {
    return ERROR;
  }

  for (;;) {
    if (tty_has_input(tty_id)) {
      return tty_consume_input(tty_id, buf, len); 
    }

    g_current_process->state = BLOCKED; 
    g_current_process->waiting_on = WAIT_TTY_READ;
    g_current_process->wait_arg = tty_id;

    tty_add_read_waiter(g_current_process, tty_id);

    schedule();

    // When wake up, recheck the terminal
  }
  return SUCCESS; 
}
int kernel_TtyWrite (int tty_id, void *buf, int len){
  /**
   * Write len bytes from buffer to terminal
   * Blocks until all bytes have been written. May require multiple TtyTransmit calls. 
   */

  if (tty_id < 0 || tty_id >= NUM_TERMINALS){
    return ERROR; 
  }

  if (len < 0){
    return ERROR; 
  }

  if (len == 0) {
    return SUCCESS; 
  }

  if (validate_user_buffer(buf, len, PROT_READ) == ERROR) {
    return ERROR; 
  } 

  // Kernel-side buffer
  void *kbuf = malloc(len); 
  if (kbuf == NULL) {
    TracePrintf(0, "kernel_TtyWrite: malloc failed\n");
    return ERROR; 
  }

  tty_write_start(tty_id, kbuf, len, g_current_process);

  g_current_process->state = BLOCKED;
  g_current_process->waiting_on = WAIT_TTY_WRITE;
  g_current_process->wait_arg = tty_id;

  schedule(); 

  return len;
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