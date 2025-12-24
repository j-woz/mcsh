
/**
  MCSH EXPR GRAMMAR Y
*/

%{
  #include <assert.h>
  #include <stdio.h>
  #include <stdlib.h>

  #include "mcsh-expr-parser.h"

  // Declare stuff from Flex that Bison needs to know about:
  extern int mcsh_expr_lex();

  extern FILE* mcsh_expr_in;
  extern int   mcsh_expr_line;

  void yyerror(const char* message);
%}

%define api.prefix {mcsh_expr_}
/* %define api.value.type {double} */
%define parse.error verbose


// Bison fundamentally works by asking flex to get the next token, which it
// returns as an object of type "yystype".  Initially (by default), yystype
// is merely a typedef of "int", but for non-trivial projects, tokens could
// be of any arbitrary data type.  So, to deal with that, the idea is to
// override yystype's default typedef to be a C union instead.  Unions can
// hold all of the types of tokens that Flex could return, and this this means
// we can return ints or floats or strings cleanly.  Bison implements this
// mechanism with the %union directive:
%union {
  char* sval;
  mcsh_node* node;
}

%token LPAREN
%token RPAREN
%token NL
%token END
%token SEMICOLON
%token <sval> EQ
%token <sval> NE
%token <sval> LT
%token <sval> GT
%token <sval> LE
%token <sval> GE
%token <sval> TOKEN
%token <sval> PLUS
%token <sval> MINUS
%token <sval> MULT
%token <sval> DIV
%token <sval> IDIV
%token <sval> MOD
%token <sval> QM
%token <sval> COLON

%type <node> program lines line expr ;

%left EQ NE LT GT LE GE
%left PLUS MINUS
%left MULT DIV IDIV MOD

%start program

%%

program: lines END {
  // printf("parser: program END.\n");
  mcsh_parse_output_set($1);
  return 1;
 }

lines: %empty {
  $$ = NULL;
  // printf("lines EMPTY\n");
 }
        |
                lines line
                {
                  $$ = mcsh_node_expr_join($1, $2, mcsh_expr_line);
                }
                ;
        |
                lines line NL
                {
                  $$ = mcsh_node_expr_join($1, $2, mcsh_expr_line);
                }
                ;

line:
                %empty
                {
                  // printf("EMPTY LINE\n");
                  $$ = NULL;
                }
        |
                expr
        |
                line SEMICOLON expr
                {
                  // printf("SEMICOLON\n");
                  $$ = mcsh_node_expr_join($1, $3, mcsh_expr_line);
                }
                ;

expr:
                TOKEN
                {
                  $$ = mcsh_node_token($1, mcsh_expr_line);
                }
        |
                LPAREN expr RPAREN
                {
                  $$ = $2;
                  // printf("unpack parens\n");
                }
        |
                expr QM expr COLON expr
                {
                  // printf("found TERN\n");
                  $$ = mcsh_node_tern($1, $3, $5, mcsh_expr_line);
                }
        |
                expr EQ expr
                {
                  // printf("found: EQ: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_EQ, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr NE expr
                {
                  // printf("found: NE: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_NE, $1, $3, mcsh_expr_line);
                }
        |
                expr LT expr
                {
                  // printf("found: LT: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_LT, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr GT expr
                {
                  // printf("found: GT: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_GT, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr LE expr
                {
                  printf("found: LE: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_LE, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr GE expr
                {
                  printf("found: GE: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_GE, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr PLUS expr
                {
                  // printf("found: PLUS: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_PLUS, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr MINUS expr
                {
                  // printf("found: minus: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_MINUS, $1, $3,
                                    mcsh_expr_line);
                }
        |
                expr MULT expr
                {
                  // printf("found: mult: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_MULT, $1, $3,
                                    mcsh_expr_line);
                }
        |       expr DIV expr
                {
                  // printf("found: div: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_DIV, $1, $3,
                                    mcsh_expr_line);
                }
        |       expr IDIV expr
                {
                  // printf("found: idiv: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_IDIV, $1, $3,
                                    mcsh_expr_line);
                }
        |       expr MOD expr
                {
                  // printf("found: mod: %s\n", $2);
                  $$ = mcsh_node_op(MCSH_OP_MOD, $1, $3,
                                    mcsh_expr_line);
                }
        ;

%%

/* line: */
/*                 expr ; */


        /* | */
        /*         line SEMICOLON expr { */

        /*         } */
        /*         ; */


// extern int yylex(void);

void
yyerror(const char* message)
{
  mcsh_expr_grammar_status = false;

  // printf("MCSH EXPR ERROR: line=%i %s \n", mcsh_expr_line, s);

  valgrind_assert(mcsh_expr_grammar_message == NULL);

  mcsh_expr_grammar_message = strdup(message);

  // exit(EXIT_FAILURE);
}
