= x 0
loop {
  = x (( $ $x + 1 ))
  print HELLO $x
} until { $ $x == 3 }
