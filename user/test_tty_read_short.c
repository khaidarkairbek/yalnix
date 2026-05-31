#include <yuser.h>

int main(int argc, char **argv) {
  char buf[8];   /* deliberately small */
  
  TtyWrite(0, "type (more than 8 chars):\n", 42);
  
  int n1 = TtyRead(0, buf, 8);
  TracePrintf(0, "first TtyRead: %d bytes\n", n1);
  TtyWrite(0, "first chunk: ", 13);
  TtyWrite(0, buf, n1);
  TtyWrite(0, "\n", 1);
  
  /* The rest of the line should still be buffered. */
  int n2 = TtyRead(0, buf, 8);
  TracePrintf(0, "second TtyRead: %d bytes\n", n2);
  TtyWrite(0, "second chunk: ", 14);
  TtyWrite(0, buf, n2);
  TtyWrite(0, "\n", 1);
  
  Exit(0);
  return 0;
}