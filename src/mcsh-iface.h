
#pragma once

#include <stdbool.h>

#include "util.h"

bool mcsh_iface_get(const char* prompt, char** line);

/** True if Unicode features (e.g. the shamrock prompt) are on:
      MCSH_UNICODE=0        off
      MCSH_UNICODE_FORCE=1  on; switches to a UTF-8 locale if possible
      otherwise             on if the locale uses UTF-8 and TERM is
                            not dumb, linux, vt52/vt100/..., or unset
    MCSH_UNICODE=0 takes precedence.  Decided once, on first call,
    which also sets the character-type locale for readline. */
bool mcsh_unicode(void);

/** Why mcsh_unicode() decided as it did, e.g. "TERM=linux" */
const char* mcsh_unicode_reason(void);

/** The character set of the current character-type locale, e.g.
    "UTF-8" or "ANSI_X3.4-1968" (ASCII) */
const char* mcsh_locale_codeset(void);

/** The default interactive prompt for program name, e.g. "mcsh": the
    name, then a shamrock (U+2618) and two spaces if mcsh_unicode(),
    otherwise ">" and one space.  Returns static storage. */
const char* mcsh_prompt_dflt(const char* name);

/** The readline library version, e.g. "8.2", or NULL if mcsh was
    built without readline */
const char* mcsh_iface_readline_version(void);
