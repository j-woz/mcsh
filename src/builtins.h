
#pragma once

#include <stdbool.h>

#include "mcsh.h"
#include "list-array.h"

void mcsh_builtins_init(void);

bool mcsh_builtins_has(const char* symbol);

bool mcsh_builtins_execute(mcsh_module* module, list_array* args,
                           mcsh_value** value,  mcsh_status* status);

void mcsh_builtins_finalize(void);
