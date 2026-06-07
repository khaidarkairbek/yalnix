#ifndef _yalnix_pipes_h
#define _yalnix_pipes_h

#include <ykernel.h>
#include "pcb.h"

typedef struct pipe_s {
  int id;

  char *buf; 
  int capacity; // total buffer size
  int head;  // next byte to read
  int tail;  // next byte to write
  int count; // bytes in buffer

  pcb_t *read_waiters; // readers blocked by empty buffer
  pcb_t *write_waiters; // writers blocked by full buffer

  struct pipe_s *next;
} pipe_t;

int pipe_destroy(int); 
int pipe_create(void); 
int pipe_read(int pipe_id, void *buf, int len); 
int pipe_write(int pipe_id, void *buf, int len);

void pipe_remove_waiter(pcb_t *p); 

#endif