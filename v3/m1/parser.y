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
  
  extern ModuleDef* Parse(String* input);
}

%code {
  extern YY_DECL;

  #define yyerror(x)

  ModuleDef* ParsedModuleDef;

  Expr* buildBinaryOpExpr(
      Expr* leftExpr, const ::std::string& op, Expr* rightExpr) {
    Expr* expr = new Expr();
    expr->type = new String("binaryOp");
    expr->binaryOpExpr = new BinaryOpExpr();
    expr->binaryOpExpr->leftExpr = leftExpr;
    expr->binaryOpExpr->op = new String(op);
    expr->binaryOpExpr->rightExpr = rightExpr;
    return expr;
  }

  Expr* buildUnaryOpExpr(
      const ::std::string& op, Expr* rightExpr) {
    Expr* expr = new Expr();
    expr->type = new String("unaryOp");
    expr->unaryOpExpr = new UnaryOpExpr();
    expr->unaryOpExpr->op = new String(op);
    expr->unaryOpExpr->rightExpr = rightExpr;
    return expr;
  }
}

%token CLASS ELSE FN IF IMPORT LET NEW RETURN WHILE
%token <::std::string> IDENTIFIER STRING_LITERAL
%token <int64_t> NUMBER_LITERAL
%token COLON SEMICOLON COMMA DOT LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token AND OR NOT ADD SUB EXP MUL DIV MOD EQ NE GTE LTE GT LT ASSIGN

%nterm <ModuleDef*> module
%nterm <ImportDecl*> importDecl
%nterm <FnDef*> fnDef fnSig voidFnSig
%nterm <Array<VarDecl*>*> params optParams propDecls varDecls
%nterm <VarDecl*> param propDecl varDecl
%nterm <Expr*> optInitExpr
%nterm <ClassDef*> classDef
%nterm <Array<Stmt*>*> block stmts
%nterm <Stmt*> stmt
%nterm <ExprStmt*> exprStmt
%nterm <IfStmt*> ifStmt
%nterm <Array<Stmt*>*> optElseBlock
%nterm <WhileStmt*> whileStmt
%nterm <ReturnStmt*> returnStmt
%nterm <VarDeclStmt*> varDeclStmt
%nterm <Expr*> expr lhsExpr
%nterm <Expr*> expr10 expr9 expr8 expr7 expr6 expr5 expr4 expr3
%nterm <Expr*> expr2 expr1 expr0
%nterm <Expr*> varRefExpr fnCallExpr stringLiteralExpr numberLiteralExpr
%nterm <Expr*> memberExpr subscriptExpr newExpr
%nterm <Array<Expr*>*> exprs optExprs
%nterm <::std::string> typeString

%%

start: module  { ParsedModuleDef = $1; }

module: %empty  {
        $$ = new ModuleDef();
        $$->name = new String("source");
        $$->importDecls = new Array<ImportDecl*>();
        $$->classDefs = new Array<ClassDef*>();
        $$->fnDefs = new Array<FnDef*>();
        $$->varDecls = new Array<VarDecl*>();
    }
    | module importDecl  {
        $$ = $1;
        $$->importDecls->add($2);
    }
    | module classDef  {
        $$ = $1;
        $$->classDefs->add($2);
    }
    | module fnDef  {
        $$ = $1;
        $$->fnDefs->add($2);
    }
    | module varDeclStmt  {
        $$ = $1;
        $$->varDecls->addAll($2->varDecls);
    }
    ;

importDecl:
    IMPORT STRING_LITERAL SEMICOLON  {
        $$ = new ImportDecl();
        $$->moduleName = new String($2);
    }
    ;

fnDef:
    fnSig block  {
        $$ = $1;
        $$->body = $2;
    }
    ;

fnSig:
    voidFnSig  { $$ = $1; }
    | voidFnSig COLON typeString  {
        $$ = $1;
        $$->returnTypeString = new String($3);
    }
    ;

voidFnSig:
    FN IDENTIFIER LPAREN optParams RPAREN   {
        $$ = new FnDef();
        $$->name = new String($2);
        $$->params = $4;
        $$->returnTypeString = nullptr;
    }
    ;

optParams:
    %empty  { $$ = new Array<VarDecl*>(); }
    | params  { $$ = $1; }
    ;

params:
    param  {
        $$ = new Array<VarDecl*>();
        $$->add($1);
    }
    | params COMMA param  {
        $$ = $1;
        $$->add($3);
    }
    ;

param:
    IDENTIFIER COLON typeString  {
        $$ = new VarDecl();
        $$->name = new String($1);
        $$->typeString = new String($3);
        $$->initExpr = nullptr;
    }
    ;

