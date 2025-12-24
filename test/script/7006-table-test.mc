
= T (( table ))
print "TEST_RESULT: size:" $#T
print "TEST_RESULT: 0:"    $+T[x]

+ $T x 42

print "TEST_RESULT: 1:"  $+T[x]
print "TEST_RESULT: 42:" $T[x]
