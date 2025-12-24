m4_dnl Generate README.adoc with ./mk-doc.sh
m4_dnl Prevents direct editing of generated README.adoc
m4_dnl          via write-protection
m4_dnl This include must happen early so we get COMMENT()
m4_dnl Note we change m4 quoting so that backquotes work!
m4_include(m4/common.m4)

= MCSH Guide

== Invocation

For help, use `mcsh -h`.

=== Execute a simple command string

----
$ mcsh -c "print HELLO"
----

=== Execute a script

----
$ mcsh V1=text1 V2=text2 script.mc arg1 arg2 arg3
----

=== Execute interactively

----
$ mcsh
----

== Basic built-ins

=== Output

----
print HELLO
----

== Basic variables

----
\= x 3
print $x
----

== Grammar

=== Plain commands

=== Conditionals

=== Loops

=== Defining functions

COMMENT(
Local Variables:
mode: doc;
End:
)
