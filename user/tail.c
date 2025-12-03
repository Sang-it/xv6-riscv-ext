#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXLINE 1024
#define DEFAULT_LINES 10
#define MAX_LINES 64

char lines[MAX_LINES][MAXLINE];
int linelens[MAX_LINES];

void
tail(int fd, int nlines)
{
  char buf[512];
  char linebuf[MAXLINE];
  int n, i, idx;
  int count = 0;
  int linepos = 0;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      if (linepos < MAXLINE - 1)
        linebuf[linepos++] = buf[i];

      if (buf[i] == '\n') {
        idx = count % nlines;
        memmove(lines[idx], linebuf, linepos);
        linelens[idx] = linepos;
        count++;
        linepos = 0;
      }
    }
  }

  // Handle last line without newline
  if (linepos > 0) {
    idx = count % nlines;
    memmove(lines[idx], linebuf, linepos);
    linelens[idx] = linepos;
    count++;
  }

  // Print buffered lines in order
  int start = (count <= nlines) ? 0 : count - nlines;
  int toprint = (count < nlines) ? count : nlines;

  for (i = 0; i < toprint; i++) {
    idx = (start + i) % nlines;
    write(1, lines[idx], linelens[idx]);
  }
}

int
main(int argc, char *argv[])
{
  int fd, nlines = DEFAULT_LINES;
  int filearg = 1;

  if (argc > 2 && strcmp(argv[1], "-n") == 0) {
    nlines = atoi(argv[2]);
    if (nlines > MAX_LINES)
      nlines = MAX_LINES;
    filearg = 3;
  }

  if (filearg >= argc) {
    tail(0, nlines);
    exit(0);
  }

  if ((fd = open(argv[filearg], O_RDONLY)) < 0) {
    fprintf(2, "tail: cannot open %s\n", argv[filearg]);
    exit(1);
  }
  tail(fd, nlines);
  close(fd);
  exit(0);
}
