// mv - move/rename files

#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(2, "usage: mv source dest\n");
    exit(1);
  }

  // First, try to unlink dest if it exists (overwrite behavior)
  unlink(argv[2]);

  // Create a new link (new name pointing to same inode)
  if (link(argv[1], argv[2]) < 0) {
    fprintf(2, "mv: cannot move %s to %s\n", argv[1], argv[2]);
    exit(1);
  }

  // Remove the old link (old name)
  if (unlink(argv[1]) < 0) {
    fprintf(2, "mv: cannot remove %s\n", argv[1]);
    // Try to undo the link we just created
    unlink(argv[2]);
    exit(1);
  }

  exit(0);
}
