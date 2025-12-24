if { $ 0 } {
  exit 1
} if { $ 0 + 1 } {
  print OK
  exit 0
}
exit 1

# Local Variables:
# mode: sh
# End:
