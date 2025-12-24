
%{
  #include <stdio.h>
  #include <stdlib.h>

  // Declare stuff from Flex that Bison needs to know about:
  extern int yylex();
  extern int yyparse();
  extern FILE* yyin;
 
  void yyerror(const char *s);
%}

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
}

%token <sval> STRING
%token LBRACE
%token RBRACE
%token NL
%token END

%%

program:
 stmts { printf("program end\n"); return 1; } END NL

stmts:
 terms NL { printf("stmts more:\n"); } stmts 
 | terms NL { printf("stmts2\n"); }
 ;

terms:
  /* empty */
  /*  | term     { printf("terms2\n"); } */
  | term { printf("terms1\n"); } terms 
  ;

term: 
  STRING {
      printf("bison found a string: '%s'\n", $1);
      free($1);
  } 
  | LBRACE { printf("block start\n"); }
    terms 
    RBRACE { printf("block end\n"); } 
  ;
%%

int
main(int argc, char* argv[])
{
  printf("START\n");
  yyparse();
  printf("DONE\n");
  return EXIT_SUCCESS;
}

void
yyerror(const char *s)
{
  printf("YY ERROR: %s \n", s);
  exit(EXIT_FAILURE);
}
