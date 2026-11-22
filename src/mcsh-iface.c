
#include <ctype.h>
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mcsh-iface.h"

#include "config.h"

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
static bool have_readline = true;
#else
static bool have_readline = false;
#endif

static bool iface_get_readline(const char* prompt, char** line);
static bool iface_get_plain   (const char* prompt, char** line);

static bool initialized = false;

#ifdef HAVE_LIBREADLINE
static char* history_file = NULL;
#endif

/** Adopt the user's character-type locale (LC_CTYPE), once, so that
    readline handles multibyte characters, e.g. the shamrock in the
    prompt; otherwise readline would count its bytes as separate
    characters and misplace the cursor. */
static void
locale_init(void)
{
  static bool done = false;
  if (done) return;
  done = true;
  setlocale(LC_CTYPE, "");
}

/** True if the current character-type locale uses UTF-8 */
static bool
locale_utf8(void)
{
  locale_init();
  const char* codeset = nl_langinfo(CODESET);
  return strcasecmp(codeset, "UTF-8") == 0 ||
         strcasecmp(codeset, "UTF8")  == 0;
}

/** Switch the character-type locale to UTF-8 if it is not already,
    using C.UTF-8 where the system has it; otherwise keep it. */
static void
locale_force_utf8(void)
{
  if (locale_utf8()) return;
  if (setlocale(LC_CTYPE, "C.UTF-8") == NULL)
    setlocale(LC_CTYPE, "C.utf8");
}

/** True if TERM is a terminal that cannot be trusted to show Unicode:
    unset or empty, dumb, the Linux console (linux, linux-*), or a DEC
    VT type (vt52, vt100, vt220, ...).  vte-256color is not one: VTE
    terminals handle Unicode. */
static bool
term_limited(void)
{
  const char* term = getenv("TERM");
  if (term == NULL || term[0] == '\0') return true;
  if (strcmp(term, "dumb")  == 0) return true;
  if (strcmp(term, "linux") == 0) return true;
  if (strncmp(term, "linux-", 6) == 0) return true;
  if (strncmp(term, "vt", 2) == 0 &&
      isdigit((unsigned char) term[2]))
    return true;
  return false;
}

/** A boolean environment variable, or dflt if it is unset, empty, or
    invalid (getenv_boolean() warns about an invalid value) */
static bool
env_flag(const char* name, bool dflt)
{
  bool result = dflt;
  if (! getenv_boolean(name, dflt, &result))
    result = dflt;
  return result;
}

/** Why mcsh_unicode() decided as it did, for mcsh -B */
static char unicode_reason[128] = "";

bool
mcsh_unicode(void)
{
  static int result = -1;  // not yet known
  if (result >= 0) return result;
  const char* term = getenv("TERM");
  if (! env_flag("MCSH_UNICODE", true))
  {
    result = false;
    strcpy(unicode_reason, "MCSH_UNICODE=0");
  }
  else if (env_flag("MCSH_UNICODE_FORCE", false))
  {
    locale_force_utf8();
    result = true;
    strcpy(unicode_reason, "MCSH_UNICODE_FORCE=1");
  }
  else if (! locale_utf8())
  {
    result = false;
    snprintf(unicode_reason, sizeof(unicode_reason),
             "locale is not UTF-8");
  }
  else if (term_limited())
  {
    result = false;
    snprintf(unicode_reason, sizeof(unicode_reason),
             "TERM=%s", (term == NULL) ? "(unset)" : term);
  }
  else
  {
    result = true;
    snprintf(unicode_reason, sizeof(unicode_reason),
             "UTF-8 locale and TERM=%s", term);
  }
  return result;
}

const char*
mcsh_unicode_reason(void)
{
  mcsh_unicode();
  return unicode_reason;
}

const char*
mcsh_locale_codeset(void)
{
  locale_init();
  return nl_langinfo(CODESET);
}

const char*
mcsh_prompt_dflt(const char* name)
{
  static char prompt[64];
  // U+2618 SHAMROCK, as UTF-8 bytes, with an extra space after it
  const char* mark = mcsh_unicode() ? "\xe2\x98\x98 " : ">";
  snprintf(prompt, sizeof(prompt), "%s%s ", name, mark);
  return prompt;
}

static inline void
iface_init(void)
{
  // The only real work in here is if readline/history are enabled

  if (initialized) return;
  initialized = true;
  // Set the character-type locale before readline first reads
  mcsh_unicode();

#ifdef HAVE_LIBREADLINE

  char* home = getenv("HOME");
  struct stat s;
  int rc;
  rc = stat(home, &s);
  if (rc != 0) return;  // No HOME directory!
  char p[PATH_MAX];
  strcpy(p, home);
  strcat(p, "/.config/mcsh");
  rc = stat(p, &s);
  if (rc != 0)
    goto use_home;
  if (S_ISDIR(s.st_mode))
    goto use_config_mcsh;

  use_home:  // Start over with HOME:
  strcpy(p, home);
  strcat(p, "/.mcsh_history");

  use_config_mcsh:

  strcat(p, "/mcsh.history");
  history_file = strdup(p);
  // printf("history_file: %s\n", history_file);
  read_history(history_file);
  // printf("entries: %i\n", history_length);
#endif
}

const char*
mcsh_iface_readline_version(void)
{
#ifdef HAVE_LIBREADLINE
  return rl_library_version;
#else
  return NULL;
#endif
}

bool
mcsh_iface_get(const char* prompt, char** line)
{
  iface_init();
  bool result;
  if (have_readline)
    result = iface_get_readline(prompt, line);
  else
    result = iface_get_plain(prompt, line);
  return result;
}

static void insert_history(char* line);

static bool
iface_get_readline(const char* prompt, char** line)
{
  fflush(stdout);
#ifdef HAVE_LIBREADLINE
  *line = readline(prompt);
  insert_history(*line);
#else
  printf("readline was not enabled at configure time.\n");
  *line = NULL;
#endif
  /* printf("readline: '%s'\n", *line); */
  /* fflush(stdout); */

  return true;
}

static void
insert_history(char* line)
{
  if (line == NULL || line[0] == '\0')
    return;
#ifdef HAVE_LIBREADLINE
  add_history(line);
  append_history(1, history_file);
#endif
}

static bool
iface_get_plain(const char* prompt, char** line)
{
  const int line_max = 1024;
  char b[line_max];
  printf("%s", prompt);
  fflush(stdout);
  char* t = fgets(b, line_max, stdin);
  if (t == NULL)
    *line = t;
  else
    *line = strdup(b);
  return true;
}
