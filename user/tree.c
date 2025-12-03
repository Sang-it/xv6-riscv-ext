#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_DEPTH 10

int dir_count = 0;
int file_count = 0;

// Append src to end of dst
char*
strcat(char *dst, const char *src)
{
  char *p = dst;
  while (*p)
    p++;
  while ((*p++ = *src++) != 0)
    ;
  return dst;
}

void
tree(char *path, char *prefix, int depth)
{
  int fd, fd2;
  struct stat st;
  struct dirent de;
  char buf[512], new_prefix[256];
  char *p;
  int count, i;

  if (depth <= 0)
    return;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "tree: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "tree: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    // Not a directory, just count as file
    close(fd);
    return;
  }

  // First pass: count valid entries
  count = 0;
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;
    count++;
  }
  close(fd);

  // Second pass: print entries
  if ((fd2 = open(path, O_RDONLY)) < 0) {
    fprintf(2, "tree: cannot reopen %s\n", path);
    return;
  }

  i = 0;
  while (read(fd2, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;

    i++;
    int is_last = (i == count);

    // Print prefix and branch
    if (is_last)
      printf("%s\\-- %s", prefix, de.name);
    else
      printf("%s|-- %s", prefix, de.name);

    // Build full path
    if (strlen(path) + 1 + strlen(de.name) + 1 >= sizeof(buf)) {
      printf(" [path too long]\n");
      continue;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    // de.name may not be null-terminated if exactly DIRSIZ chars
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    // Stat to check if directory
    if (stat(buf, &st) < 0) {
      printf(" [cannot stat]\n");
      continue;
    }

    if (st.type == T_DIR) {
      dir_count++;
      printf("\n");
      // Recurse with updated prefix
      strcpy(new_prefix, prefix);
      if (is_last)
        strcat(new_prefix, "    ");
      else
        strcat(new_prefix, "|   ");
      tree(buf, new_prefix, depth - 1);
    } else {
      file_count++;
      printf("\n");
    }
  }

  close(fd2);
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  struct stat st;

  if (argc > 1) {
    path = argv[1];
  }

  // Check if path exists and is valid
  if (stat(path, &st) < 0) {
    fprintf(2, "tree: cannot stat %s\n", path);
    exit(1);
  }

  printf("%s\n", path);

  if (st.type == T_DIR) {
    tree(path, "", MAX_DEPTH);
  } else {
    file_count = 1;
  }

  printf("\n%d directories, %d files\n", dir_count, file_count);

  exit(0);
}
