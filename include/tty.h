#ifndef _yalnix_tty_h
#define _yalnix_tty_h

#include "kernel.h"


void tty_init(void); 
int tty_has_input(int tty_id);
int tty_consume_input(int tty_id, void *buf, int len); 
void tty_add_read_waiter(pcb_t *p, int tty_id);

void tty_write_start(int tty_id, void *kbuf, int len, pcb_t *writer);

void tty_receive_line(int tty_id, void *kbuf, int len);
void tty_transmit_complete(int tty_id);


#endif
