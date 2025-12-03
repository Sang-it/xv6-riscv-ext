#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int ticks = uptime();
  printf("uptime: %d ticks\n", ticks);
  exit(0);
}
