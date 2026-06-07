#include <yuser.h>

int main(int argc, char **argv) {
  int lock_id;
  if (LockInit(&lock_id) == ERROR) {
    TracePrintf(0, "LockInit failed\n");
    Exit(1);
  }
  TracePrintf(0, "lock id %d\n", lock_id);
  
  int pid = Fork();
  if (pid == 0) {
    /* Child: try to acquire while parent holds. */
    Delay(2);
    TracePrintf(0, "child: trying to acquire\n");
    Acquire(lock_id);
    TracePrintf(0, "child: got the lock\n");
    Delay(2);
    TracePrintf(0, "child: releasing\n");
    Release(lock_id);
    Exit(0);
  }
  
  /* Parent: take lock, hold for a while. */
  Acquire(lock_id);
  TracePrintf(0, "parent: got lock, holding\n");
  Delay(5);
  TracePrintf(0, "parent: releasing\n");
  Release(lock_id);
  
  Wait(NULL);
  Reclaim(lock_id);
  Exit(0);
  return 0;
}