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
AM_PROG_AR
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
AC_CHECK_FUNCS([bzero dup2 getcwd gettimeofday localtime_r memmove])
AC_CHECK_FUNCS([memset realpath regcomp strchr strdup strerror])
AC_CHECK_FUNCS([strfromd strlcpy strndup strstr strtol])

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

# Build information for 'mcsh -B', recorded in config.h as strings.
# mcsh_esc: escape backslashes and double quotes for a C string
mcsh_esc()
{
  printf '%s' "$1" | sed 's/[[\\"]]/\\&/g'
}
# mcsh_version: first line of 'PROGRAM --version', or unknown
mcsh_version()
{
  mcsh_v=$($1 --version 2>/dev/null | head -n 1)
  printf '%s' "${mcsh_v:-unknown}"
}
mcsh_b_date=$(date '+%Y-%m-%d %H:%M:%S')
mcsh_b_args=$(mcsh_esc "$(printf '%s' "$ac_configure_args" | sed 's/^ *//')")
mcsh_b_git=$(cd "$srcdir" && git describe --always --dirty 2>/dev/null)
mcsh_b_host=$(mcsh_esc "$(uname -srm)")
mcsh_b_prefix=$(mcsh_esc "$prefix")
# Without --prefix, configure has prefix=NONE until later; report
# the default that will be used:
if test "x$prefix" = xNONE
then
  mcsh_b_prefix="$ac_default_prefix (default)"
fi
mcsh_b_cc=$(mcsh_esc "$CC")
mcsh_b_cc_version=$(mcsh_esc "$(mcsh_version "$CC")")
mcsh_b_cppflags=$(mcsh_esc "$CPPFLAGS")
mcsh_b_cflags=$(mcsh_esc "$CFLAGS")
mcsh_b_ldflags=$(mcsh_esc "$LDFLAGS")
mcsh_b_libs=$(mcsh_esc "$LIBS")
mcsh_b_lex=$(mcsh_esc "$LEX: $(mcsh_version "$LEX")")
mcsh_b_yacc=$(mcsh_esc "$YACC: $(mcsh_version "$YACC")")
mcsh_b_readline="requested $with_readline, found ${ax_cv_lib_readline:-no}"
AC_DEFINE_UNQUOTED([MCSH_BUILD_CONFIGURE_DATE], ["$mcsh_b_date"],
                   [mcsh -B: when configure ran])
AC_DEFINE_UNQUOTED([MCSH_BUILD_CONFIGURE_ARGS], ["$mcsh_b_args"],
                   [mcsh -B: arguments given to configure])
AC_DEFINE_UNQUOTED([MCSH_BUILD_GIT], ["${mcsh_b_git:-unknown}"],
                   [mcsh -B: git commit when configure ran])
AC_DEFINE_UNQUOTED([MCSH_BUILD_HOST], ["$mcsh_b_host"],
                   [mcsh -B: system that ran configure])
AC_DEFINE_UNQUOTED([MCSH_BUILD_PREFIX], ["$mcsh_b_prefix"],
                   [mcsh -B: installation prefix])
AC_DEFINE_UNQUOTED([MCSH_BUILD_CC], ["$mcsh_b_cc"],
                   [mcsh -B: C compiler])
AC_DEFINE_UNQUOTED([MCSH_BUILD_CC_VERSION], ["$mcsh_b_cc_version"],
                   [mcsh -B: C compiler version])
AC_DEFINE_UNQUOTED([MCSH_BUILD_CPPFLAGS], ["$mcsh_b_cppflags"],
                   [mcsh -B: CPPFLAGS])
AC_DEFINE_UNQUOTED([MCSH_BUILD_CFLAGS], ["$mcsh_b_cflags"],
                   [mcsh -B: CFLAGS])
AC_DEFINE_UNQUOTED([MCSH_BUILD_LDFLAGS], ["$mcsh_b_ldflags"],
                   [mcsh -B: LDFLAGS])
AC_DEFINE_UNQUOTED([MCSH_BUILD_LIBS], ["$mcsh_b_libs"],
                   [mcsh -B: LIBS])
AC_DEFINE_UNQUOTED([MCSH_BUILD_LEX], ["$mcsh_b_lex"],
                   [mcsh -B: lexer generator and version])
AC_DEFINE_UNQUOTED([MCSH_BUILD_YACC], ["$mcsh_b_yacc"],
                   [mcsh -B: parser generator and version])
AC_DEFINE_UNQUOTED([MCSH_BUILD_READLINE], ["$mcsh_b_readline"],
                   [mcsh -B: readline as requested and found])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
