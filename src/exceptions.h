
#pragma once

/** Raise exception and return */
#define RAISE(status, line, source, tag, msg...) \
  do                                             \
  {                                              \
    mcsh_raise(status, line, source, tag, msg);  \
    return true;                                 \
  } while (0);

/** Conditionally raise exception and return */
#define RAISE_IF(condition, status, line, source, tag, msg...)    \
  do                                                              \
  {                                                               \
    if (condition)                                                \
    {                                                             \
      mcsh_raise(status, line, source, tag, msg);                 \
      return true;                                                \
    }                                                             \
  } while (0);

/** Conditionally raise exception and return */
#define TYPE_CHECK(value, required, status, name, index, msg...)   \
  do                                                \
  {                                                 \
    if (value->type != required)                    \
    {                                               \
      mcsh_exception_invalid_type(status, name,     \
                                  value, required,  \
                                  index);           \
      return true;                                  \
    }                                               \
  } while (0);

/** If mcsh_status.code indicates exception, just pass it up */
#define PROPAGATE(status) \
  do { if (status->code != MCSH_OK) return true; } while (0);

void mcsh_exception_invalid_argc(mcsh_status* status,
                                 const char* name,
                                 int required, int given);

void mcsh_exception_toofew_argc(mcsh_status* status,
                                const char* name,
                                int required, int given);

void mcsh_exception_invalid_type(mcsh_status* status,
                                 const char* name,
                                 mcsh_value* value,
                                 mcsh_value_type required,
                                 int index);
