#include <yuser.h>

void recurse(int n) {
  int big_array[256];
  big_array[0] = n;
  if (n < 100) recurse(n + 1);
}

int main(int argc, char **argv) {
  recurse(0);
  TracePrintf(0, "test_stack: survived recursion\n");
  Exit(0);
  return 0;
}