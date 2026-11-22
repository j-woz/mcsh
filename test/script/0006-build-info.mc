
# mcsh -B prints build information and exits, before any script runs.
# Its last section reports Unicode for the current environment; the
# result depends on the machine, so only the fixed text is checked.
# TEST:ARGS_MCSH: -B
# TEST:EXPECT: build information:
# TEST:EXPECT: configure args
# TEST:EXPECT: C compiler
# TEST:EXPECT: readline
# TEST:EXPECT: Unicode in this environment:
# TEST:EXPECT: MCSH_UNICODE_FORCE
# TEST:EXPECT: locale codeset
# TEST:EXPECT: Unicode is

print should not run
