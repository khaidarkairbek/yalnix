#ifndef _yalnix_trap_handler_h
#define _yalnix_trap_handler_h

#include <hardware.h>

extern void handle_trap_kernel(UserContext *uctx); 
extern void handle_trap_clock(UserContext *uctx); 
extern void handle_trap_illegal(UserContext *uctx); 
extern void handle_trap_memory(UserContext *uctx); 
extern void handle_trap_math(UserContext *uctx); 
extern void handle_trap_tty_receive(UserContext *uctx); 
extern void handle_trap_tty_transmit(UserContext *uctx); 
extern void handle_trap_disk(UserContext *uctx); 

#endif