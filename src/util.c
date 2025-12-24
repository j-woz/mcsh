
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "util.h"

void
show(const char* format, ...)
{
  printf("show: ");
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
  printf("\n");
  fflush(stdout);
}

NORETURN void
fail(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
  exit(EXIT_FAILURE);
}

/**
   Is there another way to detect we are under valgrind?
 */
static bool
using_valgrind(void)
{
  // User must set VALGRIND to get this to work
  char* s = getenv("VALGRIND");
  if (s != NULL && strlen(s) > 0)
    return true;
  return false;
}

static size_t barf = 1;

NORETURN void
valgrind_assert_failed(const char* file, int line)
{
  printf("valgrind_assert(): failed: %s:%d\n", file, line);
  fflush(NULL);
  if (using_valgrind())
  {
    printf("valgrind_assert(): inducing memory fault...\n");
    fflush(NULL);
    // This will give us more information from valgrind
    barf = line;
    puts((char*) barf);
  }
  printf("abort.\n");
  fflush(NULL);
  abort();
}

int buffer_size = 4096;

NORETURN void
valgrind_assert_failed_msg(const char* file, int line,
                           const char* format, ...)
{
  printf("\nvalgrind_assert(): failed: %s:%d\n", file, line);
  fflush(NULL);
  char buffer[buffer_size];
  int count = 0;
  char* p = &buffer[0];
  fflush(stdout);
  va_list ap;
  va_start(ap, format);
  count += sprintf(p, "valgrind_assert(): ");
  count += vsnprintf(buffer+count, (size_t)(buffer_size-count), format, ap);
  va_end(ap);
  printf("%s\n", buffer);
  fflush(NULL);
  if (using_valgrind())
  {
    printf("valgrind_assert(): inducing memory fault...\n");
    fflush(NULL);
    // This will give us more information from valgrind
    barf = line;
    puts((char*) barf);
  }
  exit(1);
}

char*
slurp(const char* filename)
{
  FILE* fp = fopen(filename, "r");
  if (fp == NULL)
  {
    printf("slurp(): could not read from: %s\n", filename);
    return NULL;
  }

  char* result = slurp_fp(fp);

  fclose(fp);
  return result;
}

char*
slurp_fp(FILE* fp)
{
  struct stat s;
  int rc = fstat(fileno(fp), &s);
  valgrind_assert(rc == 0);

  size_t length = s.st_size;
  char* result = malloc_checked(length+1);

  size_t actual = fread(result, sizeof(char), length, fp);
  if (actual != length)
  {
    free(result);
    return NULL;
  }
  result[length] = '\0';
  return result;
}

size_t
time_string(char* s)
{
  time_t my_time_t = time(NULL);
  struct tm my_tm;
  localtime_r(&my_time_t, &my_tm);
  size_t n = strftime(s, 128, "%Y-%m-%d %H:%M:%S", &my_tm);
  return n;
}

/**
   Like dirname
   Could use libgen.h dirname()
*/
bool
parent(char* filename)
{
  int n = strlen(filename);
  for (int i = n - 1; i >= 0; i--)
  {
    if (filename[i] == '/')
    {
      filename[i] = '\0';
      return true;
    }
  }
  strcpy(filename, ".");
  return false;
}

bool
is_integer(const char* s, size_t* output)
{
  errno = 0;
  char* t;
  size_t result = strtol(s, &t, 10);
  if (output != NULL) *output = result;
  if (s == t) return false;
  valgrind_assert(errno == 0);
  return true;
}

bool
getenv_boolean(const char* name, bool dflt, bool* result)
{
  char* s = getenv(name);
  if (s == NULL || strlen(s) == 0)
  {
    // Undefined or empty: return default
    *result = dflt;
    return true;
  }

  // Try to parse as number
  char* end = NULL;
  long num_val = strtol(s, &end, 10);
  if (end != NULL && end != s && *end == '\0')
  {
    // Whole string was number
    *result = (num_val != 0);
    return true;
  }

  // Try to parse as true/false
  size_t length = strlen(s);
  // should not be longer than 5 characters "false"
  const size_t max_length = 5;
  if (length > max_length)
    goto error;

  // Convert to lower case
  char lower_s[8];
  for (size_t i = 0; i < length; i++)
    lower_s[i] = (char) tolower(s[i]);
  lower_s[length] = '\0';

  if (strcmp(lower_s, "true") == 0)
    *result = true;
  else if (strcmp(lower_s, "false") == 0)
    *result = false;
  else
    goto error;

  // Successful return:
  return true;

  error:
  printf("Invalid boolean environment variable value: %s=\"%s\"\n",
         name, s);
  return false;
}

char*
trim_right(char* s)
{
  for (size_t i = strlen(s) - 1; i > 0; i++)
  {
    switch (s[i])
    {
      case '\n':
      case '\t':
      case ' ':
        s[i] = '\0';
        break;
      default:
        i = -1;  // break from top loop
    }
  }

  return s;
}
