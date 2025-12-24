
/**
   function f { x y=1000 z ... }
   f $x z=$z y=$y q
*/

#include "mcsh.h"

int
main()
{
  mcsh_signature sg;
  #define count 3
  mcsh_value v_y_dflt;
  mcsh_value_init_int(&v_y_dflt, 1000);
  char*       names[count] = { "x",  "y", "z" };
  mcsh_value* dflts[count] = { NULL, &v_y_dflt, NULL };
  mcsh_signature_init(&sg, count, names, dflts, true);

  mcsh_arg     x,   y,   z,   q;
  mcsh_value v_x, v_y, v_z, v_q;
  mcsh_value name_y, name_z;
  mcsh_value_init_string(&name_y, "y");
  mcsh_value_init_string(&name_z, "z");
  mcsh_value_init_int(&v_x, 42);
  mcsh_value_init_int(&v_y, 43);
  mcsh_value_init_int(&v_z, 44);
  mcsh_value_init_int(&v_q, 45);
  mcsh_arg_init(&x, NULL,    &v_x);
  mcsh_arg_init(&y, &name_y, &v_y);
  mcsh_arg_init(&z, &name_z, &v_z);
  mcsh_arg_init(&q, NULL,    &v_q);

  // List of mcsh_arg:
  list_array L;
  list_array_init(&L, 8);
  list_array_add (&L, &x);
  list_array_add (&L, &z);
  list_array_add (&L, &y);
  list_array_add (&L, &q);

  mcsh_parameters P;
  mcsh_parameterize(&sg, &L, &P);

  mcsh_parameters_print(&P);

  list_array_finalize(&L);

  return 0;
}
