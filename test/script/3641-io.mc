
= fp (( open "f.txt" w ))
>> $fp HELLO
close $fp

# TEST:POST grep -q HELLO f.txt
# TEST:POST rm f.txt
