#include <yuser.h>

int main(int argc, char **argv) {
  int a = 5, b = 0;
  int c = a / b; 
  TracePrintf(0, "SHOULD NOT REACH: c=%d\n", c);
  return 0;
}