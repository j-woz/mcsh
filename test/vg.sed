s@/usr/libexec/valgrind@VALGRIND@
s@ \(at\|by\) 0x.*: @@
s@puts (ioputs.c.*)@VALGRIND ASSERT FAIL@
s@==[0-9]*==@@
/If you believe this happened/Q
