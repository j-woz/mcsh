
#pragma once

#include "mcsh.h"

bool mcsh_exec(UNUSED mcsh_module* module,
               char* cmd, char** a,
               mcsh_value** output,
               mcsh_status* status);

bool mcsh_subcmd_capture(mcsh_module* module,
                         mcsh_stmts* stmts,
                         mcsh_value** output,
                         mcsh_status* status);

bool mcsh_bg(mcsh_module* module,
             mcsh_stmts* stmts,
             mcsh_value** output,
             mcsh_status* status);
