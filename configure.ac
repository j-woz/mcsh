#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([MCSH], [0.0], [justin@wozniaks.org])
AC_CONFIG_SRCDIR([src/mcsh.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
# [subdir-objects]

AC_USE_SYSTEM_EXTENSIONS

m4_include([m4/ax_lib_readline.m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX([noyywrap])
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_YACC([bison])

# Checks for libraries.
AC_CHECK_LIB([c], [malloc])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([inttypes.h libintl.h limits.h malloc.h stddef.h])
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([sys/param.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 getcwd gettimeofday localtime_r memset regcomp])
AC_CHECK_FUNCS([strchr strdup strerror strfromd strlcpy strstr])
AC_CHECK_FUNCS([strtol])

# Checks for libraries.
AC_ARG_ENABLE([readline],
  AS_HELP_STRING([--enable-readline],
        [Enables readline. Provide "yes", "required", or "no". Default "yes".]),
  [with_readline=$enableval],
  [with_readline="yes"])

if [[ $with_readline = "yes" ]] || [[ $with_readline = "required" ]]
then
  AX_LIB_READLINE()
elif [[ $with_readline = "no" ]]
then
  AC_MSG_RESULT([User disabled readline.])
  ax_cv_lib_readline=no
  ax_cv_lib_readline_history=no
else
  AC_MSG_ERROR([Bad argument for readline: '$with_readline'])
fi
if ! [[ $with_readline = "no" ]]
then
  if [[ ${ax_cv_lib_readline} = "no" ]]
  then
    if [[ $with_readline = "required" ]]
    then
      AC_MSG_ERROR([Readline not found, but was required.  Aborting...])
    fi
    AC_MSG_RESULT([Not found: readline.  Proceeding without readline...])
  fi
  if [[ ${ax_cv_lib_readline_history:-no} = "no" ]]
  then
    if [[ $with_readline = "required" ]]
    then
      AC_MSG_ERROR([Readline history not found, but was required.  Aborting...])
    fi
    AC_MSG_RESULT([Not found: history.  Proceeding without history...])
  fi
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
