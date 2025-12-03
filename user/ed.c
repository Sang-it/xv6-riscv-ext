// ed - line editor for xv6
// A minimal implementation of the classic Unix ed editor

#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXLINES 1000
#define MAXLINE  512

// Line buffer
char *lines[MAXLINES];
int nlines = 0;
int curline = 0;

// State
char filename[128];
int dirty = 0;

// Input buffer
char buf[MAXLINE];

// Duplicate a string
char*
strdup(const char *s)
{
  int len = strlen(s);
  char *p = malloc(len + 1);
  if (p)
    strcpy(p, s);
  return p;
}

// Read a line from fd into buf, return length (including \n) or 0 on EOF
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
  buf[i] = '\0';
  return i;
}

// Free all lines in buffer
void
clear_buffer(void)
{
  for (int i = 0; i < nlines; i++) {
    if (lines[i])
      free(lines[i]);
    lines[i] = 0;
  }
  nlines = 0;
  curline = 0;
}

// Insert a line at position pos (1-indexed, 0 means before first)
// Shifts existing lines down
int
insert_line(int pos, const char *text)
{
  if (nlines >= MAXLINES) {
    return -1;
  }
  if (pos < 0 || pos > nlines) {
    return -1;
  }
  
  // Shift lines down
  for (int i = nlines; i > pos; i--) {
    lines[i] = lines[i - 1];
  }
  
  lines[pos] = strdup(text);
  if (!lines[pos])
    return -1;
  
  nlines++;
  return 0;
}

// Delete lines from start to end (1-indexed, inclusive)
int
delete_lines(int start, int end)
{
  if (start < 1 || end > nlines || start > end)
    return -1;
  
  // Free the lines
  for (int i = start - 1; i < end; i++) {
    if (lines[i])
      free(lines[i]);
  }
  
  // Shift remaining lines up
  int count = end - start + 1;
  for (int i = end; i < nlines; i++) {
    lines[i - count] = lines[i];
  }
  
  nlines -= count;
  
  // Adjust current line
  if (curline > nlines)
    curline = nlines;
  if (curline < 1 && nlines > 0)
    curline = 1;
  
  return 0;
}

// Load a file into the buffer
int
load_file(const char *fname)
{
  int fd = open(fname, O_RDONLY);
  if (fd < 0)
    return -1;
  
  clear_buffer();
  
  int total = 0;
  while (nlines < MAXLINES) {
    int n = readline(fd, buf, MAXLINE);
    if (n <= 0)
      break;
    total += n;
    if (insert_line(nlines, buf) < 0)
      break;
  }
  
  close(fd);
  curline = nlines;
  dirty = 0;
  
  return total;
}

// Save buffer to file
int
save_file(const char *fname)
{
  int fd = open(fname, O_WRONLY | O_CREATE | O_TRUNC);
  if (fd < 0)
    return -1;
  
  int total = 0;
  for (int i = 0; i < nlines; i++) {
    int len = strlen(lines[i]);
    if (write(fd, lines[i], len) != len) {
      close(fd);
      return -1;
    }
    total += len;
  }
  
  close(fd);
  dirty = 0;
  
  return total;
}

// Print error (classic ed just prints ?)
void
error(void)
{
  printf("?\n");
}

// Parse a single address, return line number (1-indexed) or -1 on error
// Updates *p to point past the address
int
parse_addr(char **p, int def)
{
  char *s = *p;
  int addr = def;
  
  // Skip whitespace
  while (*s == ' ' || *s == '\t')
    s++;
  
  if (*s == '.') {
    addr = curline;
    s++;
  } else if (*s == '$') {
    addr = nlines;
    s++;
  } else if (*s == '+') {
    s++;
    int n = 0;
    while (*s >= '0' && *s <= '9')
      n = n * 10 + (*s++ - '0');
    if (n == 0) n = 1;
    addr = curline + n;
  } else if (*s == '-') {
    s++;
    int n = 0;
    while (*s >= '0' && *s <= '9')
      n = n * 10 + (*s++ - '0');
    if (n == 0) n = 1;
    addr = curline - n;
  } else if (*s >= '0' && *s <= '9') {
    addr = 0;
    while (*s >= '0' && *s <= '9')
      addr = addr * 10 + (*s++ - '0');
  }
  
  *p = s;
  return addr;
}

