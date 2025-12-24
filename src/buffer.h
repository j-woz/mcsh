
#pragma once

#include <sys/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/** The amount of data that can be added in one function call */
static size_t buffer_chunk_size = 1024;

typedef struct
{
  /** Start of user data */
  char* data;
  /** Byte length: includes NULL byte if string */
  size_t length;
  size_t capacity;
} buffer;

static inline void
buffer_init(buffer* B, size_t capacity)
{
  B->data = malloc_checked(capacity);
  B->length = 0;
  B->capacity = capacity;
}

static inline void
buffer_reset(buffer* B)
{
  B->data[0] = '\0';
  B->length = 0;
}

static inline void
buffer_chunk_size_set(size_t c)
{
  buffer_chunk_size = c;
}

static inline size_t
buffer_chunk_size_get(void)
{
  return buffer_chunk_size;
}

static inline char*
buffer_dup(buffer* B)
{
  return strdup(B->data);
}

static inline void
catn(buffer* B, const char* data, size_t count);

/// cat means string operation like strcat

static inline void
buffer_cat(buffer* B, const char* text)
{
  // printf("cat: '%s'\n", text);
  size_t count = strlen(text);
  catn(B, text, count);
}

/** Put buffer a after B as string */
static inline void
buffer_catb(buffer* B, buffer* a)
{
  // The length here is the strlen():
  catn(B, a->data, a->length-1);
}

static inline void buffer_catn(buffer* B, const char* data,
                               size_t count);

static inline void
buffer_catv(buffer* B, const char* format, ...)
{
  char chunk[buffer_chunk_size];
  va_list ap;
  va_start(ap, format);
  size_t count = vsnprintf(chunk, buffer_chunk_size, format, ap);
  va_end(ap);

  valgrind_assert(count < buffer_chunk_size);

  buffer_catn(B, chunk, count);
}

static inline void
check_size(buffer* B, size_t count)
{
  size_t capacity_new = B->capacity;
  // The new required data length:
  size_t length_new = B->length + count;
  while (length_new > capacity_new)
    capacity_new *= 2;
  if (B->capacity == capacity_new)
    return;  // No change in size

  // printf("realloc: %zi -> %zi\n", B->capacity, capacity_new);
  char* new = realloc_checked(B->data, capacity_new);
  B->capacity = capacity_new;
  B->data = new;
}

/** count is strlen: does not include NULL byte */
static inline void
buffer_catn(buffer* B, const char* data, size_t count)
{
  catn(B, data, count);
}

/** count is the strlen() of the text */
static inline void
catn(buffer* B, const char* data, size_t count)
{
  check_size(B, count+1);

  // Empty buffer?
  if (B->length == 0)
  {
    memcpy(B->data, data, count+1);
    // Include trailing NULL byte:
    B->length = count + 1;
    return;
  }

  // Assume there is already a trailing NULL, overwrite it
  char* tail = B->data + B->length - 1;
  memcpy(tail, data, count + 1);
  // There was already a NUL byte, we just moved it:
  B->length += count;
}

/** Put character c at end of buffer, NULL-terminate */
static inline void
buffer_catc(buffer* B, char c)
{
  check_size(B, 2);

  // Empty buffer?
  if (B->length == 0)
  {
    B->data[0] = c;
    B->data[1] = '\0';
    // Include trailing NULL byte:
    B->length = 2;
    return;
  }

  char* tail = B->data + B->length - 1;
  tail[0] = c;
  tail[1] = '\0';
  B->length++;
}

// /** Put buffer a after B as binary data */
// UNUSED void buffer_putb(buffer* B, buffer* a);
// Not yet needed/implemented.

void buffer_show(buffer* B);

static inline void
buffer_finalize(buffer* B)
{
  free(B->data);
}

static inline void
buffer_free(buffer* B)
{
  buffer_finalize(B);
  free(B);
}

bool slurp_buffer(FILE* stream, buffer* B);