classDef:
    CLASS IDENTIFIER LBRACE propDecls RBRACE  {
          $$ = new ClassDef();
          $$->name = new String($2);
          $$->props = $4;
    }
    ;

propDecls:
    %empty  { $$ = new Array<VarDecl*>(); }
    | propDecls propDecl  { $$ = $1; $1->add($2); }
    ;

propDecl:
    varDecl SEMICOLON  { $$ = $1; }
    ;


block:
    LBRACE stmts RBRACE  { $$ = $2; }
    ;

stmts:
    %empty  { $$ = new Array<Stmt*>(); }
    | stmts stmt  { $$ = $1; $1->add($2); }
    ;

stmt:
    exprStmt  {
          $$ = new Stmt();
          $$->type = new String("expr");
          $$->exprStmt = $1;
    }
    | ifStmt  {
          $$ = new Stmt();
          $$->type = new String("if");
          $$->ifStmt = $1;
    }
    | whileStmt  {
          $$ = new Stmt();
          $$->type = new String("while");
          $$->whileStmt = $1;
    }
    | returnStmt  {
          $$ = new Stmt();
          $$->type = new String("return");
          $$->returnStmt = $1;
    }
    | varDeclStmt  {
          $$ = new Stmt();
          $$->type = new String("varDecl");
          $$->varDeclStmt = $1;
    }
    ;

exprStmt:
    expr SEMICOLON  {
        $$ = new ExprStmt();
        $$->expr = $1;
    }
    ;

ifStmt:
    IF LPAREN expr RPAREN block optElseBlock  {
        $$ = new IfStmt();
        $$->condExpr = $3;
        $$->ifBlock = $5;
        $$->elseBlock = $6;
    }
    ;

optElseBlock:
    %empty  { $$ = new Array<Stmt*>(); }
    | ELSE block  { $$ = $2; }
    ;

whileStmt:
    WHILE LPAREN expr RPAREN block  {
        $$ = new WhileStmt();
        $$->condExpr = $3;
        $$->block = $5;
    }
    ;

returnStmt:
    RETURN SEMICOLON  {
        $$ = new ReturnStmt();
        $$->valueExpr = nullptr;
    }
    | RETURN expr SEMICOLON  {
        $$ = new ReturnStmt();
        $$->valueExpr = $2;
    }
    ;

varDeclStmt:
    LET varDecls SEMICOLON {
        $$ = new VarDeclStmt();
        $$->varDecls = $2;
    }
    ;

varDecls:
    varDecl  {
        $$ = new Array<VarDecl*>();
        $$->add($1);
    }
    | varDecls COMMA varDecl {
        $$ = $1;
        $$->add($3);
    }
    ;

varDecl:
    IDENTIFIER COLON typeString optInitExpr  {
        $$ = new VarDecl();
        $$->name = new String($1);
        $$->typeString = new String($3);
        $$->initExpr = $4;
    }
    ;

optInitExpr:
    %empty  { $$ = nullptr; }
    | ASSIGN expr  { $$ = $2; }
    ;

expr: expr10  { $$ = $1; }
    ;

expr10: expr9  { $$ = $1; }
    | lhsExpr ASSIGN expr  {
        $$ = new Expr();
        $$->type = new String("assign");
        $$->assignExpr = new AssignExpr();
        $$->assignExpr->leftExpr = $1;
        $$->assignExpr->rightExpr = $3;
    }
    ;

expr9:
      expr8  { $$ = $1; }
    | expr9 OR expr8  { $$ = buildBinaryOpExpr($1, "||", $3); }
    ;

expr8:
      expr7  { $$ = $1; }
    | expr8 AND expr7  { $$ = buildBinaryOpExpr($1, "&&", $3); }
    ;

expr7:
      expr6  { $$ = $1; }
    | NOT expr6  { $$ = buildUnaryOpExpr("!", $2); }
    ;

expr6:
      expr5  { $$ = $1; }
    | expr6 EQ expr5  { $$ = buildBinaryOpExpr($1, "==", $3); }
    | expr6 NE expr5  { $$ = buildBinaryOpExpr($1, "!=", $3); }
    | expr6 GT expr5  { $$ = buildBinaryOpExpr($1, ">", $3); }
    | expr6 GTE expr5  { $$ = buildBinaryOpExpr($1, ">=", $3); }
    | expr6 LT expr5  { $$ = buildBinaryOpExpr($1, "<", $3); }
    | expr6 LTE expr5  { $$ = buildBinaryOpExpr($1, "<=", $3); }
    ;

