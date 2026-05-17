#include <yalnix.h>
#include "trap_handler.h"
#include "syscalls.h"
#include "pcb.h"
#include "kernel.h"
#include "scheduler.h"

/*
  On the kernel start, write the handlers into `REG_VECTOR_BASE` with WriteRegister 
  to initialize the trap handlers. 

  The interrupt vector table must have exactly TRAP VECTOR SIZE entries in it, 
  even though the use for some of them is currently undefined by the hardware.
*/

static void (*vector_table[TRAP_VECTOR_SIZE])(UserContext *); 

static void handle_trap_unhandled(UserContext *uctx) {

  return; 
}

/* Trap Vector Table Initializer */
void trap_init(void) {
  for (int i = 0; i < TRAP_VECTOR_SIZE; i++) {
    vector_table[i] = handle_trap_unhandled; 
  }

  vector_table[TRAP_KERNEL] = handle_trap_kernel;
  vector_table[TRAP_CLOCK] = handle_trap_clock;
  vector_table[TRAP_MEMORY] = handle_trap_memory;
  vector_table[TRAP_ILLEGAL] = handle_trap_illegal;
  vector_table[TRAP_MATH] = handle_trap_math;
  vector_table[TRAP_TTY_RECEIVE] = handle_trap_tty_receive;
  vector_table[TRAP_TTY_TRANSMIT] = handle_trap_tty_transmit;
  vector_table[TRAP_DISK] = handle_trap_disk;
  WriteRegister(REG_VECTOR_BASE, (unsigned int)vector_table);
}; 

void handle_trap_kernel(UserContext *uctx) {
  /*
    TRAP_KERNEL
    Results from a "kernel call" executed by the current user process.
    Such a trap is used by user processes to request some type of service from the operating system kernel,
    such as creating a new process, allocating memory, or performing I/O
  */
  int return_code = 0;
  g_current_process->uctx = *uctx;  

  switch (uctx->code) {
  /*
    Process Coordination Syscalls
  */
  case YALNIX_NOP:
    return_code = kernel_Nop((int)uctx->regs[0], (int)uctx->regs[1], (int)uctx->regs[2], (int)uctx->regs[3]); 
    break;
  case YALNIX_FORK:
    return_code = kernel_Fork();
    break; 
  case YALNIX_EXEC:
    return_code = kernel_Exec((char *) uctx->regs[0], (char **) uctx->regs[1]);
    break; 
  case YALNIX_EXIT:
    kernel_Exit((int)uctx->regs[0]);
    break; 
  case YALNIX_GETPID:
    return_code = kernel_GetPid();
    break; 
  case YALNIX_BRK:
    return_code = kernel_Brk((void *) uctx->regs[0]);
    break; 
  case YALNIX_DELAY:
    return_code = kernel_Delay((int)uctx->regs[0]);
    break; 
  /*
    I/O Syscalls
  */
  case YALNIX_TTY_READ:
    return_code = kernel_TtyRead((int)uctx->regs[0], (void *)uctx->regs[1], (int)uctx->regs[2]);
    break; 
  case YALNIX_TTY_WRITE:
    return_code = kernel_TtyWrite((int)uctx->regs[0], (void *)uctx->regs[1], (int)uctx->regs[2]);
    break;
  /*
    Disk Syscalls
  */
  case YALNIX_READ_SECTOR:
    return_code = kernel_ReadSector((int)uctx->regs[0], (void *)uctx->regs[1]);
    break; 
  case YALNIX_WRITE_SECTOR:
    return_code = kernel_WriteSector((int)uctx->regs[0], (void *)uctx->regs[1]);
    break; 
  /*
    IPC Syscalls
  */
  case YALNIX_PIPE_INIT: 
    return_code = kernel_PipeInit((int *) uctx->regs[0]);
    break; 
  case YALNIX_PIPE_READ:
    return_code = kernel_PipeRead((int)uctx->regs[0], (void *)uctx->regs[1], (int)uctx->regs[2]);
    break; 
  case YALNIX_PIPE_WRITE:
    return_code = kernel_PipeWrite((int)uctx->regs[0], (void *)uctx->regs[1], (int)uctx->regs[2]);
    break;

  /*
    Semaphore Syscalls
  */
  case YALNIX_SEM_INIT:
    return_code = kernel_SemInit((int *)uctx->regs[0], (int)uctx->regs[1]);
    break; 
  case YALNIX_SEM_DOWN:
    return_code = kernel_SemDown((int)uctx->regs[0]);
    break; 
  case YALNIX_SEM_UP:
    return_code = kernel_SemUp((int)uctx->regs[0]);
    break; 
  /*
    Lock Syscalls
  */
  case YALNIX_LOCK_INIT:
    return_code = kernel_LockInit((int *)uctx->regs[0]);
    break; 
  case YALNIX_LOCK_ACQUIRE:
    return_code = kernel_Acquire((int)uctx->regs[0]);
    break; 
  case YALNIX_LOCK_RELEASE:
    return_code = kernel_Release((int)uctx->regs[0]);
    break; 

  /*
    Condition Variable Syscalls
  */
  case YALNIX_CVAR_INIT:
    return_code = kernel_CvarInit((int *)uctx->regs[0]);
    break; 
  case YALNIX_CVAR_WAIT:
    return_code = kernel_CvarWait((int)uctx->regs[0], (int)uctx->regs[1]);
    break; 
  case YALNIX_CVAR_SIGNAL:
    return_code = kernel_CvarSignal((int)uctx->regs[0]);
    break; 
  case YALNIX_CVAR_BROADCAST:
    return_code = kernel_CvarBroadcast((int)uctx->regs[0]);
    break;

  /*
    Destroy the lock, cvar or semaphore
  */
  case YALNIX_RECLAIM:
    return_code = kernel_Reclaim((int)uctx->regs[0]);
    break; 

  default:
    Halt(); 
  }

  (g_current_process->uctx).regs[0] = return_code;
  *uctx = g_current_process->uctx;
}

