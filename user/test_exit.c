#include <yuser.h>

int main(int argc, char **argv) {
  TracePrintf(0, "test_exit: about to call Exit\n");
  Exit(42);
  TracePrintf(0, "test_exit: should not reach\n");
  return 0; 
}