m4_dnl Generate README.adoc with ./mk-doc.sh
m4_dnl Prevents direct editing of generated README.adoc
m4_dnl          via write-protection
m4_dnl This include must happen early so we get the doc macros
m4_dnl Note we change m4 quoting so that backquotes work!
m4_include(m4/common.m4)

= MCSH: The Macro Shell
:toc: left
:toclevels: 3
:sectnums:

MCSH ("macro shell", command `mcsh`) is a small scripting language with a
shell-like command syntax.  Notable improvements over POSIX-like shells include:

. Ability to execute macro-like expressions
. First-class data structures (lists and tables)
. A unified and clean argument-handling system shared between command-line arguments and function calls
. Rich control structures, modules, types, and exceptions.

== Invocation

For help, use `mcsh -h`.

=== Run a command string

----
$ mcsh -c "print HELLO"
----

=== Run a script

A script is run by naming it.  Any `KEY=VALUE` pairs before the script name
become global variables; anything after the script name is passed to the
script as its arguments.

----
$ mcsh V1=text1 V2=text2 script.mc arg1 arg2 arg3
----

Inside the script, declare each `KEY=VALUE` name with `global` to read it:

----
$ mcsh x=42 script.mc

global x
print $x            # 42
----

Script flags that begin with `-` are passed through to the script untouched
(see <<_command_line_arguments,Command-line arguments>>):

----
$ mcsh script.mc -v -o out.txt input.txt
----

=== Executable scripts

A script whose first line is `#!/usr/bin/env mcsh` and which has its
execute bit set can be run directly, if `mcsh` is on the `PATH`.  Its
arguments are bound with `signature` (see <<_command_line_arguments,Command-line arguments>>):

----
#!/usr/bin/env mcsh

signature DIR
print DIR: $DIR
----

See <<signature,the signature builtin>> for more information.

=== Run interactively

----
$ mcsh
mcsh☘  print hello
hello
----

When Unicode is enabled, the default prompt is `mcsh` and a shamrock
(☘, U+2618) followed by two spaces.  Otherwise it is `mcsh>` followed
by one space:

----
$ mcsh
mcsh> print hello
hello
----

The calculator `mcc` uses the same rule for its prompt: `mcc☘` and two
spaces, or `mcc>` and one space.

== Lexical structure

The syntax for MCSH is very similar to other shell-like languages.

=== Statements

A program is a sequence of statements.  Statements are separated by
newlines or by a semicolon:

----
print ab ; print bc
----

A long statement may be continued on the next line with a trailing
backslash.

=== Comments

A `#` begins a comment that runs to the end of the line.  C-style block
comments `/* ... */` are also accepted:

----
# This is a line comment
/* C comment */
print HI
/** C doc comment */
----

=== Words and strings

A command is a word followed by whitespace-separated argument words.  A
word may be quoted with double quotes to include spaces or to force it to
be treated as a string literal:

----
print "ab cd"           # one argument with a space
s="a    b"            # internal spaces preserved
e=""                  # the empty string
----

A quoted literal is always a string: it is never coerced to a number, and
it is rejected as an operand to arithmetic (see <<_expressions,Expressions>>).

[[subcommand]]
=== Expression evaluation

Commands are sequences of words (as in the POSIX shell), but always return a value.  This value is obtained for further evaluation with the _subcommand expression_, written `+(( command ))+` .  For example:

----
first="George"
last="Washington"
# String concat builtin:
fullname=(( + $first " " $last ))
print $fullname          # George Washington
----

== Values and types

MCSH has these value types, reported by the `type` builtin:

[cols="1,3"]
|===
| Type | Notes

| `string`   | Text.  The default type for a bare word that is not numeric.
| `int`      | 64-bit signed integer.
| `float`    | Double-precision floating point (written with a `.`, e.g. `2.3`).
| `bool`     | `true` or `false` (case-insensitive); prints as `true`/`false`.
| `list`     | Ordered sequence, created with `list`.
| `table`    | String-keyed map, created with `table`.
| `function` | A defined function, inplace, or macro.
| `null`     | The absent value.
|===

A bare word is classified when it is evaluated: `true`/`false` become
`bool`, an integer literal becomes `int`, a number with a decimal point
becomes `float`, and anything else is a `string`.

