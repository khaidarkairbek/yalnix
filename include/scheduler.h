#ifndef _yalnix_scheduler_h
#define _yalnix_scheduler_h
#include "pcb.h"

extern pcb_t *g_current_process;  // global currently running process
extern pcb_t *g_idle_process; 

/* Ready Queue Operations */
void ready_enqueue(pcb_t *);
pcb_t *ready_dequeue(void);
int ready_empty(void); 

/* Delay Queue Operations */
void delay_enqueue(pcb_t *);
void delay_tick(void);
pcb_t *delay_dequeue(void); 

/* Choose the next process to run*/
void schedule(void); 
/* Context switch from one process to the next */
void context_switch(pcb_t *next); 

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb, void *next_pcb); 
KernelContext *KCCopy(KernelContext *kc_in, void *curr_pcb, void *new_pcb);

#endif