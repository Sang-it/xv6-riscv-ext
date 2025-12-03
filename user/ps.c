#include "kernel/types.h"
#include "user/user.h"

// Process info structure (must match kernel/proc.h struct pinfo)
struct pinfo {
  int pid;
  int ppid;
  int state;
  uint64 sz;
  char name[16];
};

// State names matching enum procstate in kernel/proc.h
char *states[] = {
  "unused",
  "used",
  "sleep",
  "runble",
  "run",
  "zombie"
};

int
main(int argc, char *argv[])
{
  struct pinfo procs[64];  // NPROC = 64
  int nprocs;
  int i;

  nprocs = getprocs(procs, 64);
  if(nprocs < 0){
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  // Print header
  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");

  // Print each process
  for(i = 0; i < nprocs; i++){
    char *state;
    if(procs[i].state >= 0 && procs[i].state < 6)
      state = states[procs[i].state];
    else
      state = "???";
    
    printf("%d\t%d\t%s\t%d\t%s\n",
           procs[i].pid,
           procs[i].ppid,
           state,
           (int)procs[i].sz,
           procs[i].name);
  }

  exit(0);
}
