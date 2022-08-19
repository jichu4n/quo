%{
#include "parser.h"
#include <cstdlib>

using namespace yy;
%}

%option noyywrap nounput noinput batch

%%

"//"[^\n]*		/* Discard comment. */
[ \t\n]+		/* Discard whitespace. */

  /* Keywords. */
"class"			{ return parser::make_CLASS(); }
"else"			{ return parser::make_ELSE(); }
"fn"			{ return parser::make_FN(); }
"if"			{ return parser::make_IF(); }
"import"			{ return parser::make_IMPORT(); }
"let"			{ return parser::make_LET(); }
"new"			{ return parser::make_NEW(); }
"null"			{ return parser::make_NULL_LITERAL(); }
"return"		{ return parser::make_RETURN(); }
"while"			{ return parser::make_WHILE(); }
  /* Other classes of tokens. */
[_a-zA-Z][_a-zA-Z0-9]*	{ return parser::make_IDENTIFIER(yytext); }
[0-9]+			{ return parser::make_NUMBER_LITERAL(strtoll(yytext, nullptr, 10)); }
'([^\\']|\\.)*'		{ return parser::make_STRING_LITERAL(::std::string(yytext + 1, yyleng - 2)); }
\"([^\\\"]|\\.)*\"	{ return parser::make_STRING_LITERAL(::std::string(yytext + 1, yyleng - 2)); }
  /* Operators. */
"&&"			{ return parser::make_AND(); }
"||"			{ return parser::make_OR(); }
"!"			{ return parser::make_NOT(); }
">="			{ return parser::make_GTE(); }
">"			{ return parser::make_GT(); }
"<="			{ return parser::make_LTE(); }
"<"			{ return parser::make_LT(); }
"!="			{ return parser::make_NE(); }
"="			{ return parser::make_ASSIGN(); }
"=="			{ return parser::make_EQ(); }
"+"			{ return parser::make_ADD(); }
"-"			{ return parser::make_SUB(); }
"*"			{ return parser::make_MUL(); }
"/"			{ return parser::make_DIV(); }
"%"			{ return parser::make_MOD(); }
"("			{ return parser::make_LPAREN(); }
")"			{ return parser::make_RPAREN(); }
"["			{ return parser::make_LBRACKET(); }
"]"			{ return parser::make_RBRACKET(); }
"{"			{ return parser::make_LBRACE(); }
"}"			{ return parser::make_RBRACE(); }
"."			{ return parser::make_DOT(); }
","			{ return parser::make_COMMA(); }
":"			{ return parser::make_COLON(); }
";"			{ return parser::make_SEMICOLON(); }

%%
