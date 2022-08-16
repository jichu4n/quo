@preprocessor typescript

@{%
import _ from 'lodash';
import {Token} from 'moo';
import lexer from './lexer';
import {
  ModuleDef,
  Expr,
  ExprType,
  BinaryOpExpr,
  UnaryOpExpr,
  StringLiteralExpr,
  NumberLiteralExpr,
  VarRefExpr,
  FnCallExpr,
  MemberExpr,
  SubscriptExpr,
  AssignExpr,
  NewExpr,
  Stmt,
  Stmts,
  StmtType,
  VarDecl,
  VarDeclStmt,
  ExprStmt,
  IfStmt,
  WhileStmt,
  ReturnStmt,
  ImportDecl,
	FnDef,
	ClassDef,
} from './ast';

// ----
// Helper functions.
// ----

function discard() { return null; }

function buildBinaryOpExpr([$1, $2, $3]: Array<any>): BinaryOpExpr {
  return {
    type: ExprType.BINARY_OP,
    op: id($2).type.toLowerCase(),
    leftExpr: $1,
    rightExpr: $3,
  };
}

function buildUnaryOpExpr([$1, $2]: Array<any>): UnaryOpExpr {
  return {
    type: ExprType.UNARY_OP,
    op: id($1).type.toLowerCase(),
    rightExpr: $2,
  };
}

/** Type of the 'reject' parameter passed to postprocessors. */
type Reject = Object | undefined;

// ----
// Generated grammer below
// ----

%}

@lexer lexer

# ----
# Program structure
# ----

module ->
      null  {%
          (): ModuleDef => ({
            name: 'source',
            importDecls: [],
            classDefs: [],
            fnDefs: [],
            varDecls: [],
          })
      %}
    | module importDecl  {%
        ([$1, $2]): ModuleDef => ({
          ...$1,
          importDecls: [...$1.importDecls, $2],
        })
      %}
    | module classDef  {%
        ([$1, $2]): ModuleDef => ({
          ...$1,
          classDefs: [...$1.classDefs, $2],
        })
      %}
    | module fnDef  {%
        ([$1, $2]): ModuleDef => ({
          ...$1,
          fnDefs: [...$1.fnDefs, $2],
        })
      %}
    | module varDeclStmt  {%
        ([$1, $2]): ModuleDef => ({
          ...$1,
          varDecls: [...$1.varDecls, ...$2.varDecls],
        })
      %}

importDecl ->
    %IMPORT %STRING_LITERAL %SEMICOLON  {%
        ([$1, $2, $3]): ImportDecl => ({
          moduleName: $2.value,
        })
    %}

fnDef ->
    %FN %IDENTIFIER %LPAREN params %RPAREN (%COLON typeString):? block  {%
        ([$1, $2, $3, $4, $5, $6, $7]): FnDef => ({
          name: $2.value,
          params: $4,
					returnTypeString: $6 ? $6[1] : null,
          body: $7,
        })
    %}

params ->
      null  {% (): Array<VarDecl> => [] %}
    | (param %COMMA):* param  {%
        ([$1, $2]) => [...($1 ? $1.map(([$1_1, $1_2]: Array<any>) => $1_1) : []), $2]
    %}

param ->
      %IDENTIFIER %COLON typeString  {%
          ([$1, $2, $3]): VarDecl => ({
            name: $1.value,
            typeString: $3,
            initExpr: null,
          })
    %}

classDef ->
    %CLASS %IDENTIFIER %LBRACE propDecl:* %RBRACE  {%
          ([$1, $2, $3, $4, $5]): ClassDef => ({
            name: $2.value,
            props: $4,
          })
    %}

propDecl ->
    varDecl %SEMICOLON  {% id %}


# ----
# Statements
# ----
block -> %LBRACE stmts %RBRACE  {% ([$1, $2, $3]) => $2 %}

stmts ->
    stmt:*  {% id %}

stmt ->
      exprStmt  {% id %}
    | ifStmt  {% id %}
    | whileStmt  {% id %}
    | returnStmt  {% id %}
    | varDeclStmt  {% id %}

exprStmt ->
    expr %SEMICOLON  {%
		    ([$1, $2]): ExprStmt =>
				    ({ type: StmtType.EXPR, expr: $1 })
		%}

ifStmt ->
    %IF %LPAREN expr %RPAREN block (%ELSE block):?  {%
		    ([$1, $2, $3, $4, $5, $6]): IfStmt => ({
					type: StmtType.IF,
					condExpr: $3,
					ifBlock: $5,
					elseBlock: $6 ? $6[1] : [],
				})
		%}

whileStmt ->
    %WHILE %LPAREN expr %RPAREN block  {%
		    ([$1, $2, $3, $4, $5]): WhileStmt => ({
					type: StmtType.WHILE,
					condExpr: $3,
					block: $5,
				})
		%}

returnStmt ->
    %RETURN (expr):? %SEMICOLON  {%
        ([$1, $2, $3]): ReturnStmt =>
            ({ type: StmtType.RETURN, valueExpr: $2 ? $2[0] : null })
    %}

