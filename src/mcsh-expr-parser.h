
/**
   MCSH EXPR PARSER
*/

#pragma once

#include <stdbool.h>
#include "mcsh-parser.h"

/** Current source code line: incremented by lexer */
extern int mcsh_expr_line;

/** This is true unless there is an error */
extern bool  mcsh_expr_grammar_status;

/** On error, this contains the error message */
extern char* mcsh_expr_grammar_message;

void mcsh_expr_token(const char* token);

void mcsh_expr_end(void);

void mcsh_expr_op(const char* op_string);

#include "mcsh-parser-nodes.h"
