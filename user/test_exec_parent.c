#include <yuser.h>

int main(int argc, char **argv) {
  TracePrintf(0, "parent before exec\n");
  char *args[] = {"user/test_exec_child", NULL};
  Exec("user/test_exec_child", args);
  TracePrintf(0, "SHOULD NOT REACH\n");
  return 0;
}