# TEST:EXPECT: ~/proj/mcsh
# TEST:EXPECT: ~
# TEST:EXPECT: /etc/passwd
# A sibling of $HOME (e.g. $HOME plus "x") is not abbreviated: prints 1
h=(( os env HOME ))
print (( tilde (( + $h "/proj/mcsh" )) ))
print (( tilde $h ))
print (( tilde /etc/passwd ))
print (( string = (( tilde (( + $h "x" )) )) (( + $h "x" )) ))