// Parse a range (addr1,addr2), return 0 on success
// If no range given, uses defaults
int
parse_range(char **p, int *start, int *end, int def_start, int def_end)
{
  char *s = *p;
  
  // Skip whitespace
  while (*s == ' ' || *s == '\t')
    s++;
  
  // Check for , alone (means 1,$)
  if (*s == ',') {
    *start = 1;
    s++;
    // Check if there's an end address
    char *tmp = s;
    *end = parse_addr(&s, nlines);
    if (s == tmp)
      *end = nlines;
    *p = s;
    return 0;
  }
  
  // Try to parse first address
  char *orig = s;
  int addr1 = parse_addr(&s, def_start);
  
  if (s == orig) {
    // No address given, use defaults
    *start = def_start;
    *end = def_end;
    *p = s;
    return 0;
  }
  
  // Skip whitespace
  while (*s == ' ' || *s == '\t')
    s++;
  
  // Check for comma (range)
  if (*s == ',') {
    s++;
    int addr2 = parse_addr(&s, nlines);
    *start = addr1;
    *end = addr2;
    *p = s;
    return 0;
  }
  
  // Single address
  *start = addr1;
  *end = addr1;
  *p = s;
  return 0;
}

// Enter input mode, inserting after line 'after' (0 = before first)
void
input_mode(int after)
{
  while (1) {
    if (gets(buf, MAXLINE) == 0)
      break;
    
    // Check for single '.' to exit input mode
    if (buf[0] == '.' && (buf[1] == '\n' || buf[1] == '\0'))
      break;
    
    // Ensure line ends with newline
    int len = strlen(buf);
    if (len > 0 && buf[len-1] != '\n') {
      if (len < MAXLINE - 1) {
        buf[len] = '\n';
        buf[len+1] = '\0';
      }
    }
    
    if (insert_line(after, buf) < 0) {
      error();
      break;
    }
    after++;
    curline = after;
    dirty = 1;
  }
}

// Command: append after addressed line
void
cmd_append(int addr)
{
  if (addr < 0 || addr > nlines) {
    error();
    return;
  }
  input_mode(addr);
}

// Command: insert before addressed line
void
cmd_insert(int addr)
{
  if (addr < 1) addr = 1;
  if (nlines == 0) {
    input_mode(0);
  } else {
    if (addr > nlines) {
      error();
      return;
    }
    input_mode(addr - 1);
  }
}

// Command: change (delete then insert)
void
cmd_change(int start, int end)
{
  if (nlines == 0 || start < 1 || end > nlines || start > end) {
    error();
    return;
  }
  
  int insert_pos = start - 1;
  if (delete_lines(start, end) < 0) {
    error();
    return;
  }
  dirty = 1;
  input_mode(insert_pos);
}

// Command: delete lines
void
cmd_delete(int start, int end)
{
  if (nlines == 0 || start < 1 || end > nlines || start > end) {
    error();
    return;
  }
  
  if (delete_lines(start, end) < 0) {
    error();
    return;
  }
  dirty = 1;
  
  // Set current line
  if (start <= nlines)
    curline = start;
  else
    curline = nlines;
}

// Command: print lines
void
cmd_print(int start, int end)
{
  if (nlines == 0 || start < 1 || end > nlines || start > end) {
    error();
    return;
  }
  
  for (int i = start; i <= end; i++) {
    printf("%s", lines[i - 1]);
  }
  curline = end;
}

// Command: print lines with line numbers
void
cmd_number(int start, int end)
{
  if (nlines == 0 || start < 1 || end > nlines || start > end) {
    error();
    return;
  }
  
  for (int i = start; i <= end; i++) {
    printf("%d\t%s", i, lines[i - 1]);
  }
  curline = end;
}

// Command: edit file
void
cmd_edit(char *arg)
{
  // Skip whitespace
  while (*arg == ' ' || *arg == '\t')
    arg++;
  
  // Remove trailing newline
  int len = strlen(arg);
  if (len > 0 && arg[len-1] == '\n')
    arg[len-1] = '\0';
  
  // If arg given, use it as filename
  if (*arg) {
    strcpy(filename, arg);
  }
  
  if (filename[0] == '\0') {
    error();
    return;
  }
  
  int bytes = load_file(filename);
  if (bytes < 0) {
    error();
    return;
  }
  
  printf("%d\n", bytes);
}

