#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXLINE 1024

char buf[MAXLINE];

int
readline(int fd, char *buf, int max)
{
  int i = 0;
  char c;

  while (i < max - 1) {
    if (read(fd, &c, 1) != 1)
      break;
    buf[i++] = c;
    if (c == '\n')
      break;
  }
  buf[i] = 0;
  return i;
}

void
nl(int fd)
{
  int linenum = 1;
  int len;

  while ((len = readline(fd, buf, MAXLINE)) > 0) {
    printf("%d\t%s", linenum++, buf);
  }
}

int
main(int argc, char *argv[])
{
  int fd;

  if (argc == 1) {
    nl(0);  // stdin
    exit(0);
  }

  if (argc != 2) {
    fprintf(2, "usage: nl [file]\n");
    exit(1);
  }

  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    fprintf(2, "nl: cannot open %s\n", argv[1]);
    exit(1);
  }
  nl(fd);
  close(fd);
  exit(0);
}
