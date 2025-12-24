
signature n

if { $ $n < 0 } {
  print "error: n too small:" $n
  exit 1
}

function fib { x } {

  if { $ $x == 0 } {
    = r 0
  } \
  if { $ $x == 1 } {
    = r 1
  } \
  or {
    = x1 (( $ $x - 1 ))
    = r1 (( fib $x1 ))
    = x2 (( $ $x - 2 ))
    = r2 (( fib $x2 ))
    = r  (( $ $r1 + $r2 ))
  }
  $ $r
}

= f (( fib $n ))
print FIB: $n -> $f
