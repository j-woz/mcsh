
/**
   MCSH SCRIPT PARSER
*/

#pragma once

#include "mcsh-parser.h"

// in mcsh.c
extern bool mcsh_script_token_quoted;

mcsh_node* mcsh_script_token(char* term, int line);

mcsh_node* mcsh_node_term(mcsh_node* left, mcsh_node* right,
                          int line);

mcsh_node* mcsh_node_stmt(mcsh_node* left, mcsh_node* right,
                          int line);

mcsh_node* mcsh_node_block(mcsh_node* stmts, int line);

mcsh_node* mcsh_node_subcmd(mcsh_node* stmts, int line);

mcsh_node* mcsh_node_subfun(mcsh_node* stmts, int line);
