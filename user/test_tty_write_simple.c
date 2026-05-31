#include <yuser.h>

int main(int argc, char **argv) {
  char *msg = "hello from TtyWrite\n";
  int len = 0;
  while (msg[len] != '\0') len++;
  
  int n = TtyWrite(0, msg, len);
  TracePrintf(0, "TtyWrite returned %d (expected %d)\n", n, len);
  
  Exit(0);
  return 0;
}