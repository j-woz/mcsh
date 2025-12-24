#!/bin/zsh
set -eu
setopt PIPE_FAIL

# VG SH
# Valgrind runner
# Cleans the valgrind output with vg.sed
# May provide -l or --leak-check - this will override the default
#     specified below, which is --leak-check=no

# Like $( dirname $( readlink --canonicalize $0 ) ) :
THIS=${0:h:A}
export VALGRIND=${VALGRIND:-1}

LEAK=( --leak-check=no )

# Also allows --leak-check=X arguments from test.sh
zparseopts -D -E l=L v=VERBOSE
if (( ${#L} )) LEAK=( --leak-check=yes )

if (( ${#} == 0 )) {
  print "vg.sh: Provide a command to run!"
  exit 1
}

if (( ${#VERBOSE} )) set -x
valgrind $LEAK ${*} |& sed -f $THIS/vg.sed -e s@$PWD/@@
