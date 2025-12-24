#!/bin/zsh -f
set -eu

TEST=test/util/$1.x

make $TEST

THIS=$( readlink --canonicalize $( dirname $0 ) )

CODE=0
SUCCESS=0
export VALGRIND=${VALGRIND:-1}
if (( VALGRIND )) {
  VG=( $THIS/../vg.sh  --leak-check=no ) # # --tool=exp-sgcheck
} else {
  VG=""
}

echo "TEST: $TEST"
if ! $VG $TEST
then
  print "FAILED!"
  return 1
fi
print "OK"