// Command: write file
void
cmd_write(char *arg)
{
  // Skip whitespace
  while (*arg == ' ' || *arg == '\t')
    arg++;
  
  // Remove trailing newline
  int len = strlen(arg);
  if (len > 0 && arg[len-1] == '\n')
    arg[len-1] = '\0';
  
  // If arg given, use it as filename
  char *fname = filename;
  if (*arg) {
    fname = arg;
  }
  
  if (fname[0] == '\0') {
    error();
    return;
  }
  
  int bytes = save_file(fname);
  if (bytes < 0) {
    error();
    return;
  }
  
  // Update default filename if writing to new file
  if (*arg && filename[0] == '\0') {
    strcpy(filename, arg);
  }
  
  printf("%d\n", bytes);
}

// Command: print line number
void
cmd_equal(int addr)
{
  printf("%d\n", addr);
}

// Command: set/show filename
void
cmd_filename(char *arg)
{
  // Skip whitespace
  while (*arg == ' ' || *arg == '\t')
    arg++;
  
  // Remove trailing newline
  int len = strlen(arg);
  if (len > 0 && arg[len-1] == '\n')
    arg[len-1] = '\0';
  
  if (*arg) {
    strcpy(filename, arg);
  }
  
  if (filename[0])
    printf("%s\n", filename);
  else
    error();
}

// Main command loop
int
main(int argc, char *argv[])
{
  filename[0] = '\0';
  
  // Optional filename argument
  if (argc > 1) {
    strcpy(filename, argv[1]);
    int bytes = load_file(filename);
    if (bytes >= 0) {
      printf("%d\n", bytes);
    } else {
      // File doesn't exist yet, that's ok for new files
      filename[0] = '\0';
      strcpy(filename, argv[1]);
    }
  }
  
  // Command loop
  while (1) {
    if (gets(buf, MAXLINE) == 0)
      break;
    
    char *p = buf;
    
    // Skip whitespace
    while (*p == ' ' || *p == '\t')
      p++;
    
    // Empty line: print next line
    if (*p == '\n' || *p == '\0') {
      if (curline < nlines) {
        curline++;
        printf("%s", lines[curline - 1]);
      } else {
        error();
      }
      continue;
    }
    
    // Parse address/range
    int start, end;
    parse_range(&p, &start, &end, curline, curline);
    
    // Skip whitespace
    while (*p == ' ' || *p == '\t')
      p++;
    
    // Get command character
    char cmd = *p++;
    
    switch (cmd) {
    case 'a':
      cmd_append(end);
      break;
      
    case 'i':
      cmd_insert(start);
      break;
      
    case 'c':
      cmd_change(start, end);
      break;
      
    case 'd':
      cmd_delete(start, end);
      break;
      
    case 'p':
      cmd_print(start, end);
      break;
      
    case 'n':
      cmd_number(start, end);
      break;
      
    case 'e':
      if (dirty) {
        printf("?\n");
        dirty = 0;  // Warn once
      } else {
        cmd_edit(p);
      }
      break;
      
    case 'E':
      cmd_edit(p);
      break;
      
    case 'w':
      // Check for 'wq'
      if (*p == 'q') {
        cmd_write(p + 1);
        if (!dirty)
          exit(0);
      } else {
        cmd_write(p);
      }
      break;
      
    case 'W':
      cmd_write(p);
      break;
      
    case 'q':
      if (dirty) {
        printf("?\n");
        dirty = 0;  // Warn once
      } else {
        exit(0);
      }
      break;
      
    case 'Q':
      exit(0);
      break;
      
    case 'f':
      cmd_filename(p);
      break;
      
    case '=':
      cmd_equal(end);
      break;
      
    case 'h':
    case 'H':
      // Help - print brief usage
      printf("Commands: a i c d p n e w q Q f =\n");
      printf("Address: . $ n n,m ,\n");
      break;
      
    default:
      error();
      break;
    }
  }
  
  exit(0);
}