----
b=true
print (( type $b ))      # bool
i=5
print (( type $i ))      # int
----

In arithmetic a `bool` coerces to `1` (true) or `0` (false).

== Variables

=== Assignment

Note that, like other shells, MCSH is sensitive to spacing in assignment operations!

`=` assigns a value to a variable.  `$name` reads it.

----
x=3
print $x
----

The command-attached form `name=value` also assigns:

----
x=(( $ 3 + 2 ))
print $x                 # 5
----

=== Assign only if undefined

The builtin `default` assigns only when the variable is not already set; otherwise it does nothing:

----
default x 10
print $x          # 10
default x 20
print $x          # still 10
----

One or more `NAME=VALUE` pairs may be provided:

----
default k1=v1 k2=v2
----

NOTE: "Already set" means anything the <<defined_text,defined test>> `$+name` finds: a variable, a global (including a `KEY=VALUE` given on the command line), an environment
variable, or a magic variable such as `$this`.  So `default PATH x`
leaves the environment's `PATH` alone.

=== Dropping a variable

`drop` removes a variable.

----
x=42
drop x
print $x                 # uncaught exception
----

=== Length and the defined-test

`$#name` gives the length of a string or the size of a list/table.
`$+name` is `1` if the variable is defined, else `0`.

----
x=abc
print $#x          # 3
print $+x          # 1
print $+missing    # 0
----

== Expressions

The `$` builtin evaluates an arithmetic/logical expression:

----
print (( $ 2 + 3 ))          # 5
print (( $ 2.3 + 4.5 ))      # 6.8
print (( $ ( 1 + 5 ) / 3 ))  # 2   -- parentheses group
----

=== Operators

[cols="1,3"]
|===
| Operators | Meaning

| `+ - * /`      | Add, subtract, multiply, divide
| `%/`           | Integer division
| `%`            | Modulo
| `- x`          | Unary negation
| `== != < > <= >=` | Comparisons (yield `1`/`0`)
| `&& \|\|`      | Logical and / or (yield `1`/`0`)
| `c ? a : b`    | Ternary conditional
|===

----
print (( $ 1 ? 600 : 700 ))          # 600
print (( $ ( 1 - 1 ) ? 800 : 900 ))  # 900
print (( $ 1 < 2 && 3 < 4 ))         # 1
----

Comparison and logical operators return integer `1`/`0`, not `bool`.

A quoted string literal is rejected as an arithmetic operand:

----
print (( $ 2 + "3" ))   # error: invalid operand (quoted string literal)
----

=== Math functions

The expression language provides these functions.  Unknown names fall back
to a user-defined function of that name.

----
print (( $ sqrt( 16 ) ))         # 4.0
print (( $ pow( 2, 10 ) ))       # 1024.0
print (( $ max( 3, 7 ) ))        # 7.0
print (( $ sqrt( abs( -16 ) ) )) # 4.0   -- calls nest
print (( $ abs( -2 ) + 10 ))     # 12    -- mix with operators
----

`abs` preserves the operand type (int stays int, float stays float).  The
unary functions include `sqrt cbrt exp log log2 log10 sin cos tan asin
acos atan floor ceil round trunc`; the binary functions include `pow atan2
hypot min max`.

[[subshell]]
=== Substitution in subshells

Like other shells, MCSH can run commands in subprocess shells (subshells) and capture the output.  This is different from <<subcommand,subcommand evaluation>>.

[cols="1,3"]
|===
| Form | Meaning

| `+(( cmd ))+`   | Run `cmd` and substitute its return value.
| `+$(( cmd ))+`  | Run `cmd` and substitute its captured standard output.
|===

----
y=(( f 3 ))        # y gets f's return value
print (( type $y ))  # substitute a builtin's result
----

== Strings and data operations

=== Concatenation, appending, inserting: `+`

`+` means different things by the type of its first argument:

----
print (( + AA BB CC ))   # string concatenation -> AABBCC
+ $L a b c               # append to a list
+ $T key value           # insert into a table
----

=== Increment: `++`

----
i=0
++ i
print $i            # 1
----

=== Subscripts and slices

Index a string or list with `$x[i]`.  A range `$x[a:b]` is the half-open
slice from `a` up to (not including) `b`; either end may be omitted.

