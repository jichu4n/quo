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
  #include "parser/quo_ast_types.hpp"
}

%code provides {
  #define YY_DECL \
    ::yy::parser::symbol_type yylex()
}

%code {
  extern YY_DECL;
  #define yyerror(x)
}

// ============================================================================
//   Tokens
// ============================================================================

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
%token NULL_LITERAL
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

// ============================================================================
//   Nonterminal symbols
// ============================================================================

%nterm <Expr*> expr
%nterm <Expr*> primary_expr
%nterm <LiteralExpr*> literal_expr
%nterm <VarExpr*> var_expr
%nterm <MemberExpr*> member_expr
%nterm <CallExpr*> call_expr
%nterm <::std::vector<Expr*>> expr_list

%%

expr:
    primary_expr;

primary_expr:
    literal_expr {
        $$ = __Expr_Create(
	    "literal"_Q,
	    $1,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr);
    }
    | var_expr {
        $$ = __Expr_Create(
	    "var"_Q,
	    nullptr,
	    $1,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr);
    }
    | member_expr {
        $$ = __Expr_Create(
	    "member"_Q,
	    nullptr,
	    nullptr,
	    $1,
	    nullptr,
	    nullptr,
	    nullptr);
    }
    | call_expr {
        $$ = __Expr_Create(
	    "call"_Q,
	    nullptr,
	    nullptr,
	    nullptr,
	    $1,
	    nullptr,
	    nullptr);
    }
    | L_PAREN expr R_PAREN {
        $$ = $2;
    }
    ;

literal_expr:
    INTEGER_LITERAL {
        $$ = __LiteralExpr_Create(
	    "int"_Q,
	    nullptr,
	    __QIntValue_Create($1));
    }
    | STRING_LITERAL {
        $$ = __LiteralExpr_Create(
	    "string"_Q,
	    __QStringValue_Create($1),
	    nullptr);
    }
    ;

var_expr:
    IDENTIFIER {
        $$ = __VarExpr_Create(__QStringValue_Create($1));
    }
    ;

member_expr:
    primary_expr DOT IDENTIFIER {
        $$ = __MemberExpr_Create($1, __QStringValue_Create($3));
    }
    ;

call_expr:
    primary_expr L_PAREN expr_list R_PAREN {
        $$ = __CallExpr_Create($1, __QArrayValue_CreateFromContainer($3));
    }
    ;

expr_list:
    %empty {
        $$ = ::std::vector<Expr*> {};
    }
    | expr {
        $$ = ::std::vector<Expr*> { $1 };
    }
    | expr COMMA expr_list {
        $$ = ::std::vector<Expr*> { $1 };
	$$.insert($$.end(), $3.begin(), $3.end());
    }
    ;

%%

void ::yy::parser::error (const std::string& m) {
  ::std::cerr << m << ::std::endl;
}

