#include <yuser.h>

int main(int argc, char **argv) {
  char *lines[] = {
    "line one\n",
    "line two\n",
    "line three\n",
    "line four\n",
    "line five\n",
  };
  
  for (int i = 0; i < 5; i++) {
    int len = 0;
    while (lines[i][len] != '\0') len++;
    int n = TtyWrite(0, lines[i], len);
    TracePrintf(0, "wrote line %d (returned %d)\n", i, n);
  }
  
  Exit(0);
  return 0;
}