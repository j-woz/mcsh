
#pragma once

// #incl ude "mcs h.h"  // for FlyCheck

#define mcsh_resolve(_v) \
  do { if (_v->type == MCSH_VALUE_LINK) _v=_v->link; } while (0);

void mcsh_data_init(mcsh_vm* vm);

bool mcsh_token_to_value(mcsh_logger* logger,
                         mcsh_entry* entry, const char* token,
                         mcsh_value** output, mcsh_status* status);

void mcsh_stack_print(mcsh_module* module);

bool mcsh_set_value(mcsh_module* module,
                    const char* name, mcsh_value* value,
                    mcsh_status* status);

bool mcsh_drop_variable(mcsh_module* module,
                        const char* name,
                        mcsh_status* status);

void mcsh_data_finalize(mcsh_vm* vm);

void mcsh_assign_specials(mcsh_vm* vm, strmap* parameters);

bool mcsh_data_env(mcsh_vm* vm, const char* name,
                   mcsh_value** result);

bool mcsh_drop_env(mcsh_vm* vm, const char* name);

bool mcsh_data_special(mcsh_vm* vm, const char* name,
                       mcsh_value** result);

/**
   @param L list_array of mcsh_value
   Stringify each value and join to result
*/
void mcsh_join_list_to_buffer(mcsh_logger* logger,
                              list_array* L, const char* delimiter,
                              buffer* result);

void mcsh_join_table_to_buffer(mcsh_logger* logger,
                               struct table* T, const char* delimiter,
                               buffer* result);
