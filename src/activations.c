
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "activations.h"

mcsh_activation mcsh_activation_time,
                mcsh_activation_random,
                mcsh_activation_this,
                mcsh_activation_last;

static bool
activation_time_get(UNUSED mcsh_entry* entry,
                    UNUSED const char* name,
                    mcsh_value** output,
                    mcsh_status* status)
{
  // time_t t = time(NULL);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double f = (double) tv.tv_sec;
  f += ((double) tv.tv_usec) / 1000000.0;
  mcsh_value* value = mcsh_value_new_float(f);
  *output = value;
  status->code = MCSH_OK;
  return true;
}

static bool
activation_time_set(UNUSED mcsh_entry* entry,
                    UNUSED const char* name,
                    UNUSED mcsh_value* input,
                    mcsh_status* status)
{
  mcsh_raise(status, NULL, 0,
             "MCSH_READ_ONLY", "cannot assign to time");
  return true;
}

static void
activation_random_init(void)
{
  srandom(42);
}

static bool
activation_random_set(mcsh_entry* entry,
                      UNUSED const char* name,
                      mcsh_value* input,
                      mcsh_status* status)
{
  mcsh_logger* logger = &entry->module->vm->logger;
  int64_t seed;
  bool rc = mcsh_value_integer(input, &seed);
  valgrind_assert(rc);
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "random set: %"PRId64, seed);
  srandom(seed);
  status->code = MCSH_OK;
  return true;
}

static bool
activation_random_get(UNUSED mcsh_entry* entry,
                      UNUSED const char* name,
                      mcsh_value** output,
                      mcsh_status* status)
{
  long r = random();
  mcsh_value* value = mcsh_value_new_int(r);
  *output = value;
  status->code = MCSH_OK;
  return true;
}

static bool
activation_this_get(mcsh_entry* entry,
                    UNUSED const char* name,
                    mcsh_value** output,
                    mcsh_status* status)
{
  char* s = entry->stack->vm->argv[0];
  if (strcmp(s, "stdin") == 0)
  {
    *output = &mcsh_null;
    return true;
  }
  char  t[PATH_MAX];
  strcpy(t, s);
  bool rc = parent(t);
  assert(rc);
  char  d[PATH_MAX];
  char* r = realpath(t, d);
  assert(r != NULL);
  char  p[PATH_MAX];
  char* c = getcwd(p, PATH_MAX);
  assert(c != NULL);
  strcat(c, "/");
  strcat(c, r);
  mcsh_value* result =
    mcsh_value_new_string(entry->stack->vm, c);
  *output = result;
  status->code = MCSH_OK;
  return true;
}

static bool
activation_this_set(UNUSED mcsh_entry* entry,
                    UNUSED const char* name,
                    UNUSED mcsh_value* input,
                    mcsh_status* status)
{
  mcsh_raise(status, NULL, 0,
             "MCSH_READ_ONLY", "cannot assign to 'this'");
  return true;
}

static bool
activation_last_get(UNUSED mcsh_entry* entry,
                    UNUSED const char* name,
                    mcsh_value** output,
                    mcsh_status* status)
{
  int code = entry->stack->vm->exit_code_last;
  mcsh_value* result = mcsh_value_new_int(code);
  *output = result;
  status->code = MCSH_OK;
  return true;
}

static bool
activation_last_set(UNUSED mcsh_entry* entry,
                    UNUSED const char* name,
                    UNUSED mcsh_value* input,
                    mcsh_status* status)
{
  mcsh_raise(status, NULL, 0,
             "MCSH_READ_ONLY", "cannot assign to '?'");
  return true;
}

static void init(const char* name, mcsh_activation* a,
                 struct table* T, mcsh_logger* logger);

void
mcsh_activation_init(mcsh_logger* logger, struct table* T)
{
  mcsh_activation_time  .get = activation_time_get;
  mcsh_activation_time  .set = activation_time_set;
  mcsh_activation_random.get = activation_random_get;
  mcsh_activation_random.set = activation_random_set;
  mcsh_activation_this  .get = activation_this_get;
  mcsh_activation_this  .set = activation_this_set;
  mcsh_activation_last  .get = activation_last_get;
  mcsh_activation_last  .set = activation_last_set;

  init("time",   &mcsh_activation_time,   T, logger);
  init("random", &mcsh_activation_random, T, logger);
  init("this",   &mcsh_activation_this,   T, logger);
  init("?",      &mcsh_activation_last,   T, logger);

  activation_random_init();
}

static void
init(const char* name, mcsh_activation* a,
     struct table* T, mcsh_logger* logger)
{
  mcsh_value* v = mcsh_value_new_activation(a);
  table_add(T, name, v);
  mcsh_value_grab(logger, v);
}