expr5:
      expr4  { $$ = $1; }
    | expr5 ADD expr4  { $$ = buildBinaryOpExpr($1, "+", $3); }
    | expr5 SUB expr4  { $$ = buildBinaryOpExpr($1, "-", $3); }
    ;

expr4:
      expr3  { $$ = $1; }
    | expr4 MOD expr3  { $$ = buildBinaryOpExpr($1, "%", $3); }
    ;

expr3:
      expr2  { $$ = $1; }
    | expr3 MUL expr2  { $$ = buildBinaryOpExpr($1, "*", $3); }
    | expr3 DIV expr2  { $$ = buildBinaryOpExpr($1, "/", $3); }
    ;

expr2:
      expr1  { $$ = $1; }
    | SUB expr1  { $$ = buildUnaryOpExpr("-", $2); }
    ;

expr1:
      expr0  { $$ = $1; }
    | expr1 EXP expr0  { $$ = buildBinaryOpExpr($1, "**", $3); }
    ;

expr0:
      varRefExpr  { $$ = $1; }
    | fnCallExpr  { $$ = $1; }
    | stringLiteralExpr  { $$ = $1; }
    | numberLiteralExpr  { $$ = $1; }
    | memberExpr  { $$ = $1; }
    | subscriptExpr  { $$ = $1; }
    | LPAREN expr RPAREN  { $$ = $2; }
    | newExpr  { $$ = $1; }
    ;

lhsExpr:
      varRefExpr  { $$ = $1; }
    | memberExpr  { $$ = $1; }
    | subscriptExpr  { $$ = $1; }
    ;

varRefExpr:
    IDENTIFIER  {
        $$ = new Expr();
        $$->type = new String("varRef");
        $$->varRefExpr = new VarRefExpr();
        $$->varRefExpr->name = new String($1);
    }
    ;

fnCallExpr:
    expr0 LPAREN optExprs RPAREN  {
        $$ = new Expr();
        $$->type = new String("fnCall");
        $$->fnCallExpr = new FnCallExpr();
        $$->fnCallExpr->fnExpr = $1;
        $$->fnCallExpr->argExprs = $3;
    }
    ;

stringLiteralExpr:
    STRING_LITERAL  {
        $$ = new Expr();
        $$->type = new String("stringLiteral");
        $$->stringLiteralExpr = new StringLiteralExpr();
        $$->stringLiteralExpr->value = new String($1);
    }
    ;

numberLiteralExpr:
    NUMBER_LITERAL  {
        $$ = new Expr();
        $$->type = new String("numberLiteral");
        $$->numberLiteralExpr = new NumberLiteralExpr();
        $$->numberLiteralExpr->value = new Int64($1);
    }

subscriptExpr:
    expr0 LBRACKET expr RBRACKET  {
        $$ = new Expr();
        $$->type = new String("subscript");
        $$->subscriptExpr = new SubscriptExpr();
        $$->subscriptExpr->arrayExpr = $1;
        $$->subscriptExpr->indexExpr = $3;
    }
    ;

memberExpr:
    expr0 DOT IDENTIFIER  {
        $$ = new Expr();
        $$->type = new String("member");
        $$->memberExpr = new MemberExpr();
        $$->memberExpr->objExpr = $1;
        $$->memberExpr->fieldName = new String($3);
    }
    ;

newExpr:
    NEW typeString LPAREN RPAREN  {
        $$ = new Expr();
        $$->type = new String("new");
        $$->newExpr = new NewExpr();
        $$->newExpr->typeString = new String($2);
    }
    ;

optExprs:
    %empty  { $$ = new Array<Expr*>(); }
    | exprs  { $$ = $1; }
    ;

exprs:
    expr  {
        $$ = new Array<Expr*>();
        $$->add($1);
    }
    | exprs COMMA expr  {
        $$ = $1;
        $$->add($3);
    }
    ;


typeString:
      IDENTIFIER  { $$ = $1; }
    | IDENTIFIER LT typeString GT  {
        $$ = $1 + "<" + $3 + "*>";
    }
    ;

%% 

void ::yy::parser::error (const std::string& m) {
  ::std::cerr << m << ::std::endl;
}

extern void yy_scan_string(const char*);

ModuleDef* Parse(String* input) {
  yy_scan_string(input->c_str());
  ::yy::parser p;
  // p.set_debug_level(1);
  const int parse_result = p.parse();
  if (parse_result != 0) {
    ::std::cerr << "Parse failed";
    return nullptr;
  }
  return ParsedModuleDef;
}
