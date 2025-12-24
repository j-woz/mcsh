#!/bin/zsh -f
set -eu
setopt PIPE_FAIL

LINE_LIMIT=100

# Defaults:
LEAK_CHECK="no"
REDIRECT=0
VERBOSITY=1

LEAK=""
# -F : fail on unknown flags
zparseopts -D -E -F l=LEAK L=LEAK o=R q=QUIET v=V

if (( ${#QUIET} )) VERBOSITY=0
if (( ${#R}     )) REDIRECT=1
if (( ${#V}     )) VERBOSITY=2

if (( ${#*} < 1 )) {
  print "test.sh: Provide a test LABEL!"
  return 1
}

LABEL=$1
shift

if (( ${#LEAK} )) {
  if [[ ${LEAK} == "-l" ]] {
    # summary does not seem to work: 2021-12-05
    LEAK_CHECK="summary"
  } else {
    LEAK_CHECK="yes"
  }
}

CMD=$0
THIS=${${CMD:h}:a}

cd $THIS/../..

TEST=test/script/$LABEL.mc
if [[ ! -f $TEST ]] {
  print "test.sh: does not exist: $TEST"
  return 1
}

if (( ${MAKE:-1} )) {
  # This line limit for the compiler is probably ok
  if make bin/mcsh |& head -${LINE_LIMIT}
  then
    print "MAKE OK."
  else
    print "test.sh: MAKE: FAILED!"
    return 1
  fi
}

if grep -q "TEST:SKIP" $TEST
then
  return
fi

TEST_FAIL=0
if grep -q "TEST:FAIL" $TEST
then
  if (( VERBOSITY >= 2 )) print "TEST_FAIL=1"
  TEST_FAIL=1
fi

# Arguments passed to mcsh (before the user script)
TEST_ARGS_MCSH=""
if grep -q "TEST:ARGS_MCSH" $TEST
then
  TEST_ARGS_MCSH=$( sed -n 's/.*TEST:ARGS_MCSH: \(.*\)/\1/p' $TEST )
  # print "TEST_ARGS_MCSH: $TEST_ARGS_MCSH"
fi

# Arguments passed to mcsh (before the user script)
TEST_ARGS_SCRIPT=""
if grep -q "TEST:ARGS_SCRIPT" $TEST
then
  TEST_ARGS_SCRIPT=$( sed -n 's/.*TEST:ARGS_SCRIPT: \(.*\)/\1/p' $TEST )
  # print "TEST_ARGS_SCRIPT: $TEST_ARGS_SCRIPT"
fi

CODE=0
SUCCESS=0

# Possibly turn on valgrind:
export VALGRIND=${VALGRIND:-1}
if [[ $OS == "Cygwin" ]] VALGRIND=0
if (( VALGRIND )) {
  V_FLAG=""
  if (( VERBOSITY >= 2 )) V_FLAG="-v"
  VG=( $THIS/../vg.sh $V_FLAG --leak-check=$LEAK_CHECK )
  # # --tool=exp-sgcheck
} else {
  VG=""
}

TEST_OUTPUT=test/script/$LABEL.out
if (( REDIRECT )) {
  alias -g output=">& $TEST_OUTPUT"
} else {
  alias -g output=""
}

if $VG bin/mcsh $TEST_ARGS_MCSH $TEST $TEST_ARGS_SCRIPT ${*} output
then
  if (( ! TEST_FAIL )) SUCCESS=1
else
  CODE=$?
  if ((   TEST_FAIL )) SUCCESS=1
fi

if (( SUCCESS && REDIRECT )) {
  if grep -q "TEST:EXPECT:" $TEST
  then
    TEST_EXPECT=$( sed -n 's/.*TEST:EXPECT: \(.*\)/\1/p' $TEST )
    EXPECTEDS=( ${(f)TEST_EXPECT} )
    print "EXPECTED TOTAL: $#EXPECTEDS"
    COUNT=1
    for EXPECTED in $EXPECTEDS
    do
      if ! grep -F -q "$EXPECTED" $TEST_OUTPUT
      then
        print "test.sh: expected output missing: '$EXPECTED'"
        SUCCESS=0
        break
      fi
      if (( SUCCESS && VERBOSITY > 1 )) print "TEST OUTPUT $COUNT OK."
      (( COUNT ++ ))
    done
  fi
}

if (( ! SUCCESS )) {
  echo "TEST CODE:  $CODE"
  echo "TEST FAILED: $LABEL"
  return 1
}

if (( VERBOSITY > 0 )) print "TEST OK."
