#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BUFSIZE 512

char buf[BUFSIZE];

int
main(int argc, char *argv[])
{
  int fd, n;

  if (argc != 2) {
    fprintf(2, "usage: tee file\n");
    exit(1);
  }

  fd = open(argv[1], O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0) {
    fprintf(2, "tee: cannot open %s\n", argv[1]);
    exit(1);
  }

  while ((n = read(0, buf, sizeof(buf))) > 0) {
    write(1, buf, n);
    write(fd, buf, n);
  }

  close(fd);
  exit(0);
}
