#include <yuser.h>

int main(int argc, char **argv) {
  int pid = Fork();
  if (pid == 0) {
    Delay(2);
    Exit(99);
  } else {
    int status;
    int child_pid = Wait(&status);
    TracePrintf(0, "parent: child %d exited with %d\n", child_pid, status);
    Exit(0);
  }
  return 0;
}