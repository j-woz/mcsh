
#include <stdio.h>

void
new_int(int i)
{
  printf("int: %i\n", i);
}

void
new_str(char* s)
{
  printf("str: %s\n", s);
}

#define new_g(V) _Generic((V),                 \
  int :  new_int,                              \
  char * : new_str                             \
  )(V)

int
main()
{
  new_g(3);
  return 0;
}
