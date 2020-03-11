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
}

%code provides {
  #define YY_DECL \
    ::yy::parser::symbol_type yylex()
}

%code {
  extern YY_DECL;
  #define yyerror(x)
}

// Program structure.
%token CLASS
%token EXTENDS
%token LET
%token CONST
%token FUNCTION
%token EXTERN
%token EXPORT
%token OVERRIDE
// Control structures.
%token IF
%token ELSE
%token FOR
%token WHILE
%token CONTINUE
%token BREAK
%token RETURN
// Other classes of tokens.
%token <::std::string> IDENTIFIER
%token <int64_t> INTEGER_LITERAL
%token <::std::string> STRING_LITERAL
%token <bool> BOOLEAN_LITERAL
%token THIS
// Operators.
%token AND
%token OR
%token NOT
%token GE
%token GT
%token LE
%token LT
%token NE
%token ASSIGN
%token EQ
%token ADD
%token SUB
%token MUL
%token DIV
%token MOD
%token L_PAREN
%token R_PAREN
%token L_BRACKET
%token R_BRACKET
%token L_BRACE
%token R_BRACE
%token DOT
%token COMMA
%token COLON
%token SEMICOLON

%%

program: ;

%%

void ::yy::parser::error (const std::string& m) {
  ::std::cerr << m << ::std::endl;
}

