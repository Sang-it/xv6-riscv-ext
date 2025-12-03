#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BUFSIZE 512
#define DEFAULT_LINES 10

char buf[BUFSIZE];

void
head(int fd, int nlines)
{
  int n, i, linecount = 0;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      write(1, &buf[i], 1);
      if (buf[i] == '\n') {
        linecount++;
        if (linecount >= nlines)
          return;
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  int fd, nlines = DEFAULT_LINES;
  int filearg = 1;

  // Parse -n option
  if (argc > 2 && strcmp(argv[1], "-n") == 0) {
    nlines = atoi(argv[2]);
    filearg = 3;
  }

  if (filearg >= argc) {
    head(0, nlines);  // stdin
    exit(0);
  }

  if ((fd = open(argv[filearg], O_RDONLY)) < 0) {
    fprintf(2, "head: cannot open %s\n", argv[filearg]);
    exit(1);
  }
  head(fd, nlines);
  close(fd);
  exit(0);
}
