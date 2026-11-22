# TEST:UI
# TEST:UI_ENV: LC_ALL=C.UTF-8
# In a UTF-8 locale the default prompt ends with a shamrock, U+2618,
# whose UTF-8 bytes E2 98 98 cat -v shows as M-bM-^XM-^X, then two
# spaces.
# TEST:EXPECT: ^[[?2004hmcshM-bM-^XM-^X  ^[[?2004l^M^M
exit
