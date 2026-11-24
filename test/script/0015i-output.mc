# TEST:UI
# Each line of output must sit between two prompts.
# TEST:POST: awk -f test/script/ui-interleave.awk test/script/0015i-output.out
print 111
print 222
x=3
print $x $x $x
