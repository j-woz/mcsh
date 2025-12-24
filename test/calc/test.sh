#!/bin/zsh -f
set -eu

if [[ ${#*} != 1 ]] {
  print "calc/test.sh: Provide a test LABEL!"
  return 1
}

LABEL=$1

CMD=$0
THIS=${CMD:h}
THIS=${THIS:a}
cd $THIS/../..

TEST=test/calc/$LABEL.mcc
OUTPUT=test/calc/$LABEL.out

if (( ${MAKE:-1} )) {
  make bin/mcc
  # |& head -25 # why? 2021-05-22
  if (( ${pipestatus[1]} != 0 )) {
    print "calc/test.sh: make failed!"
    return 1
  }
  print "MAKE OK."
}

print "TEST: $LABEL"
if [[ ! -f $TEST ]] {
  print "does not exist: $TEST"
  return 1
}

if grep -q "TEST:SKIP" $TEST
then
  return
fi

TEST_FAIL=0
if grep -q "TEST:FAIL" $TEST
then
  echo "expecting test to fail..."
  TEST_FAIL=1
fi

vg()
{
  $THIS/../vg.sh --leak-check=no $* |& \
    awk -f $THIS/shorten.awk  |  \
      tee $OUTPUT
  # Return exit code from test, not awk, not tee:
  return ${pipestatus[1]}
}

SUCCESS=0
export VALGRIND=1
if vg bin/mcc < $TEST
then
  if (( ! TEST_FAIL )) SUCCESS=1
else
  if ((   TEST_FAIL )) SUCCESS=1
fi

if (( ! SUCCESS )) {
  echo "TEST CRASHED: $LABEL"
  return 1
}

if grep -q ANSWER: $TEST
then
  ANSWER=$( sed -n '/ANSWER:/{s/.*ANSWER:\(.*\)/\1/;p;}' $TEST )
  print "ANSWER='$ANSWER'"
  if ! grep -q "RESULT: $ANSWER" $OUTPUT
  then
    print "ANSWER NOT FOUND"
    return 1
  fi
fi

print "TEST OK."
