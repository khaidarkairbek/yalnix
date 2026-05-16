#include "syscalls.h"
#include "pcb.h"

#include "ykernel.h"

int kernel_Nop (int a, int b, int c, int d) {
  return 0; 
}

int kernel_Fork (void) {
  // clone the current running process
  pcb_t *child_pcb;
  if (pcb_clone(g_current_process, &child_pcb) == ERROR) {
    return ERROR; 
  }

  child_pcb->uctx.regs[0] = 0;

  // enqueue the child to ready queue

  return child_pcb->pid; 
}

int kernel_Exec (char * filename, char ** argv) {
  // load program for the current running process
  // UNIMPLEMENTED
  return ERROR; 
}

void kernel_Exit (int status) {
  // terminate the current running process
  pcb_terminate(g_current_process, status);

  // free the resources used by the running process and context switch
  pcb_destroy(g_current_process); 

  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_Wait (int *status_ptr) {
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_GetPid (void) {
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_Brk (void *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_Delay (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_TtyRead (int, void *, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_TtyWrite (int, void *, int){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_ReadSector (int, void *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_WriteSector (int, void *){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_PipeInit (int *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_PipeRead (int, void *, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_PipeWrite (int, void *, int){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_SemInit (int *, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_SemUp (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_SemDown (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_LockInit (int *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_Acquire (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_Release (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarInit (int *){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarWait (int, int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarSignal (int){
  // UNIMPLEMENTED
  return ERROR; 
}
int kernel_CvarBroadcast (int){
  // UNIMPLEMENTED
  return ERROR; 
}

int kernel_Reclaim (int){
  // UNIMPLEMENTED
  return ERROR; 
}