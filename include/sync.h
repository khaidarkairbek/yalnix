#ifndef _yalnix_sync_h
#define _yalnix_sync_h

#include "pcb.h"

typedef struct lock_s {
  int id;
  pcb_t *owner;
  pcb_t *waiters;
  struct lock_s *next; 
} lock_t;

int lock_init(void);
int lock_acquire(int); 
int lock_release(int);
int lock_destroy(int);
void lock_remove_waiter(pcb_t *); 

int cvar_init(void); 
int cvar_signal(int); 
int cvar_broadcast(int); 
int cvar_wait(int); 

#endif