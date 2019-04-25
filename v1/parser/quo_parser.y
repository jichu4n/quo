%{
extern int yylex();
#define yyerror(x)
%}

%token X

%%

program: X;