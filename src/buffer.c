
#include <assert.h>

#include "buffer.h"

void
buffer_show(buffer* B)
{
  printf("buffer: %p\n", B);
  printf("length: %zi\n", B->length);
  printf("capcty: %zi\n", B->capacity);
  printf("\n");
  for (size_t i = 0; i < B->length; i++)
  {
    switch (B->data[i])
    {
      case '\0':
        printf("%4zi: 00", i);
        break;
      case ' ':
        printf("%4zi: SP", i);
        break;
      default:
        printf("%4zi:%2c", i, B->data[i]);
    }
    if ((i+1) % 10 == 0) printf("\n");
  }
  printf("\n");
}

bool
slurp_buffer(FILE* stream, buffer* B)
{
  int max_line = 1024;
  char line[max_line];
  // In case the file is empty:
  B->data[0] = '\0';
  while (true)
  {
    char* t = fgets(line, max_line, stream);
    if (t == NULL)
    {
      if (feof(stream))
        return true;
      else
        return false;
    }
    buffer_cat(B, t);
  }
  // unreachable- should hit EOF first...
  assert(false);
  return false;
}
