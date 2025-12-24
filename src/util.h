
/**
   UTIL H

   These functions should have no knowledge of MCSH.

   MCSH Errors: There are 2 categories, 3 types:
   INTERNAL: These are really MCSH bugs
   - ASSERTS: These trigger assert() or valgrind_assert() failures
              Valgrind may provide a stack trace
   - CHECKS:  These trigger return false.
              We can provide a stack trace and possibly recover
   - EXCEPTIONS: User errors that the user can handle.
*/

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED   __attribute__((unused))
#define NORETURN __attribute__((noreturn))

void show(const char* format, ...)
  __attribute__ ((format (printf, 1, 2)));

#define CHECK_FAILED(format, args...)           \
  do {                                          \
    printf("CHECK FAILED: %s:%i " format "\n",  \
           __FILE__, __LINE__,                  \
           ##args);                             \
    fflush(NULL);                               \
    return false;                               \
  } while (0);

/** Nice vargs error check and message.
    This is for internal errors.
    User errors should trigger MCSH exceptions.
    This could be disabled with a future option for speed.
 */
#define CHECK(condition, format, args...)  \
  do {                                     \
    if (!(condition)) {                    \
      CHECK_FAILED(format, ##args);        \
    } } while (0);

/** Like CHECK() but no error message */
#define CHECK0(condition)                    \
  do {                                       \
    if (!(condition)) {                      \
      CHECK_FAILED("Internal error!");       \
    } } while (0);

/** Called when the valgrind_assert() condition fails */
NORETURN
void valgrind_assert_failed(const char* file, int line);

/** Called when the valgrind_assert_msg() condition fails */
NORETURN
void valgrind_assert_failed_msg(const char* file, int line,
                                const char* format, ...);

/** Make DBG_ENABLED a compile-time constant */
#define DEBUG(DBG_ENABLED, format, args...)               \
  do {                                                    \
    if (DBG_ENABLED) { printf(format "\n", ## args); }    \
  } while (0);

/**
   VALGRIND_ASSERT
   Substitute for assert(): provide stack trace via valgrind
   If not running under valgrind, works like assert()
   For internal errors, not user errors.
 */
#ifdef NDEBUG
#define valgrind_assert(condition)             (void) (condition);
#define valgrind_assert_msg(condition, msg...) (void) (condition);
#else
#define valgrind_assert(condition) \
    if (!(condition)) \
    { valgrind_assert_failed(__FILE__, __LINE__); }
#define valgrind_assert_msg(condition, msg...) \
    if (!(condition)) \
    { valgrind_assert_failed_msg(__FILE__, __LINE__, ## msg); }
#endif

/**
   Cause valgrind assertion error behavior w/o condition
 */
#define valgrind_fail() \
  valgrind_assert_failed(__FILE__, __LINE__)

#define valgrind_fail_msg(msg...) \
  valgrind_assert_failed_msg(__FILE__, __LINE__, ## msg)

/**
   Read a whole file into a newly allocated string
 */
char* slurp(const char* filename);

char* slurp_fp(FILE* fp);

static inline void*
malloc_checked(size_t n)
{
  void* result = malloc(n);
  valgrind_assert(result != NULL);
  return result;
}

static inline void*
calloc_checked(size_t n, size_t size)
{
  void* result = calloc(n, size);
  valgrind_assert(result != NULL);
  return result;
}

static inline void*
realloc_checked(void* ptr, size_t n)
{
  // printf("realloc: from: %p\n", ptr);
  void* result = realloc(ptr, n);
  // printf("realloc: to:   %p\n", result);
  valgrind_assert(result != NULL);
  return result;
}

static inline char*
strdup_checked(char* ptr)
{
  valgrind_assert(ptr != NULL);

  char* result = strdup(ptr);
  valgrind_assert(result != NULL);

  return result;
}

/** Like strdup but NULL is allowed */
static inline char*
strdup_null_checked(char* ptr)
{
  char* result;
  if (ptr != NULL)
  {
    result = strdup(ptr);
    valgrind_assert(result != NULL);
  }
  else
    result = NULL;
  return result;
}

/**
   Free and reset this pointer: USAGE: null(&p);
   NOTE: Pass in the address of the pointer you want to modify!
         (Thus actually a pointer-pointer.  We
          do this because of C auto-casting limits.)

*/
static inline void
null(void* p)
{
  void** pp = (void**) p;
  free(*pp);
  *pp = NULL;
}

/**
   null-predicated: Free and reset this pointer if not already NULL
   Return true if the pointer was non-NULL and is now NULL,
   else return false
   NOTE: Pass in the address of the pointer you want to modify
         (Thus actually a pointer-pointer.  We
          do this because of C auto-casting limits.)
*/
static inline bool
nullp(void* p)
{
  void** pp = (void**) p;
  if (*pp == NULL)
    return false;
  // TODO: use null(p)
  free(*pp);
  *pp = NULL;
  return true;
}

/**
   Assign data to target if target is not NULL
   target is actually a **
 */
static inline void
maybe_assign(void* target, void* data)
{
  char** p = target;
  if (p != NULL)
  {
    *p = data;
    // printf("maybe did. %p %p\n", p, *p);
  }
  /* else */
  /*   printf("maybe did not. %p\n", p); */
}

NORETURN
void fail(const char* fmt, ...);

/*
  String Builders
  The following macros copy data and move the target pointer
*/

/// Append possibly w/o NUL byte
#define copy(string, a, n) \
  { memcpy(string, a, n); string += n; }
/// Append plain string
#define append(string, a, n) \
  do { string += strlcpy(string, a, n); } while (0);
/// Append Formatted string
#define appendf(string, args...) \
  string += sprintf(string, ## args)
/// Append Varargs string
#define appendv(string, args...) \
  string += vsprintf(string, format, ap)

/** Return current time as nice string */
size_t time_string(char* s);

/** Obtain the parent of filename, in-place
    "d/f" -> "d"
    "f"   -> "."
    ""    -> "."
*/
bool parent(char* filename);

/** output may be NULL */
bool is_integer(const char* s, size_t* output);

/**
   Receive a true/false setting by env var, which is
   false if "0", or false (case-insensitive),
   and true for a non-zero number or true (case-insensitive)
   If not found, return default value
   @return True, false if string could not be converted to boolean
 */
bool getenv_boolean(const char* name, bool dflt, bool* result);

/**
   Remove whitespace from right side of input s in-place.
   Return s.
*/
char* trim_right(char* s);

static char string_true[]  = "true";
static char string_false[] = "false";

static char* TF(bool b)
{
  return b ? string_true : string_false;
}
