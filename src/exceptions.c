
#include "mcsh.h"
#include "exceptions.h"

void
mcsh_exception_invalid_argc(mcsh_status* status,
                            const char* name, int required, int given)
{
  mcsh_raise0(status,
             "mcsh.invalid_arguments",
             "call to '%s' requires %i arguments, given %i",
             name, required, given);
}

void
mcsh_exception_toofew_argc(mcsh_status* status,
                           const char* name, int required, int given)
{
  mcsh_raise0(status,
             "mcsh.invalid_arguments",
             "call to '%s' requires at least %i arguments, given %i",
             name, required, given);
}

void
mcsh_exception_invalid_type(mcsh_status* status,
                            const char* name,
                            mcsh_value* value,
                            mcsh_value_type type_required,
                            int index)
{
  char type_name_given   [64];
  char type_name_required[64];
  mcsh_value_type_name(value->type,   type_name_given);
  mcsh_value_type_name(type_required, type_name_required);
  mcsh_raise0(status,
             "mcsh.invalid_type",
             "call to '%s' requires a %s, given a %s, in argument %i",
             name, type_name_required, type_name_given, index);
}
