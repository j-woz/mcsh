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
zparseopts -D -E -F h=H l=LEAK L=LEAK o=R q=QUIET v+=V

if (( ${#H} )) {
  print "Usage: test.sh [-h] [-l|-L] [-o] [-q] [-v] LABEL"
  print "  -h  Show this help"
  print "  -l  Enable valgrind leak check (summary)"
  print "  -L  Enable valgrind leak check (full)"
  print "  -o  Redirect output to .out file"
  print "  -q  Quiet   (verbosity 0)"
  print "  -v    Increase verbosity (repeatable)"
  print "  -vvv  Turns on set -x"
  return
}

if (( ${#QUIET} )) VERBOSITY=0
if (( ${#R}     )) REDIRECT=1
if (( ${#V}     )) VERBOSITY=${#V}

if (( VERBOSITY >= 3 )) set -x

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
  if (( VERBOSITY >= 2 )) print "TEST_SKIP: $TEST"
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
if grep -q "TEST:ARGS_MCSH:" $TEST
then
  TEST_ARGS_MCSH=$( sed -n 's/.*TEST:ARGS_MCSH: \(.*\)/\1/p' $TEST )
  if (( VERBOSITY >= 2 )) print "TEST_ARGS_MCSH $TEST_ARGS_MCSH"
fi

# Arguments passed to the user script
TEST_ARGS_SCRIPT=""
if grep -q "TEST:ARGS_SCRIPT:" $TEST
then
  TEST_ARGS_SCRIPT=$( sed -n 's/.*TEST:ARGS_SCRIPT: \(.*\)/\1/p' $TEST )
  if (( VERBOSITY >= 2 )) print "TEST_ARGS_SCRIPT: $TEST_ARGS_SCRIPT"
fi

CODE=0
SUCCESS=0

# Possibly turn on valgrind:
export VALGRIND=${VALGRIND:-1}
# Disable valgrind on Cygwin:
if [[ $( uname ) == CYGWIN* ]] VALGRIND=0
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

if grep -q "TEST:PRE:" $TEST
then
  CMD_PRE=$( $THIS/test-text.awk -v TOKEN="TEST:PRE:" < $TEST )
  if (( VERBOSITY >= 2 )) print "TEST:PRE: '${CMD_PRE}'"
  $=CMD_PRE
fi

if $VG bin/mcsh $TEST_ARGS_MCSH $TEST $=TEST_ARGS_SCRIPT ${*} output
then
  if (( ! TEST_FAIL )) SUCCESS=1
else
  CODE=$?
  if ((   TEST_FAIL )) SUCCESS=1
fi

if (( SUCCESS && REDIRECT )) {
  if grep -q "TEST:EXPECT:" $TEST
  then
    # NOTE: TEST:EXPECT: output is only tested if redirecting!
    TEST_EXPECT=$( sed -n 's/.*TEST:EXPECT: \(.*\)/\1/p' $TEST )
    EXPECTEDS=( ${(f)TEST_EXPECT} )
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

# Execute each TEST:POST:
if grep -q "TEST:POST:" $TEST
then
  grep "TEST:POST:" $TEST | while read LINE
  do
    CMD_POST=$( echo $LINE | $THIS/test-text.awk -v TOKEN="TEST:POST:" )
    if (( VERBOSITY >= 2 )) print "test.sh: TEST:POST: '${CMD_POST}'"
    if ! $=CMD_POST
    then
      print "test.sh: TEST:POST failed!"
      return 1
    fi
  done
fi

if (( ! SUCCESS )) {
  echo "TEST CODE:   $CODE"
  echo "TEST FAILED: $LABEL"
  return 1
}

if (( VERBOSITY > 0 )) print "TEST OK."
