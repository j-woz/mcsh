#!/usr/bin/env mcsh

signature DIR
print DIR: $DIR
os cd $DIR
= files (( list ))
+ $files *

= map (( table ))

foreach file $files {
  print file: $file
  # Only numbered backups: a tilde, one or more digits, a trailing tilde
  # at end of name (e.g. foo.~12~).  Uses POSIX basic regex, so the
  # one-or-more is [0-9][0-9]* rather than [0-9]+.
  if { $ $file/~[0-9][0-9]*~$ } {
    print rmbaks MATCH
    = i (( find $file ~ ))
    # print i: $i
    = base $file[0:$i]
    print rmbaks file: $base
    = j (( $ $i + 1 ))
    # print rmbaks j: $j
    = n (( $ $#file - 1 ))
    # print n: $n
    = c $file[$j:$n]
    # print n: $n
    print rmbaks index: $c
    # print test $+map[$file]
    if { $ $+map[$base] } {
      print rmbaks found
      = L $map[$base]
      + $L (( as int $c ))
      print $L
    } or {
      print rmbaks not found
      = L (( list ))
      + $L (( as int $c ))
      + $map $base $L
    }
  }
  print _______________
}

print size: $#map

= bases $@map

print bases: $bases

foreach b $bases {
  print rmbaks b: $b
  = L $map[$b]
  # A lone backup is left alone: nothing to thin or renumber.  This also
  # avoids the k-$last == 0 divide in the renumber branch for the first
  # element.
  if { $ $#L > 1 } {
    sort $L
    print $L
    = last 0
    print last $last
    for { = i 0 } { $ $i < $#L } { ++ i } {
      print i is $i
      = k $L[$i]
      print k is $k
      if { $ $random % 2 == 0 } {
        print delete $k
        os rm (( + $b ~ $k ~ ))
      } or {
        # Only renumber when $k > $last (last starts at 0).
        # When they are equal (the first element at ~0~ (which is not GNU))
        # $k - $last is 0, which would divide by zero;
        # just leave that backup in place.
        if { $ $k > $last } {
          = new (( $ $k - ( $random % ( $k - $last ) ) ))
          print new is $new
          os mv  (( + $b ~ $k ~ )) (( + $b ~ $new ~ ))
        }
        = last $k
      }
    }
  }
}
# print $PWD
# print $files
