#!/bin/zsh -f
set -eu

SKIP_TO=""
zparseopts k:=K

if (( ${#K} )) {
  SKIP_TO=${K[2]}
}

THIS=${${0:h}:A}
cd $THIS/../..

if (( ${MAKE:-1} )) {
  make bin/mcc |& head -25
  if (( ${pipestatus[1]} != 0 )) {
    return 1
  }
  print "MAKE OK."
}

export MAKE=0

cd $THIS
pwd

TESTS=( $( print *.mcc ) )
for TEST in $TESTS
do
  LABEL=${TEST%.mcc}
  NUMBER=${TEST[1,4]}
  if (( ${#SKIP_TO} )) {
    if (( $NUMBER == $SKIP_TO )) {
      SKIP_TO="" # stop skipping
    } else {
      continue
    }
  }
  print "TEST: $LABEL"
  print "...."
  cat $THIS/$TEST
  print "...."
  print
  ./test.sh $LABEL
  print
done
print "TEST ALL: OK."
