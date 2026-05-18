#ifndef _yalnix_pcb_h
#define _yalnix_pcb_h

#include "kernel.h"

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
  pte_t kstack[KSTACK_NPG]; 
  unsigned int ubrk; 
  unsigned int ustack_low;
  unsigned int udata_end; 

  UserContext uctx; 
  KernelContext kctx; 

  enum proc_state state;
  enum wait_reason waiting_on;
  int wait_arg; 

  struct pcb_s *parent;
  struct pcb_s *children; // head of children list
  struct pcb_s *next_sibling; 
  struct pcb_s *zombies; // head of dead children list
  struct pcb_s *next_zombie; 

  int exit_status;

  struct pcb_s *next; // for the process queue
} pcb_t; 

/* Creates fresh PCB without loading program */
pcb_t *pcb_create(void); 

/* Terminates PCB and calls destroy if needed */
void pcb_terminate(pcb_t *, int);

/* Frees all the remaining resources abd frees PCB */
void pcb_destroy(pcb_t *); 

/* Clones PCB and parent state */
int pcb_clone(pcb_t *parent, pcb_t **child_out); 

/* Loads the user program into PCB*/
int pcb_load_program(pcb_t *p, char *filename, char **argv); 

#endif