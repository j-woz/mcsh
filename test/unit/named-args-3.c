
/**
   function f { x y=1000 z }
   f $x z=$z y=$y
   ==
   f 42 z=44 y=43
*/

#include "mcsh.h"

int
main()
{
  mcsh_signature sg;
  #define count 3
  mcsh_value v_y_dflt;
  mcsh_value_init_int(&v_y_dflt, 1000);
  // This is stack-allocated: must hold it:
  mcsh_value_grab(NULL, &v_y_dflt);

  char*       names[count] = { "x",  "y", "z" };
  mcsh_value* dflts[count] = { NULL, &v_y_dflt, NULL };
  mcsh_signature_init(&sg, count, names, dflts, false);

  mcsh_arg     x,   y,   z;
  mcsh_value v_x, v_y, v_z;
  mcsh_value name_y, name_z;
  mcsh_value_init_string(&name_y, "y");
  mcsh_value_init_string(&name_z, "z");
  mcsh_value_init_int(&v_x, 42);
  mcsh_value_init_int(&v_y, 43);
  mcsh_value_init_int(&v_z, 44);
  // These are stack-allocated: must hold them:
  mcsh_value_grab(NULL, &v_x);
  mcsh_value_grab(NULL, &v_y);
  mcsh_value_grab(NULL, &v_z);

  mcsh_arg_init(&x, NULL,    &v_x);
  mcsh_arg_init(&y, &name_y, &v_y);
  mcsh_arg_init(&z, &name_z, &v_z);

  // List of mcsh_arg:
  list_array A;
  list_array_init(&A, 8);
  list_array_add (&A, &x);
  list_array_add (&A, &z);
  list_array_add (&A, &y);

  mcsh_parameters P;
  mcsh_parameterize(&sg, &A, &P);

  mcsh_parameters_print(&P);

  list_array_finalize(&A);
  mcsh_parameters_finalize(&P);
  mcsh_signature_finalize(&sg);

  return 0;
}
