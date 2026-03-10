
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
                { printf("bison: single stmt\n");
                  $$ = mcsh_node_stmt(NULL, $1, mcsh_script_line); }
        |
                stmts NL stmt
                { printf("bison: node stmt NL\n");
                  $$ = mcsh_node_stmt($1, $3, mcsh_script_line); }
        |
                stmts SEMICOLON stmt
                { printf("bison: node stmt SC\n");
                  $$ = mcsh_node_stmt($1, $3, mcsh_script_line); }
                ;

stmt:
                %empty
                { printf("bison: empty stmt\n");
                  $$ = NULL; }
        |
                term
                {
                  printf("bison: stmt-1-term\n");
                  $$ = mcsh_node_term(NULL, $1, mcsh_script_line);
                }
        |
                stmt WS term
                {
                  printf("bison: stmt-WS-term\n");
                  $$ = mcsh_node_term($1 , $3, mcsh_script_line);
                }
                ;

term:
                STRING
                { // printf("bison: string: '%s' @ %i\n", $1,
                  //        mcsh_script_line);
                  $$ = mcsh_script_token($1, mcsh_script_line);
                  free($1); }
        |
                LBRACE stmts RBRACE
                { // printf("bison: block\n");
                  $$ = mcsh_node_block($2, mcsh_script_line); }
        |
                SUBCMD WS stmts WS RPARENS
                { // printf("bison: subst\n");
                  $$ = mcsh_node_subcmd($3, mcsh_script_line);
                }
        |
                FUNCTN stmts RPARENS
                { // printf("bison: subst\n");
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
  printf("MCSH PARSE ERROR: line %i: %s\n", mcsh_script_line, msg);
  exit(EXIT_FAILURE);
}
