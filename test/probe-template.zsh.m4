m4_dnl Template for the bisect probe, used by bisect-test.zsh.
m4_dnl Generate with m4 -P from the project root, passing:
m4_dnl   -DBZ_WT=dir          scratch git worktree (WorkTree)
m4_dnl   -DBZ_CASE=file       copy of the test case under test
m4_dnl   -DBZ_WANT=pass|fail  result git bisect searches for
m4_dnl   -DBZ_AMF=...         "AM_CFLAGS=..." or empty to keep -Werror
m4_dnl   -DBZ_LOGDIR=dir      per-commit build logs
m4_dnl   -DBZ_SKIPLOG=file    SKIP, TIMEOUT and NOTE lines
m4_dnl   -DBZ_STRIP=0|1       1: run a copy with comment lines blanked
m4_dnl BZ = Bisect Zsh, the prefix of these template parameters.
m4_dnl Probe exit codes for git bisect run:
m4_dnl   0 good, 1 bad, 125 skip this commit, 250 abort the bisect.
m4_include(m4/common.m4)m4_dnl
#!/bin/zsh -f
# Probe one commit in the bisect worktree.  Regenerates the build
# system only when configure.ac or Makefile.am changes, for speed.
# Every skip (exit 125) is logged with its reason.
#
# Variable names and what the abbreviations stand for:
#   WT        WorkTree: the scratch git worktree under test
#   CASE      the test CASE file (a copy of the test script)
#   LOGDIR    LOG DIRectory for per-commit build logs
#   SKIPLOG   log of "KIND HASH DATE REASON" lines; KIND is SKIP,
#             TIMEOUT or NOTE
#   HASH      short git commit HASH of the commit under test
#   DATE      author DATE of that commit, as YYYY-MM-DD
#   STAMP     file that remembers SUM from the last autoreconf
#   SUM       checksum (cksum) of configure.ac and Makefile.am
#   AMF       Automake Make Flags: the AM_CFLAGS=... argument
#   STRIP     1: run a copy of CASE with comment lines blanked
#   RUN       the file that mcsh really runs: CASE or its copy
#   RC        Return Code of the mcsh run
#   WANT_BAD  the result bisect calls bad (pass; fail with -F)
#   WANT      1 if CASE expects failure (TEST:FAIL), else 0
#   OK        1 while the run matches what CASE expects
#   E         one Expected output line, from TEST:EXPECT:

WANT_BAD=BZ_WANT
WT=BZ_WT
CASE=BZ_CASE
LOGDIR=BZ_LOGDIR
SKIPLOG=BZ_SKIPLOG
STRIP=BZ_STRIP

cd $WT || exit 125
HASH=$(git rev-parse --short HEAD)
DATE=$(git log -1 --date=short --format=%ad HEAD)
mkdir -p $LOGDIR

# skip REASON: log that this commit cannot be tested; bisect skips it.
skip() {
  print -r -- "SKIP $HASH $DATE $1" >> $SKIPLOG
  exit 125
}

# note REASON: record something about this commit without skipping it.
note() {
  print -r -- "NOTE $HASH $DATE $1" >> $SKIPLOG
}

# step LIMIT NAME CMD...: run CMD under a time limit.  A timeout
# aborts the bisect (exit 250); any other status is returned.
step() {
  local limit=$1 name=$2
  shift 2
  timeout $limit "$@"
  local rc=$?
  if (( rc == 124 )) {
    print -r -- "TIMEOUT $HASH $DATE $name" >> $SKIPLOG
    exit 250
  }
  return $rc
}

# Regenerate the build system only when its inputs changed.
STAMP=.regen-stamp
SUM=$(cat configure.ac Makefile.am 2>/dev/null | cksum)
if [[ ! -x configure || $(cat $STAMP 2>/dev/null) != $SUM ]] {
  step 15 autoreconf autoreconf -i \
    >$LOGDIR/$HASH.autoreconf.log 2>&1 || skip autoreconf
  step 45 configure ./configure \
    >$LOGDIR/$HASH.configure.log 2>&1 || skip configure
  step 30 clean make -s clean >/dev/null 2>&1
  print -r -- $SUM > $STAMP
} elif [[ ! -f Makefile ]] {
  step 45 configure ./configure \
    >$LOGDIR/$HASH.configure.log 2>&1 || skip configure
  step 30 clean make -s clean >/dev/null 2>&1
} else {
  note configure-omitted
}

AMF="BZ_AMF"
step 30 make make -j 4 -s bin/mcsh ${AMF:+"$AMF"} \
  >$LOGDIR/$HASH.make.log 2>&1 || skip make

if grep -q "TEST:SKIP" $CASE; then skip test-skip; fi

# Run CASE.  With STRIP, run a copy whose comment-only lines are
# blank, so a commit whose parser rejects comments does not hide the
# feature under study.  TEST: directives are still read from CASE.
RUN=$CASE
if (( STRIP )) {
  sed 's/^[[:space:]]*#.*$//' $CASE > $CASE.run
  RUN=$CASE.run
}
timeout 10 ./bin/mcsh $RUN >$CASE.out 2>&1
RC=$?
if (( RC == 124 )) {
  print -r -- "TIMEOUT $HASH $DATE test-run" >> $SKIPLOG
  exit 250
}

# OK stays 1 only if the exit status and each TEST:EXPECT: line match.
if grep -q "TEST:FAIL" $CASE; then WANT=1; else WANT=0; fi
OK=1
if (( WANT )) {
  (( RC == 0 )) && OK=0
} else {
  (( RC != 0 )) && OK=0
}

sed -n "s/.*TEST:EXPECT: //p" $CASE > $CASE.exp
while IFS= read -r E; do
  grep -qF -- "$E" $CASE.out || OK=0
done < $CASE.exp

# Exit 1 (bad) when the result is the one being searched for.
if [ "$WANT_BAD" = pass ]; then
  [ $OK -eq 1 ] && exit 1 || exit 0
else
  [ $OK -eq 0 ] && exit 1 || exit 0
fi
