# TEST:UI
# TEST:UI_ENV: LC_ALL=C.UTF-8 TERM=dumb
# TERM=dumb (e.g. an Emacs shell buffer): plain prompt.  readline
# sends no escape codes to a dumb terminal.
# TEST:EXPECT: mcsh> 
exit
