print hi

if { $ 1 } {
  print 101
  exit 0
} or {
  print 103
  exit 103
}
print 102
exit 102
