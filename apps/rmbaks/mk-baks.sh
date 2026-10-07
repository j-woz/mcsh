#!/bin/zsh
set -eu

THIS=${0:h:A}
cd $THIS
mkdir -pv data
cd data

FILES=( f g h )

touch $FILES

for F in $FILES
do
  while (( RANDOM % 4 != 0 ))
  do
    cp -v --backup=numbered $F $F.bak
  done
done
