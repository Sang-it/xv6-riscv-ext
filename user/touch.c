#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int fd;

  if (argc != 2) {
    fprintf(2, "usage: touch file\n");
    exit(1);
  }

  fd = open(argv[1], O_CREATE | O_RDWR);
  if (fd < 0) {
    fprintf(2, "touch: cannot create %s\n", argv[1]);
    exit(1);
  }
  close(fd);
  exit(0);
}
