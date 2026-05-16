#ifndef _yalnix_scheduler_h
#define _yalnix_scheduler_h
#include "pcb.h"

// Choose the next process to run
void schedule(void); 
// Context switch from current process to next process
void context_switch(pcb_t *next); 


#endif