----
x=abcdef
print $x[1]         # b
print $x[2:4]       # cd
print $x[:2]        # ab
----

Lists subscript and slice the same way:

----
print $L[1]         # second element
print $L[2:4]       # a two-element sub-list
----

=== Regular expressions

`$x/regex` tests whether the value of `x` matches `regex` (POSIX basic
regular expressions), yielding `1` or `0`.  `$x/regex/replacement`
substitutes the first match:

----
x=abcdef
print $x/cdx        # 0   -- no match
print $x/cd/O88O    # abO88Oef   -- substitute
----

=== Globbing

An unquoted word containing `*` is expanded against the filesystem, and
the matches are split into separate arguments:

----
print *.txt         # expands to the matching file names
+ $L a b c *.txt    # glob results are appended as separate elements
----

=== Other string builtins

----
s=(( substring justin 2 4 ))   # st  -- characters [2,4)
i=(( find justin st ))         # 2   -- index of substring, or -1
print (( string = a a a ))       # 1   -- all arguments equal?
----

=== Lists

----
L=(( list ))
+ $L jk mn op
print $L                 # [jk,mn,op]
L2=(( split jk,mn,ab , ))  # split on a delimiter
s=(( join $L "," ))       # join with a delimiter
sort $L                      # sort in place (ints or strings)
----

=== Tables

----
T=(( table ))
+ $T a 1
+ $T b 2
print $T          # {a:1,b:2}
print $T[a]       # 1       -- look up by key
print $@T         # [a,b]   -- the keys as a list
print $#T         # 2       -- size
----

== Built-in commands

=== Output and the null command

----
print HELLO             # print arguments, space-separated, newline at end
:                       # the no-op command; evaluates its args, does nothing
----

=== Files

----
fp=(( open "f.txt" w ))   # modes "r", "w", ...
>> $fp HELLO                # write a line to a file handle
close $fp
fp=(( open "f.txt" r ))
line=(( << $fp ))         # read a line
close $fp
flush                       # flush output
----

With no handle, `<<` reads a line from standard input.

=== Operating system: `os`

----
print (( os pwd ))          # working directory
os cd /some/dir             # change directory (raises on failure)
os mkdir data               # make directory, including parents
os mv old new               # rename
os rm file                  # remove
print (( os env HOME ))     # environment variable
print (( os ..   /a/b/c ))  # dirname  -> /a/b
print (( os file /a/b/c ))  # basename -> c
print (( os resolve ./.. )) # canonical path
print (( os pid ))          # process id
----

=== External programs

----
! sleep 1                   # exec a program directly (searches PATH)
sh echo HELLO               # run a command line through /bin/sh
----

=== Background jobs

----
bg long-running-command     # start in the background, returns a pid
wait $pid                   # wait for it
jobs                        # list background jobs
----

=== Timing

----
sleep 2.3                   # seconds, fractional allowed
now=(( clock ))           # seconds since the epoch (int)
----

=== Evaluation and inclusion

----
eval print HI               # evaluate arguments as a command
. other.mc                  # source another script into this scope
import submodule            # import a module; names become submodule.NAME
----

=== Conversions and inspection

----
print (( type $x ))         # the type name
n=(( as int "42" ))       # parse to int
----

=== Exit

----
exit                        # exit 0
exit 1                      # exit with a code
----

=== Miscellaneous builtins

The spaced form of assignment, `= name value`, also works and means the
same as `name=value`.  This manual uses the attached form throughout:

----
= x 3                       # same as x=3
----

== Control flow

=== Conditionals

The condition and each branch are brace blocks.  `or` introduces the
else-branch; several `if` clauses may be chained:

----
if { $ 1 } {
  print YES
}

if { $ 0 } {
  print NO
} or {
  print OK
}

if { $ 0 } {
  exit 1
} if { $ 0 + 1 } {
  print OK
}
----

=== Loops

`loop` with a trailing `while`/`until` is a post-tested loop; `loop while`
in front is pre-tested; a bare `loop` runs until `break`:

----
loop {
  print HELLO $x
} while { $ $x < 3 }

loop {
  print HELLO $x
} until { $ $x == 3 }

loop while { $ $x < 3 } {
  x=(( $ $x + 1 ))
}

loop {
  break
}
----

