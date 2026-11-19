#!/bin/zsh -f
# Find the commit that changed the result of one test, using
# git bisect.
#
# Default: find the first commit where the test SUCCEEDS.
#   START is older and fails; STOP is newer and succeeds.
#   START defaults to the oldest commit in the last year.
# With -F: find the first commit where the test FAILS.
# With -C: run the test with its comment lines blanked, so a commit
#   whose parser rejects comments does not mask the feature under
#   study.  TEST: directives are still read from the original file.
#
# Each step regenerates the build and runs the test in a scratch
# worktree under bisect-tmp/.  A commit that cannot be tested is
# skipped (exit 125): its reason is logged to skips.log and its
# build output is kept in build-logs/.
#
# Variable names and what the abbreviations stand for:
#   H S E T F K W C  option arrays filled by zparseopts, named for
#                    the flag: Help, Start, End (the stop commit),
#                    Test, First-failure, Keep, Werror, Comment-strip
#   LABEL     the test name, e.g. 3207-attach-call
#   THIS      the directory holding this script
#   ROOT      the project root, the parent of THIS
#   CASE      the test CASE file under test/script
#   START     older commit; STOP is the newer commit
#   WANT_BAD  the result git bisect calls bad (pass; fail with -F)
#   SCRATCH   directory for the worktree, probe and logs
#   WT        WorkTree: the scratch git worktree
#   LOGDIR    LOG DIRectory for per-commit build logs
#   SKIPLOG   log of "KIND HASH DATE REASON" lines; KIND is SKIP,
#             TIMEOUT or NOTE; DATE is the commit's YYYY-MM-DD
#   START_DATE, STOP_DATE  the YYYY-MM-DD dates of START and STOP
#   AMF_ARG   Automake Make Flags: an AM_CFLAGS=... make argument
#   STRIP     1 when -C is given: blank comment lines in the run copy
#   PROBE     the generated probe script that bisect runs
#   S_RC      probe Return Code at START (want 0: not bad)
#   E_RC      probe Return Code at STOP, the End (want 1: bad)
#   RC        Return Code of git bisect run

set -eu

H=() S=() E=() T=() F=() K=() W=() C=()
zparseopts -D -E -F h=H t:=T s:=S e:=E F=F W=W k=K C=C

if (( ${#H} )) {
  print "Usage: bisect-test.zsh [-h] [-F] [-C] [-W] [-k]" \
        "-t LABEL [-s START] [-e STOP]"
  print "  -t LABEL  test in test/script, e.g. 3207-attach-call"
  print "  -s START  older commit (default: oldest in the last year)"
  print "  -e STOP   newer commit (default: HEAD)"
  print "  -F        search for the first failure, not success"
  print "  -C        blank comment lines in the copy that is run"
  print "  -W        keep -Werror (default drops it, for old code)"
  print "  -k        keep the worktree and scratch files"
  print "  -h        show this help"
  print "Commits that cannot be tested are skipped; see skips.log."
  return
}

if (( ! ${#T} )) {
  print "bisect-test.zsh: need -t LABEL"
  return 2
}

LABEL=${T[2]}

THIS=${${0:h}:A}
ROOT=${THIS:h}
cd $ROOT

STOP=HEAD
if (( ${#E} )) STOP=${E[2]}
STOP=$(git rev-parse --verify "${STOP}^{commit}")
START=
if (( ${#S} )) START=${S[2]}
if [[ -n $START ]] {
  START=$(git rev-parse --verify "${START}^{commit}")
}

CASE=test/script/$LABEL.mc
if [[ ! -f $CASE ]] {
  print "bisect-test.zsh: no such test: $CASE"
  return 2
}

if [[ -z $START ]] {
  START=$(git rev-list --reverse --since="1 year ago" $STOP | head -1)
  if [[ -z $START ]] {
    print "bisect-test.zsh: no commit in the last year"
    return 2
  }
}

if ! git merge-base --is-ancestor $START $STOP 2>/dev/null
then
  print "bisect-test.zsh: START must be an ancestor of STOP"
  return 2
fi

if (( ${#F} )) { WANT_BAD=fail } else { WANT_BAD=pass }

STRIP=0
if (( ${#C} )) STRIP=1

SCRATCH=$ROOT/bisect-tmp
WT=$SCRATCH/wt
LOGDIR=$SCRATCH/build-logs
SKIPLOG=$SCRATCH/skips.log
mkdir -p $SCRATCH $LOGDIR
rm -f $SKIPLOG
cp $CASE $SCRATCH/case.mc

if [[ ! -d $WT ]] {
  git worktree add --detach $WT $STOP >/dev/null
}

AMF_ARG=""
if (( ! ${#W} )) {
  AMF_ARG="AM_CFLAGS=-std=gnu2x -Wall -Wno-unused-function -g -O0"
}

PROBE=$SCRATCH/probe.zsh
m4 -P -DBZ_WT=$WT -DBZ_CASE=$SCRATCH/case.mc \
   -DBZ_WANT=$WANT_BAD -DBZ_LOGDIR=$LOGDIR \
   -DBZ_SKIPLOG=$SKIPLOG -DBZ_STRIP=$STRIP \
   "-DBZ_AMF=$AMF_ARG" \
   test/probe-template.zsh.m4 > $PROBE
chmod +x $PROBE

START_DATE=$(git log -1 --date=short --format=%ad $START)
STOP_DATE=$(git log -1 --date=short --format=%ad $STOP)
print "bisect-test.zsh: test $LABEL"
print "  searching for first $WANT_BAD between"
print "    $START ($START_DATE) and"
print "    $STOP ($STOP_DATE)"
if (( STRIP )) print "  comment lines blanked in the run copy"

# Endpoints must bracket the change: START is not "bad", STOP is.
# A probe exit of 0 is "good" and 1 is "bad", as in git bisect run.
cd $WT
set +e
git checkout -q --detach $START && $PROBE >/dev/null 2>&1
S_RC=$?
git checkout -q --detach $STOP && $PROBE >/dev/null 2>&1
E_RC=$?
set -e
if (( S_RC != 0 || E_RC != 1 )) {
  print "bisect-test.zsh: endpoints do not bracket the change"
  print "  START probe=$S_RC (want 0), STOP probe=$E_RC (want 1)"
  cd $ROOT
  git worktree remove --force $WT >/dev/null 2>&1 || true
  return 2
}
print "  endpoints verified"
git bisect reset >/dev/null 2>&1 || true
git bisect start $STOP $START >/dev/null
set +e
git bisect run $PROBE >$SCRATCH/run.log 2>&1
RC=$?
set -e

print -r -- "--- bisect result ---"
grep -E 'first bad commit|could be any of' -A 12 $SCRATCH/run.log \
  | grep -v '^Bisecting' || true
if (( RC != 0 )) print "bisect exited $RC; see $SCRATCH/run.log"

git bisect reset >/dev/null 2>&1 || true

print -r -- "--- skips by reason ---"
if [[ -s $SKIPLOG ]] {
  awk '$1 == "SKIP" || $1 == "TIMEOUT" { print $1, $4 }' $SKIPLOG \
    | sort | uniq -c
} else {
  print "none"
}

print -r -- "--- notes ---"
if [[ -s $SKIPLOG ]] {
  awk '$1 == "NOTE" { print $4 }' $SKIPLOG | sort | uniq -c
}

if (( ! ${#K} )) {
  cd $ROOT
  git worktree remove --force $WT >/dev/null 2>&1 || true
  rm -rf $SCRATCH
} else {
  print "kept: $SCRATCH (skips.log, build-logs/, run.log)"
}
return 0
