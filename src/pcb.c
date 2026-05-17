#include <ykernel.h>

#include "pcb.h"
#include "frame.h"
#include "kernel.h"

// Creates fresh pcb, but does not load the program
pcb_t *pcb_create(void) {
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

  p->pid = helper_new_pid(p->region_1);

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

void pcb_terminate(pcb_t *, int);

// Frees the resources and state held by pcb
void pcb_destroy(pcb_t *); 

// Used for fork (allocates new PCB and clone the state)
int pcb_clone(pcb_t *parent, pcb_t **child_out); 

// Used to load the program into the current process
int pcb_load_program(pcb_t *p, char *filename, char **argv); 