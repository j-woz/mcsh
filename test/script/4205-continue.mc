= x 0
# continue

loop {
  = x (( $ $x + 1 ))
  continue
  exit 1
} while { $ $x < 3 }

# Local Variables:
# mode: sh
# End:
