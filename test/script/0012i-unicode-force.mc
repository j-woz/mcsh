# TEST:UI
# TEST:UI_ENV: LC_ALL=C TERM=dumb MCSH_UNICODE_FORCE=1
# MCSH_UNICODE_FORCE=1 turns Unicode on despite TERM=dumb and the C
# locale (it switches to a UTF-8 locale if it can).
# TEST:EXPECT: mcshM-bM-^XM-^X  
exit
