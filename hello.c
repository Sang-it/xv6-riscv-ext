// Simple test for c4 interpreter
enum { MAGIC = 42 };

int fact(int n) {
  if (n <= 1)
    return 1;
  return n * fact(n - 1);
}

int main() {
  int x, *p;
  char *s;
  printf("Hello from c4!\n");
  // Arithmetic & bitwise
  x = 10;
  printf("%d + 3 = %d\n", x, x + 3);
  printf("1 << 4 = %d\n", 1 << 4);
  // Pointers
  p = &x;
  *p = 99;
  printf("x via pointer = %d\n", x);
  // Ternary, comparison
  printf("max(5,9) = %d\n", (5 > 9) ? 5 : 9);
  // Enum, sizeof
  printf("MAGIC = %d, sizeof(int) = %d\n", MAGIC, sizeof(int));
  // Recursion
  printf("5! = %d\n", fact(5));
  // malloc/free
  s = malloc(8);
  memset(s, 'A', 4);
  s[4] = 0;
  printf("string: %s\n", s);
  free(s);
  return 0;
}
