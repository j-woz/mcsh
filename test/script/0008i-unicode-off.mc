# TEST:UI
# TEST:UI_ENV: LC_ALL=C.UTF-8 MCSH_UNICODE=0
# MCSH_UNICODE=0 turns Unicode off: the plain prompt, even in a UTF-8
# locale.
# TEST:EXPECT: ^[[?2004hmcsh> ^[[?2004l^M^M
exit
