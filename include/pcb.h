#ifndef _yalnix_pcb_h
#define _yalnix_pcb_h

#include <hardware.h>

enum proc_state { BLOCKED, RUNNABLE, RUNNING, ZOMBIE }; 

enum wait_reason {
  NONE, 
  WAIT_DELAY, 
  WAIT_CHILD, 
  WAIT_TTY_READ, 
  WAIT_TTY_WRITE, 
  WAIT_PIPE_READ, 
  WAIT_PIPE_WRITE, 
  WAIT_LOCK, 
  WAIT_CVAR
  // ...
}; 

typedef struct pcb_s {
  int pid;  

  pte_t *region_1; 
  pte_t kstack[KERNEL_STACK_MAXSIZE / PAGESIZE]; 
  unsigned int ubrk; 
  unsigned int ustack_low; 
  
  UserContext uctx; 
  KernelContext kctx; 

  enum proc_state state;
  enum wait_reason waiting_on; 

  struct pcb_s *parent;
  struct pcb_s *children; // head of children
  struct pcb_s *next_sibling; 
  struct pcb_s *zombies; // head of dead children list
  struct pcb_s *next_zombie; 

  int exit_status;

  struct pcb_s *next; // for the process queue
} pcb_t; 

#endif