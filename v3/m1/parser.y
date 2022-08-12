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