`for` takes init / condition / step blocks; `foreach` iterates a list;
`repeat` runs a fixed number of times, optionally binding the index:

----
for { i=0 } { $ $i < 3 } { ++ i } {
  print i is $i
}

foreach i $L {
  print i is $i
}

repeat 3 { print LOOP }
repeat i 3 { print LOOP $i }
----

`break` and `continue` work inside any loop.

== Functions

=== Defining and calling

A function has a name, a signature block, and a body block.  It is called
by name with arguments; it is invoked only from within MCSH, never from the
command line.

----
function f { x y } {
  print XY $x $y
}
f 2 3               # XY 2 3
----

The last value produced by the body is the function's return value, which
can be captured with `+(( ... ))+`:

----
function inc { x } {
  $ $x + 1
}
y=(( inc 3 ))     # 4
----

=== Scope, globals, and macros

Function parameters and variables assigned in a body are local.  `global`
links a name to a global variable; `public` exposes a module variable.

----
function f { a } {
  global x
  x=3
}
----

`inplace` and `macro` define callables that run in the caller's scope
(useful for mutating caller variables), with the same signature grammar:

----
inplace m { } { g=1000 }
macro   n { } { print HELLO }
----

=== Signatures

The signature block declares the parameters.  A parameter may carry a
type and/or a default, and may be a flag.  The last parameter may be a
rest parameter, which collects any extra arguments.

[cols="1,3"]
|===
| Form | Meaning

| `x`            | A positional parameter.
| `x:int`        | Typed; the argument is type-checked.
| `x=789`        | A default value (used when the argument is absent).
| `x:int=789`    | Typed with a default.
| `-v:bool=false`| A boolean *flag*: the token `-v` sets it to `true`.
| `-o=out`       | A value flag: `-o VALUE` sets it; defaults to `out`.
| `+rest...+`    | A rest parameter: a list of the leftover arguments.
| `+rest:int...+` | A typed rest parameter: each leftover is type-checked.
| `+...+`        | Allows extra arguments without naming them.
|===

A default must be written with `=`, never a second colon (`x:int:789` is a
syntax error).  A rest parameter must be the last parameter, cannot have
a default or be a flag, and cannot repeat another parameter's name.
Each of these is a syntax error, as is an unknown type name.

=== Calling conventions

Arguments may be given positionally, by name, or as flags; names and flags
may appear in any order, and defaults fill the rest:

----
function f { x y z:int=789 } {
  print f_result x $x y $y z $z
}

f 1 2 3           # positional          -> x 1 y 2 z 3
f 1 2             # default applies      -> x 1 y 2 z 789
f 1 2 z=55        # named overrides      -> x 1 y 2 z 55
f x=10 y=20 z=30  # all named            -> x 10 y 20 z 30
f 1 z=5 y=9       # positional + named   -> x 1 y 9 z 5
----

Some arguments are converted to the parameter's type before the type
check: an `int` becomes a `float` for a `float` parameter, and `0` or `1`
becomes `false` or `true` for a `bool` parameter.  A quoted string stays
a string, so `f "42"` does not satisfy an `int` parameter.  Command-line
arguments are text, and are converted too (see
<<_command_line_arguments,Command-line arguments>>).

Error cases raise exceptions: an unknown name (`no such argument`), a type
mismatch (`requires a int`), a missing required argument (`did not assign
to`), extra arguments without a rest parameter (`too many arguments`),
and the syntax errors noted above.

=== Flags

Flags use the same signature grammar, and work identically whether the
arguments come from a function call or from the command line.  A `bool`
flag is presence-only (`-v` sets it `true`); any other flag consumes the
next argument as its value.

----
function run { -v:bool=false -o=out file } {
  print run v $v o $o file $file
}

run in.txt                    # run v false o out        file in.txt
run -v in.txt                 # run v true  o out        file in.txt
run -v -o result.txt in.txt   # run v true  o result.txt file in.txt
----

=== Rest parameters

A rest parameter, written `+NAME...+` as the last parameter, collects
the positional arguments that no other parameter took into a list
variable `NAME`.  The list is empty if there are none.

----
function f { x y z args... } {
  print $x $y $z    # a b c
  print $args       # [d,e,f]
  print $#args      # 3
}

f a b c d e f
----

