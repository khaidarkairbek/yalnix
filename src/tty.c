#include "tty.h"
#include "kernel.h"

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
   *
   * Pseudocode:
   *  line = terminals[tty_id].input_head
   *  if line == NULL: return 0
   *
   *  available = line->len - line->offset
   *  to_copy = min(available, len)
   *  memcpy(buf, line->data + line->offset, to_copy)
   *  line->offset += to_copy
   *
   *  if line->offset >= line->len:
   *    terminals[tty_id].input_head = line->next;
   *    if terminals[tty_id].input_head == NULL:
   *      terminals[tty_id].input_tail = NULL
   *    free(line->data);
   *    free(line);
   *
   *  return to_copy; 
   */
}

void tty_add_read_waiter(pcb_t *p, int tty_id) {
  /**
   * Adds the process to the tty_id reader queue.
   * Pseudocode:
   * p->next = NULL;
   * if terminals[tty_id].read_waiters == NULL:
   *    terminals[tty_id].read_waiters = p;
   * else:
   *    waiter = terminals[tty_id].read_waiters
   *    while waiter.next != NULL:
   *      waiter = waiter.next
   *
   *    waiter.next = p; 
   */
}

static void send_next_chunk(int tty_id) {
  /**
   * Pseudocode: 
   * write_request = terminals[tty_id].active_write
   * remaining = write_request->total_len - write_request->sent
   * to_send = min(remaining, TERMINAL_MAX_LINE)
   * 
   * TtyTransmit(tty_id, write_request->kbuf + write_request->sent, to_send)
   * write_request->sent += to_send
   */
}

void tty_write_start(int tty_id, void *kbuf, int len, pcb_t *writer) {
  /**
   * Begin a write of len bytes from kbuf to tty_id on behalf of writer.
   *
   * If the terminal is idle, start the TtyTransmit for the first chunk. Otherwise, queue the writers
   * to wait until terminal is idle.
   *
   * Pseudocode:
   * write_request = malloc(sizeof(write_req_t))
   * if write_request == NULL:
   *  free(kbuf);
   *  // TODO: figure out what to do.
   * else
   *    write_request->writer = writer
   *    write_request->kbuf = kbuf
   *    write_request->total_lem = len
   *    write_request->sent = 0
   *    write_request->next = NULL
   *  
   *    if terrminals[tty_id].active_write == NULL: 
   *       terrminals[tty_id].active_write = write_request
   *       send_next_chunk(tty_id)
   *    else
   *       if terrminals[tty_id].write_queue == NULL: 
   *        terrminals[tty_id].write_queue = write_request
   *       else: 
   *        write = terminals[tty_id].write_queue
   *        while write.next != NULL: 
   *          write = write.next
   *        write.next = write_request
   */
}

void tty_receive_line(int tty_id, void *kbuf, int len) {
  /**
   * Called from TRAP_TTY_RECEIVE handler
   *
   * Pseudocode:
   * line = malloc(sizeof(input_line_t))
   * if line == NULL:
   *  free(kbuf)
   *  return
   *
   * line->data = kbuf
   * line->len = len
   * line->offset = 0
   * line->next = NULL
   *
   * if terminals[tty_id].input_tail == NULL:
   *  terminals[tty_id].input_head = line
   *  terminals[tty_id].input_tail = line
   * else
   *  terminals[tty_id].input_tail->next = line; 
   *  terminals[tty_id].input_tail = line
   * 
   * if terminals[tty_id].read_waiters != NULL: 
   *    waiter = terminals[tty_id].read_waiters
   *    terminals[tty_id].read_waiters = terminals[tty_id].read_waiters->next
   *    waiter->next = NULL
   *    waiter->state = RUNNABLE
   *    waiter->waiting_on = NONE
   *    ready_enqueue(waiter)
   */
}
void tty_transmit_complete(int tty_id) {
  /**
   * Called from TRAP_TTY_TRANSMIT handler
   *
   * Pseudocode:
   * write_request = terminals[tty_id].active_write
   * if write_request == NULL:
   *    return
   *
   * if write_request->sent < write_request->total_len:
   *    send_next_chunk(tty_id)
   *    return
   *
   * writer = write_request->writer;
   * free(write_request->kbuf)
   * free(write_request)
   * terminals[tty_id].active_write = NULL
   *
   * writer->state = RUNNABLE
   * writer->waiting_on = NONE
   * ready_enqueue(writer); 
   * 
   * if terminals[tty_id].write_queue != NULL: 
   *  next = terminals[tty_id].write_queue
   *  terminals[tty_id].write_queue = next->next
   *  next->next = NULL
   *  terminals[tty_id].active_write = next
   *  send_next_chunk(tty_id)
   */
}
