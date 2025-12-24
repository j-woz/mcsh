%{
  #include <stdio.h>
  #include "terms.tab.h"  // to get the token types from Bison
%}

%%

[ \t]           ;
\n { return NL; }
"{"  { return LBRACE; }
"}"  { return RBRACE; }
<<EOF>> { return END; }
[a-zA-Z0-9]+      { yylval.sval = strdup(yytext);
  return STRING;
}

%%