A typed rest parameter, as in `+ns:int...+`, type-checks each leftover
argument.  A rest parameter written just `+...+` allows extra arguments
without binding them to a name; they are still available through
`+$@+` and `+$*+`.

=== Positional arguments

These read the positional arguments of the current function call:

[cols="1,3"]
|===
| Name | Meaning

| `+$0+`             | The function's name.
| `+$1+`, `+$2+`, ... | The positional arguments, counting from 1.  A missing one raises an exception.
| `++$+1++`          | `1` if `+$1+` exists, else `0`; never raises.
| `+$#+`             | The number of positional arguments.
| `+$@+`             | The positional arguments as one list.
| `+$*+`             | The positional arguments as separate words (list expansion).
|===

Only positional arguments count: not flags, flag values, or `name=value`
arguments.  They include the arguments bound to named parameters as
well as any rest arguments:

----
function f { -v:bool=false x args... } {
  print $1 $#       # a 3
  print $@          # [a,b,c]
  print $*          # a b c
}

f a -v b c
----

Unlike bash and zsh, `+$@+` is a single list value; use `+$*+` to pass
the arguments on as separate words.  An `inplace` or `macro` call shares
the arguments of the function that called it.  At script top level,
these read the script's own arguments (see <<_command_line_arguments,Command-line arguments>>).

=== shift

`shift` drops the first positional argument, so that `+$1+` is the next
one; `shift N` drops `N` of them.  It returns `true`, or `false` if
fewer than `N` arguments were left, in which case it drops all of them.
It changes only what `+$1+`, `+$@+`, `+$*+` and `+$#+` see:
`$mcsh.argc` and `$mcsh.argv` stay the same.

----
function each { args... } {
  loop while { $ $+1 } {
    print item: $1
    shift
  }
}

each x y z          # item: x   item: y   item: z
----

In an `inplace` or `macro` call, `shift` acts on the arguments of the
function that called it.

== Command-line handling

[[signature]]
=== The `signature` builtin

The `signature` command binds the script's own command-line arguments
using the exact same grammar as a function signature -- this shared
syntax is a central feature of MCSH.  After `signature`, each parameter
name becomes a variable.

----
signature -f:bool=false -g:bool=false -x=_ -y=_ z=data
print $f $g $x $y $z
----

Run as `mcsh script.mc -f -x dog`, this prints:

----
true false dog _ data
----

that is: `-f` is present (true), `-g` defaults false, `-x` takes `dog`,
`-y` defaults `_`, and the positional `z` defaults to `data`.

The rules are the same as for function calls (see <<_signatures,Signatures>> and
<<_calling_conventions,Calling conventions>>):

* Arguments after the script name fill the positional parameters in
  order; flags may appear anywhere among them.
* A parameter with a default may be omitted.
* Arguments are text, converted to the parameter's type: `42` becomes
  an `int` for an `int` parameter, and `true`, `false`, `1` or `0` a
  `bool` for a `bool` parameter.
* A required parameter that is not given raises an exception
  (`did not assign to: 'NAME'`).
* An argument that cannot be converted raises an exception
  (`requires a int, given a string`).
* Extra positional arguments go to a rest parameter, which becomes a
  global list.  Without one, they raise an exception
  (`too many arguments`).

One difference: `name=value` arguments are not recognized on the
command line.  After the script name, `x=5` is an ordinary positional
argument.  Before the script name, `KEY=VALUE` sets a global (see
<<_run_a_script,Run a script>>).

The parameters become global variables.  At top level, `+$0+` is the
script name, and `+$1+`, `+$@+`, `+$*+` and `+$#+` read the script's
arguments (see <<_positional_arguments,Positional arguments>>); `shift` works on them too.
After `signature`, they see only the positional arguments, not the
flags it consumed, and any earlier `shift` is undone.  The raw
arguments are always available as `$mcsh.argc` and `$mcsh.argv` (see
<<_magic_variables,Magic variables>>).

== Magic variables

These read-only variables are always available:

[cols="1,3"]
|===
| Name | Meaning

| `$this`       | Absolute directory of the running script.
| `$time`       | Current time in seconds (float), fresh on each read.
| `$random`     | A fresh pseudo-random integer on each read.
| `$?`          | Meant to be the exit code of the last external command, but not yet implemented: it is always `0`.
| `$PWD`        | Working directory (an environment variable).
| `$mcsh.argc`  | Number of command-line arguments, counting the script name: a script run with no arguments has `argc` 1.
| `$mcsh.argv`  | List of command-line arguments; `+(( get $mcsh.argv 0 ))+` is the script name.
|===

