#!/bin/sh
set -eu

# AUTO RESET SH
# Remove files created by autoreconf (cf. ./bootstrap)

FILES="aclocal.m4 compile config.h.in configure depcomp install-sh missing ylwrap"

for f in $FILES
do
  if [ -f $f ]
  then
    rm -v $f
  fi
done
