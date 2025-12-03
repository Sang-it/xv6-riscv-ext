#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // ANSI escape codes:
  // \033[2J - clear entire screen
  // \033[H  - move cursor to home position (top-left)
  printf("\033[2J\033[H");
  exit(0);
}
