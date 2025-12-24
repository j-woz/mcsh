
show()
{
  local V
  for V in ${*}
  do
    print "$V: ${(P)V}"
  done
}

abort()
{
  print "abort:" ${*}
  exit 1
}
