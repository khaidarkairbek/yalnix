#include <yuser.h>

int main(int argc, char **argv) {
  int pid = Fork();
  if (pid == 0) {
    TracePrintf(0, "child: pid=%d\n", GetPid());
  } else {
    TracePrintf(0, "parent: pid=%d, child=%d\n", GetPid(), pid);
  }
  while (1) Pause();
  return 0;
}