# TEST:UI
# A user-defined function prompt replaces the default prompt.
# The prompt is " " + PWD + shamrock + two spaces,
# so the test checks only the end of the first prompt,
# which is the shamrock and two spaces.
# TEST:EXPECT: M-bM-^XM-^X  ^[[?2004l^M^M
# TEST:EXPECT: 111^M
# TEST:EXPECT: 222^M
# Interactive input is read a line at a time, so prompt function is
# all on one line.
function prompt { } { return (( + " " $PWD "☘  " )) }
print 111
print 222
exit
