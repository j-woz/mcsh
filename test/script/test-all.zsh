#!/bin/zsh -f
set -eu

LIMIT=1000000
JUMP_TO=""
SKIP_TO=""

zparseopts h=H j:=J k:=K n:=N v+=V

if (( ${#H} )) {
  print "Usage: test-all.zsh " \
        "[-h] [-j NUMBER] [-k NUMBER] [-n LIMIT] [-v ...]"
  print "  -h         Show this help"
  print "  -j NUMBER  Jump to test NUMBER"
  print "  -k SKIP    SKIP NUMBER of tests"
  print "  -n LIMIT   Stop after LIMIT tests"
  print "  -v         Increase verbosity (repeatable)"
  print "  -vvv       Turns on set -x"
  return
}

if (( ${#J} )) JUMP_TO=${J[2]}
if (( ${#K} )) SKIP_TO=${K[2]}
if (( ${#N} )) LIMIT=${N[2]}
VERBOSITY=${#V}

THIS=${${0:h}:A}
cd $THIS/../..
source ./scripts/util.zsh

if (( VERBOSITY >= 3 )) set -x

if (( $#JUMP_TO > 0 )) && [[ $JUMP_TO != <-> ]] \
                            abort "Bad JUMP_TO: '$JUMP_TO'"
if (( $#SKIP_TO > 0 )) && [[ $SKIP_TO != <-> ]] \
                            abort "Bad SKIP_TO: '$SKIP_TO'"

if (( ${MAKE:-1} )) {
  make bin/mcsh
  if (( ${pipestatus[1]} != 0 )) {
    print "MAKE: FAILED!"
    return 1
  }
  print "MAKE OK."
}

export MAKE=0

cd $THIS

if (( VERBOSITY == 0 )) {
  TEST_VERBOSITY="-q"
} else {
  TEST_VERBOSITY=( $V )
}

TESTS=( [0-9]*.mc )
COUNT=$#TESTS
print "COUNT: $COUNT"
if (( ${#JUMP_TO} )) print "JUMP:  $JUMP_TO"
if (( ${#SKIP_TO} )) print "SKIP:  $SKIP_TO"
INDEX=0
for TEST in $TESTS
do
  if (( INDEX >= LIMIT )) break
  (( ++ INDEX ))

  LABEL=${TEST%.mc}
  NUMBER=${TEST[1,4]}

  if [[ $NUMBER != <-> ]] \
    abort "test-all.zsh: bad number '$NUMBER' in $TEST"

  if (( ${#SKIP_TO} )) {
    if (( $INDEX > SKIP_TO )) {
      SKIP_TO="" # stop skipping
    } else {
      continue
    }
  }
  if (( ${#JUMP_TO} )) {
    if (( NUMBER == JUMP_TO )) {
      JUMP_TO="" # stop skipping
    } else {
      continue
    }
  }
  printf "TEST %3i: %s\n" $INDEX $LABEL
  if (( VERBOSITY )) {
    print "...."
    cat $THIS/$TEST
    print "...."
    print
  }

  ./test.sh -o $TEST_VERBOSITY $LABEL
  if (( VERBOSITY )) print
done
print "TEST ALL: OK.  COUNT=$COUNT"
