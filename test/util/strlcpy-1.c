
#include <stdio.h>
#include <string.h>

#include <strlcpyj.h>

#define n 32

int
main()
{
  char t[n] = "hello ";

  int r = strlcpyj(t, "bye", n);

  puts(t);

  printf("r=%i\n",r );

  return 0;
}
