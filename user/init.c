#include <yuser.h>

int main(int argc, char **argv)
{
  int pid = GetPid();
  TracePrintf(0, "init: pid=%d argc=%d\n", pid, argc);
  
  while (1) {
    TracePrintf(1, "Init alive\n");
    Delay(5);
  }
  return 0; 
}