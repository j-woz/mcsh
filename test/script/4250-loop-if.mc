= i 0
loop  {
  = r $random
  = d (( $ $r % 3 ))
  print OUTPUT loop $i $r $d
  if { $ $d == 0 } {
    print OUTPUT $d == 0
  } if { $ $d == 1 } {
    print OUTPUT $d == 1
  } or {
    print OUTPUT $d is OTHER
  }
  ++ i
} while { $ $i < 5 }
