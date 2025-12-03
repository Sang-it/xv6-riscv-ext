#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXLINE 1024

char buf[MAXLINE];

// Reverse a string in place (up to len characters)
void
reverse(char *s, int len)
{
  int i, j;
  char tmp;

  for (i = 0, j = len - 1; i < j; i++, j--) {
    tmp = s[i];
    s[i] = s[j];
    s[j] = tmp;
  }
}

// Read lines from fd and print them reversed
void
rev(int fd)
{
  int n, i, start;

  start = 0;
  while ((n = read(fd, buf + start, sizeof(buf) - start - 1)) > 0) {
    n += start;
    for (i = 0; i < n; i++) {
      if (buf[i] == '\n') {
        // Found end of line, reverse and print
        reverse(buf, i);
        buf[i] = '\n';
        write(1, buf, i + 1);
        // Shift remaining data to beginning
        memmove(buf, buf + i + 1, n - i - 1);
        n = n - i - 1;
        i = -1;  // Will become 0 after i++
      }
    }
    start = n;  // Keep partial line for next read
  }

  // Handle last line without newline
  if (start > 0) {
    reverse(buf, start);
    write(1, buf, start);
    write(1, "\n", 1);
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if (argc <= 1) {
    rev(0);  // Read from stdin
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "rev: cannot open %s\n", argv[i]);
      exit(1);
    }
    rev(fd);
    close(fd);
  }
  exit(0);
}
