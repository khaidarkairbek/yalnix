#include "pipes.h"
#include "scheduler.h"
#include "id_helper.h"

static pipe_t *g_pipes = NULL;

static pipe_t *pipe_find(int pipe_id) {
  pipe_t *pipe = g_pipes; 
  while (pipe != NULL && pipe->id != pipe_id) {
    pipe = pipe->next; 
  }

  return pipe; 
}

static pcb_t *waiter_dequeue(pcb_t **head) {
  pcb_t *p = *head; 
  if (p != NULL) {
    *head = p->next;
    p->next = NULL; 
  }

  return p; 
}

static void waiter_enqueue(pcb_t **head, pcb_t *p) {
  p->next = NULL; 
  if (*head == NULL) {
    *head = p;
    return;
  }

  pcb_t *tail = *head; 
  while (tail->next != NULL) {
    tail = tail->next; 
  }

  tail->next = p; 
}

static int waiter_remove(pcb_t **head, pcb_t *p) {
  if (*head == p) {
    *head = p->next;
    p->next = NULL;
    return 1;
  }
  pcb_t *prev = *head;
  while (prev != NULL && prev->next != p) {
    prev = prev->next;
  }
  if (prev != NULL) {
    prev->next = p->next;
    p->next = NULL;
    return 1;
  }
  return 0;
}

int pipe_create(void) {
  pipe_t *pipe = malloc(sizeof(pipe_t)); 
  if (pipe == NULL)
    return ERROR;

  pipe->buf = malloc(PIPE_BUFFER_LEN); 
  if (pipe->buf == NULL) {
    free(pipe);
    return ERROR; 
  }

  pipe->capacity = PIPE_BUFFER_LEN;
  pipe->count = 0;
  pipe->head = 0;
  pipe->tail = 0;
  pipe->read_waiters = NULL;
  pipe->write_waiters = NULL;

  pipe->id = helper_new_id();

  pipe->next = g_pipes;
  g_pipes = pipe;

  return pipe->id; 
}

int pipe_destroy(int pipe_id) {
  pipe_t *pipe = pipe_find(pipe_id); 
  if (pipe == NULL)
    return ERROR;

  TracePrintf(2, "pipe_destroy: id=%d\n", pipe_id); 

  /* Remove pipe from the pipe linked list */
  if (g_pipes == pipe) {
    g_pipes = pipe->next;
    pipe->next = NULL; 
  } else {
    pipe_t *prev = g_pipes; 
    while (prev != NULL && prev->next != pipe) {
      prev = prev->next; 
    }

    if (prev != NULL) {
      prev->next = pipe->next;
      pipe->next = NULL; 
    }
  }

  pcb_t *waiter; 
  while ((waiter = waiter_dequeue(&pipe->read_waiters)) != NULL) {
    waiter->uctx.regs[0] = ERROR;
    waiter->state = RUNNABLE;
    waiter->waiting_on = WAIT_NONE;

    ready_enqueue(waiter); 
  }

  while ((waiter = waiter_dequeue(&pipe->write_waiters)) != NULL) {
    waiter->uctx.regs[0] = ERROR;
    waiter->state = RUNNABLE;
    waiter->waiting_on = WAIT_NONE;

    ready_enqueue(waiter); 
  }

  helper_retire_id(pipe->id); 

  free(pipe->buf);
  free(pipe); 

  return 0; 
}


int pipe_read(int pipe_id, void *buf, int len) {
  pipe_t *pipe = pipe_find(pipe_id); 

  if (pipe == NULL)
    return ERROR; 

  while (pipe->count == 0) {
    g_current_process->state = BLOCKED;
    g_current_process->waiting_on = WAIT_PIPE_READ;
    g_current_process->wait_arg = pipe_id;

    waiter_enqueue(&pipe->read_waiters, g_current_process); 

    schedule(); 

    /* Check that pipe still exists */
    pipe = pipe_find(pipe_id); 
    if (pipe == NULL)
      return ERROR;
  }

  int to_copy = len > pipe->count ? pipe->count : len;
  for (int i = 0; i < to_copy; ++i) {
    ((char *)buf)[i] = pipe->buf[pipe->head]; 
    pipe->head = (pipe->head + 1) % pipe->capacity;
    pipe->count--;
  }

  pcb_t *write_waiter; 
  if ((write_waiter = waiter_dequeue(&pipe->write_waiters)) != NULL) {
    write_waiter->next = NULL;
    write_waiter->state = RUNNABLE;
    write_waiter->waiting_on = WAIT_NONE;
    ready_enqueue(write_waiter); 
  }

  return to_copy; 
}

int pipe_write(int pipe_id, void *buf, int len) {
  pipe_t *pipe = pipe_find(pipe_id); 

  if (pipe == NULL)
    return ERROR;

  int written = 0; 
  while (written < len) {
    while (written < len && pipe->count < pipe->capacity) {
      pipe->buf[pipe->tail] = ((char *)buf)[written]; 
      pipe->tail = (pipe->tail + 1) % pipe->capacity;
      pipe->count++;
      written++; 
    }

    pcb_t *read_waiter; 
    if ((read_waiter = waiter_dequeue(&pipe->read_waiters)) != NULL) {
      read_waiter->next = NULL;
      read_waiter->state = RUNNABLE; 
      read_waiter->waiting_on = WAIT_NONE; 
      ready_enqueue(read_waiter);
    }

    // If more to write, block 
    if (written < len) {
      g_current_process->state = BLOCKED;
      g_current_process->waiting_on = WAIT_PIPE_WRITE;
      g_current_process->wait_arg = pipe_id;

      waiter_enqueue(&pipe->write_waiters, g_current_process); 

      schedule(); 

      /* Check that pipe still exists */
      pipe = pipe_find(pipe_id);
      if (pipe == NULL)
        return (written > 0) ? written : ERROR; 
    }
  }

  return written; 
}

void pipe_remove_waiter(pcb_t *p) {
  /* Used by kernel_Exit */
  pipe_t *pipe = g_pipes;
  while (pipe != NULL) {
    if (waiter_remove(&pipe->read_waiters, p)) return;
    if (waiter_remove(&pipe->write_waiters, p)) return;
    pipe = pipe->next;
  }
}