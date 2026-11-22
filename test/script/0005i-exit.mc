# TEST:UI
# The default prompt is "mcsh> ".  In a terminal, readline turns on
# bracketed paste (ESC [ ? 2004 h) just before the prompt and turns it
# off (ESC [ ? 2004 l, then CR) when a line is entered.  The last ^M is
# the CR of the newline that ends the entered line.
# TEST:EXPECT: ^[[?2004hmcsh> ^[[?2004l^M^M
exit
