#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TICKS_PER_SEC 10  // xv6 default: 10 ticks/second

int
main(int argc, char *argv[])
{
  int pid, status;
  int start, end, elapsed;
  int secs, tenths;

  if (argc < 2) {
    fprintf(2, "usage: time command [args...]\n");
    exit(1);
  }

  start = uptime();

  pid = fork();
  if (pid < 0) {
    fprintf(2, "time: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child: execute the command
    exec(argv[1], &argv[1]);
    // If exec fails, try with / prefix
    if (argv[1][0] != '/') {
      char buf[100];
      buf[0] = '/';
      strcpy(buf + 1, argv[1]);
      exec(buf, &argv[1]);
    }
    fprintf(2, "time: exec %s failed\n", argv[1]);
    exit(127);
  }

  // Parent: wait for child
  wait(&status);

  end = uptime();

  elapsed = end - start;
  secs = elapsed / TICKS_PER_SEC;
  tenths = (elapsed % TICKS_PER_SEC) * 10 / TICKS_PER_SEC;

  printf("\nreal    %d.%ds\n", secs, tenths);

  exit(0);
}
