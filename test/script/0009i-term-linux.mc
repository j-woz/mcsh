# TEST:UI
# TEST:UI_ENV: LC_ALL=C.UTF-8 TERM=linux
# The Linux console usually cannot show the shamrock: plain prompt.
# TEST:EXPECT: ^[[?2004hmcsh> ^[[?2004l^M^M
exit
