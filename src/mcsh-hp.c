
/**
   MCSH HP: Hypertext Processor
   Makes the program mchp
*/

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "mcsh.h"
#include "mcsh-preprocess.h"

int
main(int argc, char* argv[])
{
  setbuf(stdout, NULL);
  printf("MCSH HP\n");

  assert(argc > 0);
  assert(argv[0] != NULL);

  mcsh_init();
  mcsh_vm vm;
  mcsh_vm_init_argv(&vm, argc, argv);

  buffer code;
  buffer_init(&code, 1024);
  bool b = slurp_buffer(stdin, &code);
  assert(b);

  printf("read ok\n");

  mcsh_script_preprocess(code.data);
  printf("pp: %s\n--\n", code.data);

  mcsh_hp(&vm, code.data);

  buffer_finalize(&code);

  mcsh_finalize();
  return EXIT_SUCCESS;
}
