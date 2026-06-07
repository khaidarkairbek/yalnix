#include <yuser.h>

int main(int argc, char **argv) {
  int pipe_id;
  if (PipeInit(&pipe_id) == ERROR) {
    TracePrintf(0, "PipeInit failed\n");
    Exit(1);
  }
  TracePrintf(0, "got pipe id %d\n", pipe_id);
  
  int pid = Fork();
  if (pid == 0) {
    Delay(2);
    char *msg = "Hello from child via pipe!";
    int n = PipeWrite(pipe_id, msg, 27);
    TracePrintf(0, "child wrote %d bytes\n", n);
    Exit(0);
  }
  
  char buf[64];
  int n = PipeRead(pipe_id, buf, 64);
  buf[n] = '\0';
  TracePrintf(0, "parent read %d bytes: %s\n", n, buf);
  
  Wait(NULL);
  Reclaim(pipe_id);
  Exit(0);
  return 0;
}