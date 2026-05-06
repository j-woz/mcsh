#!/usr/bin/awk -f

# Find text after given TOKEN

$0 ~ TOKEN {
  # Assume space after TOKEN: +1
  start = index($0, TOKEN) + length(TOKEN) + 1
  print(substr($0, start))
  exit()
}
