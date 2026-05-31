#include <yuser.h>

int main(int argc, char **argv) {
  char buf[3000];
  
  for (int i = 0; i < 2999; i++) {
    buf[i] = 'A' + (i % 26);
  }
  buf[2999] = '\n';
  
  int n = TtyWrite(0, buf, 3000);
  TracePrintf(0, "TtyWrite returned %d (expected 3000)\n", n);
  
  Exit(0);
  return 0;
}