----
print $this
t1=$time
r=$random
----

== Experimental features

The following are present in the test suite but not yet complete, and may
change: `module { ... }` blocks and `class`/`extends` definitions.

== Examples

=== Fibonacci

A recursive Fibonacci, driven by a one-parameter script signature:

----
signature n

function fib { x } {
  if { $ $x == 0 } {
    r=0
  } if { $ $x == 1 } {
    r=1
  } or {
    r1=(( fib (( $ $x - 1 )) ))
    r2=(( fib (( $ $x - 2 )) ))
    r=(( $ $r1 + $r2 ))
  }
  $ $r
}

print (( fib $n ))
----

Run this from the command line with:

----
$ mcsh fib.mc 5
----

== Installation

=== Building and testing

Build from a git checkout with the autotools:

----
$ ./bootstrap
$ ./configure --prefix=$HOME/mcsh-install
$ make
$ make install
----

=== Build information

`mcsh -B` reports how this `mcsh` was configured and built, then
exits.  The report covers when `configure` ran and the arguments it
was given, the git commit at that time, when `mcsh` was compiled, the
host, the C compiler and its version, the compiler and linker flags
(`CPPFLAGS`, `CFLAGS`, `AM_CFLAGS`, `LDFLAGS`, `LIBS`), the lexer and
parser generators, and whether readline and its history support are
included, with the readline library version.  Its last section shows
whether Unicode features are enabled in the current environment, why,
and the variables that decide it (see <<unicode,Unicode>>).

----
$ mcsh -B
MCSH 0.0 build information:
  configured           2026-11-22 13:07:21
  compiled             Nov 22 2026 13:07:23
  ...
  readline             yes
    configure          requested yes, found -lreadline
    library            8.3
    history            yes
Unicode in this environment:
  MCSH_UNICODE         (unset)
  MCSH_UNICODE_FORCE   (unset)
  TERM                 xterm-256color
  LC_ALL               (unset)
  LC_CTYPE             (unset)
  LANG                 en_US.UTF-8
  locale codeset       UTF-8
  Unicode is enabled (UTF-8 locale and TERM=xterm-256color)
----

=== Test suite

The test suite is `test/script/test-all.zsh`, and needs `zsh`.  The
GitHub workflow (`.github/workflows/tests.yaml`) builds with
`--disable-readline` and runs the suite on `ubuntu-latest` and
`macos-15`.  On macOS the workflow installs `autoconf`, `automake`,
`libtool`, `flex`, `bison` and `coreutils` with Homebrew.  The system
`bison` is too old for the grammars, and `coreutils` supplies
`gtimeout`, since macOS has no `timeout`.  Tests that need readline are
skipped when the build has none.

=== Build and runtime options

[[unicode]]
==== Unicode

Unicode features, such as the shamrock prompt, are enabled when the
locale uses UTF-8, as most modern systems do, and `TERM` is not one of
these terminals, which often cannot show them:

* `dumb`, or `TERM` unset or empty
* `linux` or `linux-...`, the Linux text console
* `vt52`, `vt100`, `vt220`, ..., DEC VT terminals

Two environment variables override this:

[cols="1,3"]
|===
| Variable | Effect

| `MCSH_UNICODE=0`       | Disable Unicode features.  The default is `1`: decide as above.
| `MCSH_UNICODE_FORCE=1` | Enable Unicode features, even for the terminals above or a locale that is not UTF-8; `mcsh` then switches to the `C.UTF-8` locale if the system has it.  The default is `0`.
|===

`MCSH_UNICODE=0` takes precedence over `MCSH_UNICODE_FORCE=1`.  For
example, an Emacs shell buffer sets `TERM=dumb` but can show Unicode,
so set `MCSH_UNICODE_FORCE=1` there to see the shamrock.  The end of
the `mcsh -B` report shows the decision and the variables behind it
(see <<_build_information,Build information>>).

COMMENT(
  Note to Claude: Use long lines in this file.
)

COMMENT(
Local Variables:
mode: doc;
End:
)
