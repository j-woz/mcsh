
AUTOMAKE_OPTIONS = subdir-objects

SUFFIXES = .c .h .y .l

AM_CPPFLAGS = -I src
AM_CFLAGS = -std=gnu2x -Wall -Wno-unused-function -g -Werror -fdiagnostics-color=always
# -pedantic
AM_YFLAGS = -d
AM_LDFLAGS = -l readline
# @LIBREADLINE@

all: $(bin_PROGRAMS)

C_TESTS = test/util/strlcpy-1.x \
	  test/util/strmap-1.x \
	  test/util/strmap-2.x \
	  test/util/strmap-3.x \
	  test/util/strmap-4.x \
          test/util/trim.x     \
	  test/util/buffer-1.x \
	  test/util/fork-1.x   \
	  test/util/log-1.x    \
	  test/util/check-1.x  \
	  test/util/check-2.x  \
	  test/util/check-3.x  \
	  test/util/check-4.x  \
	  test/unit/named-args-1.x \
	  test/unit/named-args-2.x \
	  test/unit/named-args-3.x \
	  test/unit/arg-extras.x

bin_PROGRAMS = bin/mcsh bin/mcc bin/mcpp bin/mchp \
               $(C_TESTS)

src/lex.mcs_expr_.c: src/mcsh-expr-lexer.l src/mcsh-expr-grammar.h
	flex -o $(@) $(<)

src/mcsh-expr-lexer.o: src/lex.mcs_expr_.c
	gcc $(CFLAGS) -c -o $(@) $(<)

src/lex.mcs_script_.c: src/mcsh-script-lexer.l src/mcsh-script-grammar.h
	flex -o $(@) $(<)

src/mcsh-script-lexer.o: src/lex.mcs_script_.c
	gcc $(CFLAGS) -c -o $(@) $(<)

bin_mcc_SOURCES  = src/mcsh-calc.c
bin_mcsh_SOURCES = src/mcsh-main.c
bin_mcpp_SOURCES = src/mcsh-pp.c src/mcsh-preprocess.c \
                   src/util.c src/buffer.c
bin_mchp_SOURCES = src/mcsh-hp.c

lib_LIBRARIES = lib/libmcsh.a

bin_mcc_LDADD = lib/libmcsh.a
bin_mcsh_LDADD = lib/libmcsh.a
bin_mchp_LDADD = lib/libmcsh.a

test_util_strlcpy_1_x_SOURCES = test/util/strlcpy-1.c
test_util_strlcpy_1_x_LDADD   = lib/libmcsh.a

test_util_strmap_1_x_SOURCES = test/util/strmap-1.c
test_util_strmap_1_x_LDADD   = lib/libmcsh.a

test_util_strmap_2_x_SOURCES = test/util/strmap-2.c
test_util_strmap_2_x_LDADD   = lib/libmcsh.a

test_util_strmap_3_x_SOURCES = test/util/strmap-3.c
test_util_strmap_3_x_LDADD   = lib/libmcsh.a

test_util_strmap_4_x_SOURCES = test/util/strmap-4.c
test_util_strmap_4_x_LDADD   = lib/libmcsh.a

test_util_trim_x_SOURCES = test/util/trim.c
test_util_trim_x_LDADD   = lib/libmcsh.a

test_util_buffer_1_x_SOURCES = test/util/buffer-1.c
test_util_buffer_1_x_LDADD   = lib/libmcsh.a

test_util_fork_1_x_SOURCES = test/util/fork-1.c
test_util_fork_1_x_LDADD   = lib/libmcsh.a

test_util_log_1_x_SOURCES = test/util/log-1.c
test_util_log_1_x_LDADD   = lib/libmcsh.a

test_unit_named_args_1_x_SOURCES = test/unit/named-args-1.c
test_unit_named_args_1_x_LDADD   = lib/libmcsh.a

test_unit_named_args_2_x_SOURCES = test/unit/named-args-2.c
test_unit_named_args_2_x_LDADD   = lib/libmcsh.a

test_unit_named_args_3_x_SOURCES = test/unit/named-args-3.c
test_unit_named_args_3_x_LDADD   = lib/libmcsh.a

test_unit_arg_extras_x_SOURCES = test/unit/arg-extras.c
test_unit_arg_extras_x_LDADD   = lib/libmcsh.a

lib_libmcsh_a_SOURCES = \
	src/mcsh-expr-grammar.y    src/mcsh-expr-lexer.l    \
	src/mcsh-expr-parser.c                              \
	src/mcsh-script-grammar.y  src/mcsh-script-lexer.l  \
	src/mcsh-sys.c src/log.c                            \
	src/mcsh.c src/mcsh-data.c src/mcsh-script-parser.c \
	src/activations.c \
	src/mcsh-parser.c src/mcsh-iface.c \
	src/builtins.c 	src/exceptions.c  \
	src/table.c src/strkeys.c src/lookup3.c \
	src/list-array.c src/list_i.c \
	src/strmap.c src/mcsh-preprocess.c \
	src/util-string.c src/buffer.c src/util.c

clean-local::
	rm -fv src/*_.c src/*-grammar.[ch] src/*-lexer.c
