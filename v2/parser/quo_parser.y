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


  namespace parser_testing {
  /** For testing - enables ONLY_PARSE_EXPR_FOR_TEST. */
  extern bool ShouldOnlyParseExprForTest;
  /** For testing - the parsed Expr if ONLY_PARSE_EXPR_FOR_TEST. */
  extern Expr* ParsedExprForTest;
  /** For testing - enables ONLY_PARSE_STMTS_FOR_TEST. */
  extern bool ShouldOnlyParseStmtsForTest;
  /** For testing - the parsed Stmt if ONLY_PARSE_STMTS_FOR_TEST. */
  extern ::std::vector<Stmt*> ParsedStmtsForTest;
  }
}

%code {
  extern YY_DECL;

  #define yyerror(x)

  namespace parser_testing {
  bool ShouldOnlyParseExprForTest;
  Expr* ParsedExprForTest;
  bool ShouldOnlyParseStmtsForTest;
  ::std::vector<Stmt*> ParsedStmtsForTest;
  }

  inline Expr* CreateExprWithBinaryOpExpr(
      QStringValue* op, Expr* left, Expr* right) {
    return __Expr_Create(
	"binaryOp"_Q,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	__BinaryOpExpr_Create(op, left, right),
	nullptr);
  }
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

// Emit this token as the first token to only parse expressions.
%token ONLY_PARSE_EXPR_FOR_TEST
// Emit this token as the first token to only parse statements.
%token ONLY_PARSE_STMTS_FOR_TEST

%right ASSIGN
%left OR
%left AND
%left ADD SUB
%left MUL DIV MOD

// ============================================================================
//   Nonterminal symbols
// ============================================================================

%nterm <Expr*> expr
%nterm <Expr*> primary_expr
%nterm <Expr*> assignable_primary_expr
%nterm <LiteralExpr*> literal_expr
%nterm <VarExpr*> var_expr
%nterm <MemberExpr*> member_expr
%nterm <CallExpr*> call_expr
%nterm <::std::vector<Expr*>> expr_list
%nterm <Expr*> arith_binary_op_expr
%nterm <Expr*> comp_binary_op_expr
%nterm <QStringValue*> comp_binary_op
%nterm <Expr*> bool_binary_op_expr
%nterm <Expr*> assign_expr

%nterm <::std::vector<Stmt*>> stmts
%nterm <Block*> block
%nterm <Stmt*> stmt
%nterm <Stmt*> expr_stmt
%nterm <Stmt*> ret_stmt
%nterm <Stmt*> cond_stmt
%nterm <Stmt*> cond_loop_stmt
%nterm <::std::vector<Stmt*>> var_decl_stmt
%nterm <::std::vector<::std::string>> var_name_list

%nterm <QStringValue*> type_spec;

%%

start:
    ONLY_PARSE_EXPR_FOR_TEST expr {
        ::parser_testing::ParsedExprForTest = $2;
    }
    | ONLY_PARSE_STMTS_FOR_TEST stmts {
        ::parser_testing::ParsedStmtsForTest = $2;
    }
    ;

expr:
    bool_binary_op_expr
    | assign_expr
    ;

assignable_primary_expr:
    var_expr {
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
    };

primary_expr:
    assignable_primary_expr
    | literal_expr {
        $$ = __Expr_Create(
	    "literal"_Q,
	    $1,
	    nullptr,
	    nullptr,
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

arith_binary_op_expr:
    primary_expr {
        $$ = $1;
    }
    | arith_binary_op_expr ADD arith_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("add"_Q, $1, $3);
    }
    | arith_binary_op_expr SUB arith_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("sub"_Q, $1, $3);
    }
    | arith_binary_op_expr MUL arith_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("mul"_Q, $1, $3);
    }
    | arith_binary_op_expr DIV arith_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("div"_Q, $1, $3);
    }
    | arith_binary_op_expr MOD arith_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("mod"_Q, $1, $3);
    }
    ;

comp_binary_op_expr:
    arith_binary_op_expr {
        $$ = $1;
    }
    | arith_binary_op_expr comp_binary_op arith_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr($2, $1, $3);
    }
    ;

comp_binary_op:
    GE { $$ = "ge"_Q; }
    | GT { $$ = "gt"_Q; }
    | LE { $$ = "le"_Q; }
    | LT { $$ = "lt"_Q; }
    | NE { $$ = "ne"_Q; }
    | EQ { $$ = "eq"_Q; }
    ;

