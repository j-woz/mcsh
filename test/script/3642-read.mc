
# TEST:PRE writer f.txt HELLO

= fp (( open "f.txt" r ))
= t (( << $fp ))
print $t
close $fp

# TEST:POST grep -q HELLO f.txt
# TEST:POST rm f.txt
