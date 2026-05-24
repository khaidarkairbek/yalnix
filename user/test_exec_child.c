#include <yuser.h>

int main(int argc, char **argv) {
  TracePrintf(0, "child after exec\n");
  Exit(0);
  return 0;
}