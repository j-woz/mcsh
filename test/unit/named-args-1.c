
/**
   function f { x y }
   f $x $y
*/

#include "mcsh.h"

int
main()
{
  mcsh_signature sg;
  char*       names[2] = { "x",  "y"  };
  mcsh_value* dflts[2] = { NULL, NULL };
  mcsh_signature_init(&sg, 2, names, dflts, false);

  mcsh_arg x, y;

  // List of mcsh_arg:
  list_array A;
  list_array_init(&A, 8);
  list_array_add (&A, &x);
  list_array_add (&A, &y);

  mcsh_value v_x, v_y;
  mcsh_value_init_int(&v_x, 42);
  mcsh_value_init_int(&v_y, 43);
  // These are stack-allocated: must hold them:
  mcsh_value_grab(NULL, &v_x);
  mcsh_value_grab(NULL, &v_y);
  mcsh_arg_init(&x, NULL, &v_x);
  mcsh_arg_init(&y, NULL, &v_y);

  mcsh_parameters P;
  mcsh_parameterize(&sg, &A, &P);

  mcsh_parameters_print(&P);

  list_array_finalize(&A);
  mcsh_parameters_finalize(&P);
  mcsh_signature_finalize(&sg);

  return 0;
}
