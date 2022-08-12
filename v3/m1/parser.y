%require "3.5"
%language "c++"
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define parse.trace
%define parse.error verbose

%code requires {
  #include <string>
  #include <cstdint>
  #include "ast.h"
}

%code provides {
  #define YY_DECL \
    ::yy::parser::symbol_type yylex()

  /** Module def from latest parse. */
  extern ModuleDef* ParsedModuleDef;
}

%code {
  extern YY_DECL;

  #define yyerror(x)

  ModuleDef* ParsedModuleDef;
}

%token CLASS ELSE FN IF IMPORT LET NEW RETURN WHILE
%token <::std::string> IDENTIFIER STRING_LITERAL
%token <int64_t> NUMBER_LITERAL
%token COLON SEMICOLON COMMA DOT LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token AND OR NOT ADD SUB EXP MUL DIV MOD EQ NE GTE LTE GT LT ASSIGN

%nterm <ModuleDef*> module

%%

module: CLASS  { $$ = nullptr; }

%% 

void ::yy::parser::error (const std::string& m) {
  ::std::cerr << m << ::std::endl;
}
