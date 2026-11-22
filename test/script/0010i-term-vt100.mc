# TEST:UI
# TEST:UI_ENV: LC_ALL=C.UTF-8 TERM=vt100
# DEC VT terminals cannot show the shamrock: plain prompt.
# TEST:EXPECT: ^[[?2004hmcsh> ^[[?2004l^M^M
exit