extern void handle_trap_clock(UserContext *uctx) {
  /*
    TRAP_CLOCK
    Results from the machine’s hardware clock, which generates periodic clock interrupts
    If there are runnable processes in ready queue, perform a context switch to the next process.
    Otherwise, dispatch idle. Also, responsible for waking the processes with Delay expired.

    Pseudocode:
      g_current_process->uctx = *uctx;
      // Wake any processes whose delay expired

      update_delay();

      schedule(); 

      *uctx = g_current_process->uctx;
  */
}

extern void handle_trap_illegal(UserContext *uctx) {
  /*
    TRAP_ILLEGAL
    Results from the execution of an illegal instruction by the currently executing user process.
    An illegal instruction can be an undefined machine language opcode, an illegal addressing mode,
    or a privileged instruction when not in kernel mode

    Abort the currently running process but continue running other processes.
    Pseudocode:
    g_current_process->uctx = *uctx;

    pcb_terminate(g_current_process, ERROR); 

    *uctx = g_current_process->uctx;
  */
}

extern void handle_trap_memory(UserContext *uctx) {
  /*
    TRAP_MEMORY
    Results from a disallowed memory access by the current user process.
    The access may be disallowed because:
    - the address is outside the virtual address range of the hardware (outside Region 0 and Region 1)
    - the address is not mapped in the current page tables
    - the access violates the page protection specified in the corresponding page table entry

    If the faulting address is in Region 1, below the current stack low and
    above the current break and red zone, grow the stack to cover.

    Otherwise, terminate the process.

    Pseudocode:
    g_current_process->uctx = *uctx;

    address = uctx->address;

    if address < VMEM_1_BASE or address > VMEM_1_LIMIT:
      pcb_terminate(g_current_process, ERROR);
      *uctx = g_current_process->uctx;
      return

    if address >= g_current_process->ustack_low:
      pcb_terminate(g_current_process, ERROR);
      *uctx = g_current_process->uctx;
      return

    if address < g_current_process->ubrk + PAGESIZE:
      pcb_terminate(g_current_process, ERROR);
      *uctx = g_current_process->uctx;
      return

    // Stack growth
    new_low_page = address >> PAGESHIFT
    old_low_page = g_current_process->ustack_low >> PAGESHIFT

    if frames_available() < (old_low_page - new_low_page):
      pcb_terminate(g_current_process, ERROR);
      *uctx = g_current_process->uctx;
      return

    for (vpn = new_low_page; vpn < old_low_page; vpn++)
      region_1_index = vpn.- (VMEM_1_BASE >> PAGESHIFT);
      frame = frame_alloc()
      g_current_process->region_1[region_1_index].valid = 1; 
      g_current_process->region_1[region_1_index].prot = PROT_READ | PROT_WRITE; 
      g_current_process->region_1[region_1_index].pfn = frame
      
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1)
    g_current_process->ustack_low = new_low_page << PAGESHIFT

    *uctx = g_current_process->uctx;
  */
}

extern void handle_trap_math(UserContext *uctx) {
  /*
    TRAP_MATH
    Results from any arithmetic error from an instruction executed by the current user process, 
    such as division by zero or an arithmetic overflow

    Pseudocode: 
    g_current_process->uctx = *uctx;

    pcb_terminate(g_current_process, ERROR); 

    *uctx = g_current_process->uctx;
  */
}

extern void handle_trap_tty_receive(UserContext *uctx) {
  /*
    TRAP_TTY_RECEIVE
    Generated by the terminal device controller hardware, when a complete line of input is available 
    from one of the terminals attached to the system

    Read the line into kernel buffer, wake blocked reader. 

    Pseudocode: 
    g_current_process->uctx = *uctx;

    tty_id = uctx->code
    buf = malloc(TERMINAL_MAX_LINE)
    if buf == NULL
      // silently dropping line
      // TODO: figure out what to do
      *uctx = g_current_process->uctx;
      return

    len = TtyReceive(tty_id, buf, TERMINAL_MAX_LINE)
    tty_receive_line(tty_id, buf, len)

    *uctx = g_current_process->uctx;
  */
}

extern void handle_trap_tty_transmit(UserContext *uctx) {
  /*
    TRAP_TTY_TRANSMIT
    Generated by the terminal device controller hardware, when the current buffer of data 
    previously given to the controller on a TtyTransmit instruction has been completely sent to
    the terminal

    Wake the writer that initiated a transmit

    Pseudocode: 
    g_current_process->uctx = *uctx;
    tty_id = uctx->code
    tty_transmit_complete(tty_id)
    *uctx = g_current_process->uctx;
  */
}

extern void handle_trap_disk(UserContext *uctx) {
  /*
    TRAP_DISK
    generated by the disk when it completes an operation

    TODO: ignore it for now. 
  */
}