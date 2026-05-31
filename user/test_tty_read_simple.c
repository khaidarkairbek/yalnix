#include <yuser.h>

int main(int argc, char **argv) {
  char buf[256];
  
  char *prompt = "type a line: ";
  int plen = 0;
  while (prompt[plen] != '\0') plen++;
  TtyWrite(0, prompt, plen);
  
  int n = TtyRead(0, buf, 256);
  TracePrintf(0, "TtyRead returned %d\n", n);
  
  TtyWrite(0, "you typed: ", 11);
  TtyWrite(0, buf, n);
  TtyWrite(0, "\n", 1);
  
  Exit(0);
  return 0;
}