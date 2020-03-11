%{
#include "quo_parser.hpp"
#include <cstdlib>

using namespace yy;
%}

%option noyywrap nounput noinput batch

%%

"//"[^\n]*		/* Discard comment. */
[ \t\n]+		/* Discard whitespace. */

  /* Program structure. */
"class"			{ return parser::make_CLASS(); }
"extends"		{ return parser::make_EXTENDS(); }
"let"			{ return parser::make_LET(); }
"const"			{ return parser::make_CONST(); }
"fn"			{ return parser::make_FUNCTION(); }
"extern"		{ return parser::make_EXTERN(); }
"export"		{ return parser::make_EXPORT(); }
"override"		{ return parser::make_OVERRIDE(); }
  /* Control structures. */
"if"			{ return parser::make_IF(); }
"else"			{ return parser::make_ELSE(); }
"for"			{ return parser::make_FOR(); }
"while"			{ return parser::make_WHILE(); }
"continue"		{ return parser::make_CONTINUE(); }
"break"			{ return parser::make_BREAK(); }
"return"		{ return parser::make_RETURN(); }
  /* Other classes of tokens. */
[_a-zA-Z][_a-zA-Z0-9]*	{ return parser::make_IDENTIFIER(yytext); }
[0-9]+			{ return parser::make_INTEGER_LITERAL(strtoll(yytext, nullptr, 10)); }
'([^\\']|\\.)*'		{ return parser::make_STRING_LITERAL(::std::string(yytext + 1, yyleng - 2)); }
\"([^\\\"]|\\.)*\"	{ return parser::make_STRING_LITERAL(::std::string(yytext + 1, yyleng - 2)); }
"true"			{ return parser::make_BOOLEAN_LITERAL(true); }
"false"			{ return parser::make_BOOLEAN_LITERAL(false); }
"this"			{ return parser::make_THIS(); }
  /* Operators. */
"&&"			{ return parser::make_AND(); }
"||"			{ return parser::make_OR(); }
"!"			{ return parser::make_NOT(); }
">="			{ return parser::make_GE(); }
">"			{ return parser::make_GT(); }
"<="			{ return parser::make_LE(); }
"<"			{ return parser::make_LT(); }
"!="			{ return parser::make_NE(); }
"="			{ return parser::make_ASSIGN(); }
"=="			{ return parser::make_EQ(); }
"+"			{ return parser::make_ADD(); }
"-"			{ return parser::make_SUB(); }
"*"			{ return parser::make_MUL(); }
"/"			{ return parser::make_DIV(); }
"%"			{ return parser::make_MOD(); }
"("			{ return parser::make_L_PAREN(); }
")"			{ return parser::make_R_PAREN(); }
"["			{ return parser::make_L_BRACKET(); }
"]"			{ return parser::make_R_BRACKET(); }
"{"			{ return parser::make_L_BRACE(); }
"}"			{ return parser::make_R_BRACE(); }
"."			{ return parser::make_DOT(); }
","			{ return parser::make_COMMA(); }
":"			{ return parser::make_COLON(); }
";"			{ return parser::make_SEMICOLON(); }

%%
