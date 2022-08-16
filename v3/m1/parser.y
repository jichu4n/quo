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
}

%token CLASS ELSE FN IF IMPORT LET NEW RETURN WHILE
%token <::std::string> IDENTIFIER STRING_LITERAL
%token <int64_t> NUMBER_LITERAL
%token COLON SEMICOLON COMMA DOT LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token AND OR NOT ADD SUB EXP MUL DIV MOD EQ NE GTE LTE GT LT ASSIGN

%nterm <ModuleDef*> module
%nterm <ImportDecl*> importDecl
%nterm <FnDef*> fnDef fnSig voidFnSig
%nterm <Array<VarDecl*>*> params optParams
%nterm <VarDecl*> param
%nterm <Array<Stmt*>*> block
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
    /*
    | module classDef  {
        $$ = $1;
        $$->classDefs->add($2);
    }
    */
    | module fnDef  {
        $$ = $1;
        $$->fnDefs->add($2);
    }
    /*
    | module varDeclStmt  {
        $$ = $1;
        $$->varDecls->addAll($2->varDecls);
    }
    */
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

block:
    LBRACE RBRACE  { $$ = new Array<Stmt*>(); }
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
