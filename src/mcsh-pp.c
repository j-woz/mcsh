
/*
  MCSH PP
*/

#include <assert.h>

#include "buffer.h"
#include "util.h"
#include "mcsh-preprocess.h"

int
main(int argc, char* argv[])
{
  buffer code;
  buffer_init(&code, 1024);
  bool b = slurp_buffer(stdin, &code);
  assert(b);

  // printf("[%s]\n", code.data);
  mcsh_script_preprocess(code.data);
  // printf("\nafter:\n");
  // printf("[%s]\n", code.data);
  printf("%s", code.data);

  return EXIT_SUCCESS;
}
