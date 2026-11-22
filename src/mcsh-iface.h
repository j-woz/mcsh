
#pragma once

#include <stdbool.h>

#include "util.h"

bool mcsh_iface_get(const char* prompt, char** line);

/** The readline library version, e.g. "8.2", or NULL if mcsh was
    built without readline */
const char* mcsh_iface_readline_version(void);
