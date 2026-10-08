#!/usr/bin/env mcsh

# Create some files under data/ and make a random number of numbered
# backups of each.  MCSH port of mk-baks.sh.

# $this is the directory containing this script (cf. zsh ${0:h:A}).
os cd $this
os mkdir data
os cd data

= files (( list ))
+ $files f g h

foreach f $files {
  ! touch $f
  # Keep making numbered backups until random % 4 == 0.
  loop while { $ $random % 4 != 0 } {
    ! cp -v "--backup=numbered" $f (( + $f ".bak" ))
  }
}
