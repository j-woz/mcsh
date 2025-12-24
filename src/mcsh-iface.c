
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static char* history_file = NULL;

static inline void
iface_init(void)
{
  if (initialized) return;
  initialized = true;
#ifndef HAVE_LIBREADLINE
  return;
#endif
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

#ifdef HAVE_LIBREADLINE
  strcat(p, "/mcsh.history");
  history_file = strdup(p);
  // printf("history_file: %s\n", history_file);
  read_history(history_file);
  // printf("entries: %i\n", history_length);
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
