#include <hardware.h>

int SetKernelBrk(void *addr) {
  // UNIMPLEMENTED
  return 0; 
}

extern void KernelStart (char**, unsigned int, UserContext *) {
  Halt(); 
}