bool_binary_op_expr:
    comp_binary_op_expr {
        $$ = $1;
    }
    | bool_binary_op_expr AND bool_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("and"_Q, $1, $3);
    }
    | bool_binary_op_expr OR bool_binary_op_expr {
        $$ = CreateExprWithBinaryOpExpr("or"_Q, $1, $3);
    }
    ;

assign_expr:
    assignable_primary_expr ASSIGN expr {
        $$ = __Expr_Create(
	    "assign"_Q,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    __AssignExpr_Create($1, $3));
    }
    ;

block:
    L_BRACE stmts R_BRACE {
        $$ = __Block_Create(__QArrayValue_CreateFromContainer($2));
    }
    ;

stmts:
    %empty {
        $$ = ::std::vector<Stmt*> {};
    }
    | stmts stmt {
        $$ = ::std::vector<Stmt*> { $1 };
	$$.push_back($2);
    }
    | stmts var_decl_stmt {
        $$ = ::std::vector<Stmt*> { $1 };
	$$.insert($1.end(), $2.begin(), $2.end());
    }
    | stmts SEMICOLON {
        $$ = ::std::vector<Stmt*> { $1 };
    }
    ;

stmt:
    expr_stmt
    | ret_stmt
    | cond_stmt
    | cond_loop_stmt
    ;

expr_stmt:
    expr SEMICOLON {
        $$ = __Stmt_Create(
	    "expr"_Q,
            __ExprStmt_Create($1),
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr);
    }
    ;

ret_stmt:
    RETURN SEMICOLON {
        $$ = __Stmt_Create(
	    "ret"_Q,
	    nullptr,
	    __RetStmt_Create(nullptr),
	    nullptr,
	    nullptr,
	    nullptr);
    }
    | RETURN expr SEMICOLON {
        $$ = __Stmt_Create(
	    "ret"_Q,
	    nullptr,
	    __RetStmt_Create($2),
	    nullptr,
	    nullptr,
	    nullptr);
    }
    ;

cond_stmt:
    IF L_PAREN expr R_PAREN block {
        $$ = __Stmt_Create(
	    "cond"_Q,
	    nullptr,
	    nullptr,
	    __CondStmt_Create($3, $5, __Block_Create(__QArrayValue_Create())),
	    nullptr,
	    nullptr);
    }
    | IF L_PAREN expr R_PAREN block ELSE block {
        $$ = __Stmt_Create(
	    "cond"_Q,
	    nullptr,
	    nullptr,
	    __CondStmt_Create($3, $5, $7),
	    nullptr,
	    nullptr);
    }
    | IF L_PAREN expr R_PAREN block ELSE cond_stmt {
	const ::std::vector<Stmt*> false_block { $7 };
        $$ = __Stmt_Create(
	    "cond"_Q,
	    nullptr,
	    nullptr,
            __CondStmt_Create(
		$3, $5, __Block_Create(__QArrayValue_CreateFromContainer(false_block))),
	    nullptr,
	    nullptr);
    }
    ;

cond_loop_stmt:
    WHILE L_PAREN expr R_PAREN block {
        $$ = __Stmt_Create(
	    "condLoop"_Q,
	    nullptr,
	    nullptr,
	    nullptr,
	    __CondLoopStmt_Create($3, $5),
	    nullptr);
    }
    ;

var_decl_stmt:
    LET var_name_list COLON type_spec SEMICOLON {
        $$ = ::std::vector<Stmt*> {};
	for (const auto& var_name : $2) {
	  $$.push_back(
	      __Stmt_Create(
	          "varDecl"_Q,
		  nullptr,
		  nullptr,
		  nullptr,
		  nullptr,
		  __VarDeclStmt_Create(__QStringValue_Create(var_name), $4)));

	}
    }
    ;

var_name_list:
    IDENTIFIER {
        $$ = ::std::vector<::std::string> { $1 };
    }
    | var_name_list IDENTIFIER {
        $$ = ::std::vector<::std::string> { $1 };
	$$.push_back($2);
    }
    ;

type_spec:
    IDENTIFIER {
        $$ = __QStringValue_Create($1);
    }
    ;

%%

void ::yy::parser::error (const std::string& m) {
  ::std::cerr << m << ::std::endl;
}

