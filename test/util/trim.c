
#include <util.h>

int
main(int argc, char* argv[])
{
  for (int i = 0; i < argc; i++)
  {
    printf("'%s'\n", argv[i]);
    printf("'%s'\n", trim_right(argv[i]));
    printf("--\n");
  }
  return EXIT_SUCCESS;
}
