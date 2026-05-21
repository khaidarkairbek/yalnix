#include "tty.h"
#include "kernel.h"
#include "scheduler.h"
#include <ykernel.h>

/* The line waiting to be consumed by TtyRead */
typedef struct input_line_s {
  char *data; // kernel buffer
  int len;    // total bytes
  int offset; // bytes already consumed
  struct input_line_s *next; 
} input_line_t; 

/* Active or pending write */
typedef struct write_req_s {
  pcb_t *writer; 
  char  *kbuf;
  int    total_len;
  int    sent;
  struct write_req_s *next;
} write_req_t; 

/* The state of the terminal both on read and write side */
typedef struct tty_state_s {
  input_line_t *input_head;
  input_line_t *input_tail;
  pcb_t *read_waiters; 

  write_req_t  *active_write;
  write_req_t  *write_queue;
} tty_state_t;

static tty_state_t terminals[NUM_TERMINALS]; 

void tty_init(void) {
  for (int i = 0; i < NUM_TERMINALS; i++) {
    terminals[i].input_head = NULL; 
    terminals[i].input_tail = NULL;
    terminals[i].read_waiters = NULL; 

    terminals[i].active_write = NULL;
    terminals[i].write_queue = NULL; 
  }
}

int tty_has_input(int tty_id) {
  /**
   * True if there's at least one buffered input line on tty_id
   */
  return terminals[tty_id].input_head != NULL; 
}

int tty_consume_input(int tty_id, void *buf, int len) {
  /**
   * Copy up to len bytes from the head of the input queue into
   * buffer. Returns the number of bytes copied.
   */

  input_line_t *line = terminals[tty_id].input_head; 
  if (line == NULL) return 0; 

  int available = line->len - line->offset;
  int to_copy = available > len ? len : available;
  memcpy(buf, line->data + line->offset, to_copy);
  line->offset += to_copy; 

  if (line->offset >= line->len) {
    terminals[tty_id].input_head = line->next; 
    if (terminals[tty_id].input_head == NULL) {
      terminals[tty_id].input_tail = NULL; 
    }

    free(line->data);
    free(line); 
  }

  return to_copy; 
}

void tty_add_read_waiter(pcb_t *p, int tty_id) {
  /**
   * Adds the process to the tty_id reader queue.
   */

  p->next = NULL; 
  if (terminals[tty_id].read_waiters == NULL) {
    terminals[tty_id].read_waiters = p; 
  } else {
    pcb_t *waiter = terminals[tty_id].read_waiters; 
    while (waiter->next != NULL) {
      waiter = waiter->next; 
    }

    waiter->next = p; 
  }
}

static void send_next_chunk(int tty_id) {
  write_req_t *write_request = terminals[tty_id].active_write;
  int remaining = write_request->total_len - write_request->sent;
  int to_send = remaining > TERMINAL_MAX_LINE ? TERMINAL_MAX_LINE : remaining;

  TtyTransmit(tty_id, write_request->kbuf + write_request->sent, to_send);
  write_request->sent += to_send; 
}

void tty_write_start(int tty_id, void *kbuf, int len, pcb_t *writer) {
  /**
   * Begin a write of len bytes from kbuf to tty_id on behalf of writer.
   *
   * If the terminal is idle, start the TtyTransmit for the first chunk. Otherwise, queue the writers
   * to wait until terminal is idle.
   */

  write_req_t *write_request = malloc(sizeof(write_req_t)); 
  if (write_request == NULL) {
    TracePrintf(0, "tty_write_start: malloc failed\n"); 
    // TODO: figure out what to do.
    Halt(); 
  }

  write_request->writer = writer;
  write_request->kbuf;
  write_request->total_len = len;
  write_request->sent = 0; 
  write_request->next = NULL;
  
  if (terminals[tty_id].active_write == NULL) {
    terminals[tty_id].active_write = write_request;
    send_next_chunk(tty_id); 
  } else {
    if (terminals[tty_id].write_queue == NULL) {
      terminals[tty_id].write_queue = write_request; 
    } else {
      write_req_t *write = terminals[tty_id].write_queue; 
      while (write->next != NULL) {
        write = write->next; 
      }
      write->next = write_request; 
    }
  }
}

void tty_receive_line(int tty_id, void *kbuf, int len) {
  /**
   * Called from TRAP_TTY_RECEIVE handler
   */

  input_line_t *line = malloc(sizeof(input_line_t)); 
  if (line == NULL) {
    TracePrintf(0, "tty_receive_line: malloc failed\n"); 
    // TODO: figure out what to do.
    Halt(); 
  }

  line->data = kbuf;
  line->len = len;
  line->offset = 0; 
  line->next = NULL; 

  if (terminals[tty_id].input_tail == NULL) {
    terminals[tty_id].input_tail = terminals[tty_id].input_head = line;
  } else {
    terminals[tty_id].input_tail->next = line;
    terminals[tty_id].input_tail = line; 
  }

  if (terminals[tty_id].read_waiters != NULL) {
    pcb_t *waiter = terminals[tty_id].read_waiters;
    terminals[tty_id].read_waiters = waiter->next;
    waiter->next = NULL;
    waiter->state = RUNNABLE;
    waiter->waiting_on = WAIT_NONE;
    ready_enqueue(waiter); 
  }
}


void tty_transmit_complete(int tty_id) {
  /**
   * Called from TRAP_TTY_TRANSMIT handler
   */

  write_req_t *write_request = terminals[tty_id].active_write; 
  if (write_request == NULL) return; 

  if (write_request->sent < write_request->total_len) {
    send_next_chunk(tty_id);
    return; 
  }

  pcb_t *writer = write_request->writer;
  free(write_request->kbuf);
  free(write_request);
  terminals[tty_id].active_write = NULL;

  writer->state = RUNNABLE;
  writer->waiting_on = WAIT_NONE;
  ready_enqueue(writer); 

  if (terminals[tty_id].write_queue != NULL) {
    write_req_t *next = terminals[tty_id].write_queue;
    terminals[tty_id].write_queue = next->next;
    next->next = NULL;
    terminals[tty_id].active_write = next;
    send_next_chunk(tty_id); 
  }
}
