// cp - copy files

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BUFSIZE 512

int
main(int argc, char *argv[])
{
  int src, dst, n;
  char buf[BUFSIZE];

  if (argc != 3) {
    fprintf(2, "usage: cp source dest\n");
    exit(1);
  }

  // Open source file
  if ((src = open(argv[1], O_RDONLY)) < 0) {
    fprintf(2, "cp: cannot open %s\n", argv[1]);
    exit(1);
  }

  // Create destination file
  if ((dst = open(argv[2], O_WRONLY | O_CREATE | O_TRUNC)) < 0) {
    fprintf(2, "cp: cannot create %s\n", argv[2]);
    close(src);
    exit(1);
  }

  // Copy data
  while ((n = read(src, buf, BUFSIZE)) > 0) {
    if (write(dst, buf, n) != n) {
      fprintf(2, "cp: write error\n");
      close(src);
      close(dst);
      exit(1);
    }
  }

  if (n < 0) {
    fprintf(2, "cp: read error\n");
    close(src);
    close(dst);
    exit(1);
  }

  close(src);
  close(dst);
  exit(0);
}
