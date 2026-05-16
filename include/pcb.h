#ifndef _yalnix_pcb_h
#define _yalnix_pcb_h

#include <hardware.h>

enum proc_state { BLOCKED, RUNNABLE, RUNNING, ZOMBIE }; 

enum wait_reason {
  WAIT_NONE, 
  WAIT_DELAY, 
  WAIT_CHILD, 
  WAIT_TTY_READ, 
  WAIT_TTY_WRITE, 
  WAIT_PIPE_READ, 
  WAIT_PIPE_WRITE, 
  WAIT_LOCK, 
  WAIT_CVAR
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
  struct pcb_s *children; // head of children list
  struct pcb_s *next_sibling; 
  struct pcb_s *zombies; // head of dead children list
  struct pcb_s *next_zombie; 

  int exit_status;

  struct pcb_s *next; // for the process queue
} pcb_t; 

extern pcb_t *g_current_process;  // global currently running process

// Creates fresh pcb, but does not load the program
pcb_t *pcb_create(void); 

void pcb_terminate(pcb_t *, int);

// Frees the resources and state held by pcb
void pcb_destroy(pcb_t *); 

// Used for fork (allocates new PCB and clone the state)
int pcb_clone(pcb_t *parent, pcb_t **child_out); 

// Used to load the program into the current process
int pcb_load_program(pcb_t *p, char *filename, char **argv); 

#endif