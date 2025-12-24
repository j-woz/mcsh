
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/**
   Number of logging categories, including LOG_NULL
   Make sure this agrees with enum mcsh_logcat
*/
#define mcsh_logcats 11

/**
    Logging categories
    Make sure this agrees with counter mcsh_logcats
         and log.c:init_entries()
*/
typedef enum
{
  MCSH_LOG_NULL    =  0,
  /** For MCSH core activity: */
  MCSH_LOG_CORE    =  1,
  /** For MCSH system-level activity: */
  MCSH_LOG_SYSTEM  =  2,
  /** For MCSH parser activity: */
  MCSH_LOG_PARSE   =  3,
  /** For MCSH data operations */
  MCSH_LOG_DATA    =  4,
  /** For string/math evaluation */
  MCSH_LOG_EVAL    =  5,
  /** For program control flow */
  MCSH_LOG_CONTROL =  6,
  /** For MCSH external execution */
  MCSH_LOG_EXEC    =  7,
  /** Memory leak detection */
  MCSH_LOG_MEM     =  8,
  /** MCSH builtins */
  MCSH_LOG_BUILTIN =  9,
  /** MCSH modules */
  MCSH_LOG_MODULE  = 10
} mcsh_logcat;

typedef enum
{
  /** Use the logger level */
  MCSH_LOG_DEFAULT = 0,
  /** Use this category's level */
  MCSH_LOG_ON,
  /** Disable */
  MCSH_LOG_OFF
} mcsh_log_state;

/** A log level */
typedef uint8_t mcsh_log_lvl;

// Built-in log levels follow:
static const mcsh_log_lvl MCSH_ZERO  =   0;
static const mcsh_log_lvl MCSH_TRACE =  20;
static const mcsh_log_lvl MCSH_DEBUG =  30;
static const mcsh_log_lvl MCSH_INFO  =  50;
static const mcsh_log_lvl MCSH_WARN  =  70;
static const mcsh_log_lvl MCSH_FATAL =  90;
static const mcsh_log_lvl MCSH_FORCE = 100;

#define MCSH_LOG_LABEL_MAX 32

typedef struct
{
  mcsh_log_state state;
  mcsh_log_lvl lvl;
  char label[MCSH_LOG_LABEL_MAX];
} mcsh_log_entry;

typedef struct
{
  mcsh_log_entry entry[mcsh_logcats];
  mcsh_log_lvl lvl;
  FILE* stream;
  bool  show_pid;
  pid_t pid;
} mcsh_logger;

#define LOG(cat, lvl, format...)          \
  do {                                    \
    mcsh_log(logger, cat, lvl, ##format); \
  } while (0);

bool mcsh_log_init(mcsh_logger* logger, pid_t pid, mcsh_log_lvl lvl);

void mcsh_log_enable(bool b);

void mcsh_log_entry_set(mcsh_logger* logger, mcsh_logcat cat,
                        mcsh_log_state state, mcsh_log_lvl lvl,
                        const char* label);

bool mcsh_log_check(mcsh_logger* logger, mcsh_logcat cat,
                    mcsh_log_lvl lvl);

void mcsh_log(mcsh_logger* logger, mcsh_logcat cat,
              mcsh_log_lvl lvl, char* format, ...);

void mcsh_log_finalize(mcsh_logger* logger);
