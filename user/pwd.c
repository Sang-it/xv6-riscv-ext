#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char buf[128];
  
  if(getcwd(buf, sizeof(buf)) == 0) {
    fprintf(2, "pwd: getcwd failed\n");
    exit(1);
  }
  
  printf("%s\n", buf);
  exit(0);
}
