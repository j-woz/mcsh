
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "strlcpyj.h"
#include "util.h"

static bool enabled = false;
static const int buffer_size = 100 * 1024;
static bool flush = true;
// Internal debugging of the logger itself
static bool debug = false;

static void init_entries(mcsh_logger* logger);

bool
mcsh_log_init(mcsh_logger* logger, pid_t pid, mcsh_log_lvl lvl)
{
  init_entries(logger);
  logger->lvl      = lvl;
  logger->stream   = stdout;
  logger->show_pid = false;
  if (pid == 0)
    logger->pid = getpid();
  else
    logger->pid = pid;

  bool b = false;
  bool rc = getenv_boolean("MCSH_LOG", b, &b);
  if (! rc) return false;
  mcsh_log_enable(b);
  return true;
}

void
mcsh_log_enable(bool b)
{
  enabled = b;
}

struct log_setting
{
  mcsh_log_cat    category;
  mcsh_log_state state;
  mcsh_log_lvl   level;
  char label[64];
};

static void log_entry_setting(mcsh_logger* logger,
                              struct log_setting* setting);

/** Set initial entries- they may be changed by user code later... */
static void
init_entries(mcsh_logger* logger)
{
  struct log_setting settings[mcsh_log_cats] =
    {
      {MCSH_LOG_NULL,    MCSH_LOG_OFF,     MCSH_ZERO,  "NULL"   },
      {MCSH_LOG_SAY,     MCSH_LOG_ON,      MCSH_INFO,  "SAY"   },
      {MCSH_LOG_CORE,    MCSH_LOG_OFF,     MCSH_INFO, "CORE"   },
      {MCSH_LOG_PARSE,   MCSH_LOG_ON,      MCSH_DEBUG, "PARSE"  },
      {MCSH_LOG_SYSTEM,  MCSH_LOG_ON,      MCSH_WARN,  "SYSTEM" },
      {MCSH_LOG_EVAL,    MCSH_LOG_ON,      MCSH_INFO, "EVAL"   },
      {MCSH_LOG_CONTROL, MCSH_LOG_ON,      MCSH_INFO, "CONTROL"},
      {MCSH_LOG_DATA,    MCSH_LOG_ON,      MCSH_WARN, "DATA"   },
      {MCSH_LOG_EXEC,    MCSH_LOG_DEFAULT, MCSH_INFO,  "EXEC"   },
      {MCSH_LOG_MEM,     MCSH_LOG_OFF,      MCSH_INFO,  "MEM"   },
      {MCSH_LOG_BUILTIN, MCSH_LOG_ON,      MCSH_WARN,  "BUILTIN"},
      {MCSH_LOG_MODULE,  MCSH_LOG_ON,      MCSH_INFO,  "MODULE" }
    };

  for (int i = 0; i < mcsh_log_cats; i++)
    log_entry_setting(logger, &settings[i]);
}

static void
log_entry_setting(mcsh_logger* logger, struct log_setting* setting)
{
  mcsh_log_entry_set(logger, setting->category,
                     setting->state, setting->level,
                     setting->label);
}

void
mcsh_log_entry_set(mcsh_logger* logger, mcsh_log_cat cat,
                   mcsh_log_state state, mcsh_log_lvl lvl,
                   const char* label)
{
  logger->entry[cat].state = state;
  logger->entry[cat].lvl   = lvl;
  size_t n = strnlen(label, MCSH_LOG_LABEL_MAX);
  valgrind_assert_msg(n < MCSH_LOG_LABEL_MAX, "log label too long!");
  strcpy(logger->entry[cat].label, label);
}

static bool do_check(mcsh_logger* logger,
                     mcsh_log_cat cat,
                     mcsh_log_lvl lvl_message);

bool
mcsh_log_check(mcsh_logger* logger, mcsh_log_cat cat,
               mcsh_log_lvl lvl)
{
  if (debug) printf("mcsh_log_check()...\n");

  bool result = do_check(logger, cat, lvl);
  return result;
}

static void report_va(mcsh_logger* logger, const char* label,
                      const char* format, va_list ap);

void
mcsh_log(mcsh_logger* logger, mcsh_log_cat cat,
         mcsh_log_lvl lvl, const char* format, ...)
{
  // First, check if we are really going to write this log message:
  if (! do_check(logger, cat, lvl)) return;

  // Look up the log category entry:
  mcsh_log_entry* entry = &logger->entry[cat];
  va_list ap;
  va_start(ap, format);
  report_va(logger, entry->label, format, ap);
  va_end(ap);
}

static void
report(mcsh_logger* logger, const char* label,
       const char* format, ...)
{
  // User message:
  va_list ap;
  va_start(ap, format);
  report_va(logger, label, format, ap);
  va_end(ap);
}

static void
report_va(mcsh_logger* logger, const char* label,
          const char* format, va_list ap)
{
   // Make a prefix with the log label and a colon:
  const int label_size = 16;
  char prefix[label_size];
  size_t n;
  n = strlcpyj(prefix, label, label_size);
  prefix[n]   = ':';
  prefix[n+1] = '\0';

  // Fill in the output buffer components: DATE LABEL MESSAGE
  char buffer[buffer_size];
  char* p = &buffer[0];

  if (logger->show_pid) appendf(p, "%7i ", logger->pid);

  n = time_string(p);
  p += n;
  appendf(p, " %-8s ", label);
  appendv(p, format, ap);

  // Write the buffer:
  fprintf(logger->stream, "%s\n", buffer);
  if (flush) fflush(logger->stream);
}

static inline bool
do_check(mcsh_logger* logger,
         mcsh_log_cat cat,
         mcsh_log_lvl lvl_message)
{
  if (!enabled) return false;

  // Silently allow NULL loggers:
  if (logger == NULL)
  {
    if (debug) report(logger, "LOGGER",
                      "mcsh_log_check(): logger is NULL");
    return false;
  }

  if (cat == MCSH_LOG_NULL)
  {
    if (debug) report(logger, "LOGGER", "logger is LOG_NULL");
    return false;
  }
  if (cat == MCSH_LOG_SAY)
  {
    if (debug) report(logger, "LOGGER", "logger is LOG_SAY");
    return true;
  }

  // Look up the log category entry:
  mcsh_log_entry* entry = &logger->entry[cat];

  mcsh_log_lvl threshold = MCSH_ZERO;
  switch (entry->state)
  {
    case MCSH_LOG_OFF:
      return false;
    case MCSH_LOG_ON:
      threshold = entry->lvl;
      break;
    case MCSH_LOG_DEFAULT:
      threshold = logger->lvl;
      break;
    default:
      valgrind_fail_msg("Unknown log state: %i", lvl_message);
  }

  if (lvl_message < threshold)
    return false;

  // Message lvl exceeds relevant threshold:
  return true;
}

void
mcsh_log_finalize(UNUSED mcsh_logger* logger)
{
  fflush(stdout);
}
