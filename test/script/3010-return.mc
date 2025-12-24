
function f { x } {

  if { $ $x == 0 } {
    return 0
  }
  return 1
}

= s (( f 42 ))
print OUTPUT s $s

= t (( f 0 ))
print OUTPUT t $t
