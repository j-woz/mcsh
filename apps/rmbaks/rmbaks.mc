#!/usr/bin/env mcsh

signature DIR
print DIR: $DIR
os cd $DIR
= files (( list ))
+ $files *

= map (( table ))

foreach file $files {
  print file: $file
  # Must have 2 tildes with number between them:
  if { $ $file/.*~.*~ } {
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
      = new (( $ $k - ( $random % ( $k - $last ) ) ))
      print new is $new
      os mv  (( + $b ~ $k ~ )) (( + $b ~ $new ~ ))
      = last $k
    }
  }
}
# print $PWD
# print $files
