#include <yalnix.h>
#include "trap_handler.h"
#include "syscalls.h"
#include "pcb.h"

/*
  On the kernel start, write the handlers into `REG_VECTOR_BASE` with WriteRegister 
  to initialize the trap handlers. 

  The interrupt vector table must have exactly TRAP VECTOR SIZE entries in it, 
  even though the use for some of them is currently undefined by the hardware.
*/

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
    // TODO: panic
  }

  (g_current_process->uctx).regs[0] = return_code;
  *uctx = g_current_process->uctx;
}

extern void handle_trap_clock(UserContext *uctx) {
  /*
    TRAP_CLOCK
    Results from the machine’s hardware clock, which generates periodic clock interrupts
  */
  
}

extern void handle_trap_illegal(UserContext *uctx) {
  /*
    TRAP_ILLEGAL
    Results from the execution of an illegal instruction by the currently executing user process. 
    An illegal instruction can be an undefined machine language opcode, an illegal addressing mode, 
    or a privileged instruction when not in kernel mode
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
  */
}

extern void handle_trap_math(UserContext *uctx) {
  /*
    TRAP_MATH
    Results from any arithmetic error from an instruction executed by the current user process, 
    such as division by zero or an arithmetic overflow
  */
}

extern void handle_trap_tty_receive(UserContext *uctx) {
  /*
    TRAP_TTY_RECEIVE
    Generated by the terminal device controller hardware, when a complete line of input is available 
    from one of the terminals attached to the system
  */
}

extern void handle_trap_tty_transmit(UserContext *uctx) {
  /*
    TRAP_TTY_TRANSMIT
    Generated by the terminal device controller hardware, when the current buffer of data 
    previously given to the controller on a TtyTransmit instruction has been completely sent to
    the terminal
  */
}

extern void handle_trap_disk(UserContext *uctx) {
  /*
    TRAP_DISK
    generated by the disk when it completes an operation
  */
}