varDeclStmt ->
    %LET varDecls %SEMICOLON {%
        ([$1, $2, $3]): VarDeclStmt => ({
          type: StmtType.VAR_DECL,
          varDecls: $2,
        })
    %}

varDecls ->
    (varDecl %COMMA):* varDecl {%
        ([$1, $2]): Array<VarDecl> => [
          ...($1 ? $1.map(([$1_1, $1_2]: Array<any>) => $1_1) : []),
          $2,
        ]
    %}

varDecl ->
    %IDENTIFIER %COLON typeString (%ASSIGN expr):? {%
        ([$1, $2, $3, $4]): VarDecl => ({
          name: $1.value,
          typeString: $3,
          initExpr: $4 ? $4[1] : null,
        })
    %}


# ----
# Expressions
# ----

# An expression.
expr ->
    expr10  {% id %}

expr10 ->
      expr9  {% id %}
    | lhsExpr %ASSIGN expr  {%
        ([$1, $2, $3]): AssignExpr => ({
          type: ExprType.ASSIGN,
          leftExpr: $1,
          rightExpr: $3,
        })
     %}

expr9 ->
      expr8  {% id %}
    | expr9 (%OR) expr8  {% buildBinaryOpExpr %}

expr8 ->
      expr7  {% id %}
    | expr8 (%AND) expr7  {% buildBinaryOpExpr %}

expr7 ->
      expr6  {% id %}
    | (%NOT) expr6  {% buildUnaryOpExpr %}

expr6 ->
      expr5  {% id %}
    | expr6 (%EQ | %NE | %GT | %GTE | %LT | %LTE) expr5  {% buildBinaryOpExpr %}

expr5 ->
      expr4  {% id %}
    | expr5 (%ADD | %SUB) expr4  {% buildBinaryOpExpr %}

expr4 ->
      expr3  {% id %}
    | expr4 (%MOD) expr3  {% buildBinaryOpExpr %}

expr3 ->
      expr2  {% id %}
    | expr3 (%MUL | %DIV) expr2  {% buildBinaryOpExpr %}

expr2 ->
      expr1  {% id %}
    | (%SUB) expr1  {% buildUnaryOpExpr %}

expr1 ->
      expr0  {% id %}
    | expr1 (%EXP) expr0  {% buildBinaryOpExpr %}

expr0 ->
      varRefExpr  {% id %}
    | fnCallExpr  {% id %}
    | literalExpr  {% id %}
    | memberExpr  {% id %}
    | subscriptExpr  {% id %}
    | %LPAREN expr %RPAREN  {% ([$1, $2, $3]): Expr => $2 %}
    | newExpr  {% id %}

lhsExpr ->
      varRefExpr  {% id %}
    | memberExpr  {% id %}
    | subscriptExpr  {% id %}

varRefExpr ->
    %IDENTIFIER  {%
        ([$1]): VarRefExpr =>
            ({ type: ExprType.VAR_REF, name: $1.value })
    %}

fnCallExpr ->
    expr0 %LPAREN exprs %RPAREN  {%
        ([$1, $2, $3, $4]): FnCallExpr => ({
          type: ExprType.FN_CALL,
          fnExpr: $1,
          argExprs: $3,
        })
    %}

literalExpr ->
      stringLiteralExpr  {% id %}
    | numberLiteralExpr  {% id %}

stringLiteralExpr ->
    %STRING_LITERAL  {%
        ([$1]): StringLiteralExpr =>
            ({ type: ExprType.STRING_LITERAL, value: $1.value })
    %}

numberLiteralExpr ->
    %NUMBER_LITERAL  {%
        ([$1]): NumberLiteralExpr =>
            ({ type: ExprType.NUMBER_LITERAL, value: $1.value })
    %}

subscriptExpr ->
    expr0 (%LBRACKET expr %RBRACKET):+  {%
        ([$1, $2, $3, $4]): SubscriptExpr => ({
          type: ExprType.SUBSCRIPT,
          arrayExpr: $1,
          indexExprs: $2.map(([$2_1, $2_2, $2_3]: Array<any>) => $2_2),
        })
    %}

memberExpr ->
    expr0 %DOT %IDENTIFIER  {%
        ([$1, $2, $3]): MemberExpr => ({
          type: ExprType.MEMBER,
          objExpr: $1,
          fieldName: $3.value,
        })
    %}

newExpr ->
    %NEW typeString %LPAREN %RPAREN  {%
        ([$1, $2, $3, $4]): NewExpr => ({
          type: ExprType.NEW,
          typeString: $2,
        })
    %}

exprs ->
      null  {% (): Array<Expr> => [] %}
    | (expr %COMMA):* expr  {%
          ([$1, $2]): Array<Expr> =>
              [...($1 ? $1.map(([$1_1, $1_2]: Array<any>) => $1_1) : []), $2]
      %}

# ----
# Expressions
# ----

typeString ->
      %IDENTIFIER  {% ([$1]) => $1.value %}
		| %IDENTIFIER %LT typeString %GT  {%
		      ([$1, $2, $3, $4]) => `${$1.value}<${$3}*>`
		%}
