#!/bin/zsh -f
set -eu

LIMIT=1000000
SKIP_TO=""

zparseopts k:=K n:=N v+=V

if (( ${#K} )) SKIP_TO=${K[2]}
if (( ${#N} )) LIMIT=${N[2]}
VERBOSITY=${#V}

THIS=${${0:h}:A}
cd $THIS/../..
source ./scripts/util.zsh

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

TESTS=( *.mc )
COUNT=$#TESTS
print "COUNT: $COUNT"
INDEX=0
for TEST in $TESTS
do
  if (( COUNT >= LIMIT )) break
  LABEL=${TEST%.mc}
  NUMBER=${TEST[1,4]}
  if (( $#SKIP_TO )) {
    if [[ $NUMBER != <-> ]] \
         abort "test-all.zsh: bad number '$NUMBER' in $TEST"
    if (( $NUMBER == $SKIP_TO )) {
      SKIP_TO="" # stop skipping
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
  ./test.sh -oq $LABEL
  if (( VERBOSITY )) print
  (( ++ INDEX ))
done
print "TEST ALL: OK.  COUNT=$COUNT"
