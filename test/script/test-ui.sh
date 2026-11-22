#!/bin/zsh -f
set -eu
setopt PIPE_FAIL

# TEST UI SH
# Run one TEST:UI test for test.sh: mcsh in interactive mode, under a
# pseudo-terminal, with the test file typed in line by line.
# Usage: test-ui.sh TEST_FILE [MCSH_ARGS...]
#    or: test-ui.sh --available
# --available succeeds only if mcsh was built with readline, which
# TEST:UI tests expect; without it the prompt is printed plainly.
# It asks the binary itself, via the "readline" line of mcsh -B.

# Filters the session through cat -v, so non-printing characters appear
# as ^[ (ESC), ^M (CR), etc., and TEST:EXPECT: lines can match them.
# Returns mcsh's exit code, or 124 if the session runs too long.
#
# script(1) provides the pseudo-terminal, so that readline behaves as
# it does for a user:
#   -e             return mcsh's exit code
#   --echo never   do not echo the typed lines, so a TEST:EXPECT: line
#                  cannot match its own input
# TERM and INPUTRC are fixed, as readline's output depends on them.
# The locale is fixed to C, and MCSH_UNICODE and MCSH_UNICODE_FORCE
# are unset, as the prompt depends on them (and on TERM).  A test may
# add or override variables with lines
#   # TEST:UI_ENV: NAME=VALUE ...
# Valgrind is not used: vg.sh pipes mcsh's output, so readline would
# no longer write to the terminal.

if (( ${#*} < 1 )) {
  print "test-ui.sh: Provide a TEST_FILE!"
  return 1
}

# Run from the project root, as test.sh does
THIS=${0:h:A}
cd $THIS/../..

if [[ $1 == --available ]]
then
  # Captured first: with PIPE_FAIL, grep -q exiting early could make
  # mcsh fail with SIGPIPE in a pipeline
  BUILD_INFO=$( bin/mcsh -B )
  grep -qE '^ *readline +yes$' <<< $BUILD_INFO
  return
fi

TEST=$1
shift

# The session's environment: fixed values, then the test's own
UI_ENV=( TERM=xterm INPUTRC=/dev/null LC_ALL=C )
UI_ENV+=( ${=$( sed -n 's/.*TEST:UI_ENV: \(.*\)/\1/p' $TEST )} )

env -u MCSH_UNICODE -u MCSH_UNICODE_FORCE $UI_ENV \
  timeout 30 script -q -e --echo never \
    -c "bin/mcsh ${*}" /dev/null \
  < $TEST | cat -v
