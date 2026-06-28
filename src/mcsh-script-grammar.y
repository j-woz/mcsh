
/*
  MCSH SCRIPT Y
*/

/* %name-prefix "mcsh_script_" */
%define api.prefix {mcsh_script_}

%{
  #include <stdio.h>
  #include <stdlib.h>

  #include "mcsh-script-parser.h"

  // Declare stuff from Flex that Bison needs to know about:
  extern int mcsh_script_lex();
  extern FILE* mcsh_script_in;

  extern int mcsh_script_line;

  // For the LOG() macro used in grammar actions:
  #define logger (mcsh.parse_state.logger)

  void yyerror(const char *s);
%}

%union {
  char* sval;
  mcsh_node* node;
}

%token <sval> STRING

%token LBRACE
%token RBRACE
%token SUBCMD
%token RPARENS
%token FUNCTN
// %token RFUNCT
%token EQ

%token WS // Whitespace
%token NL
%token END
%token SEMICOLON

%type   <node>          program stmts stmt term

%%

program:
 stmts END {
   mcsh_parse_output_set($1);
   $$ = $1;
   return 1;
 }

stmts:
                stmt
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: single stmt");
                  $$ = mcsh_node_stmt(NULL, $1, mcsh_script_line); }
        |
                stmts NL stmt
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: node stmt NL");
                  $$ = mcsh_node_stmt($1, $3, mcsh_script_line); }
        |
                stmts SEMICOLON stmt
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: node stmt SC");
                  $$ = mcsh_node_stmt($1, $3, mcsh_script_line); }
                ;

stmt:
                %empty
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: empty stmt");
                  $$ = NULL; }
        |
                term
                {
                  LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: stmt-1-term");
                  $$ = mcsh_node_term(NULL, $1, mcsh_script_line);
                }
        |
                stmt WS term
                {
                  LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: stmt-WS-term");
                  $$ = mcsh_node_term($1 , $3, mcsh_script_line);
                }
        |
                stmt WS
                {
                  $$ = $1;
                }
                ;

term:
                STRING
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: string: '%s' @ %i",
                      $1, mcsh_script_line);
                  $$ = mcsh_script_token($1, mcsh_script_line);
                  free($1); }
        |
                LBRACE stmts RBRACE
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: block");
                  $$ = mcsh_node_block($2, mcsh_script_line); }
        |
                SUBCMD stmts RPARENS
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: subst");
                  $$ = mcsh_node_subcmd($2, mcsh_script_line);
                }
        |
                FUNCTN stmts RPARENS
                { LOG(MCSH_LOG_PARSE, MCSH_TRACE, "bison: subst");
                  $$ = mcsh_node_subfun($2, mcsh_script_line);
                }
        |
                STRING EQ term
                {
                  $$ = mcsh_node_tag($1, $3, mcsh_script_line);
                }
        |
                EQ
                {
                  $$ = mcsh_script_token("=", mcsh_script_line);
                }
                ;
%%

// extern int yylex(void);

void
yyerror(const char* msg)
{
  LOG(MCSH_LOG_PARSE, MCSH_FATAL, "MCSH PARSE ERROR: line %i: %s",
      mcsh_script_line, msg);
  exit(EXIT_FAILURE);
}
