#!/bin/zsh

zparseopts -D -E n:=N

make $* 2>&1 | sed "s@$PWD/@@;s@/usr/bin/@@;s@undefined reference@un.ref.@" | \
  if (( ${#N} )) {
    head ${N}
  } else {
    cat
  } | nl
