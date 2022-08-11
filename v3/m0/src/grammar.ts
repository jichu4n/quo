// Generated automatically by nearley, version 2.20.1
// http://github.com/Hardmath123/nearley
// Bypasses TS6133. Allow declared but unused functions.
// @ts-ignore
function id(d: any[]): any { return d[0]; }
declare var IMPORT: any;
declare var STRING_LITERAL: any;
declare var SEMICOLON: any;
declare var FN: any;
declare var IDENTIFIER: any;
declare var LPAREN: any;
declare var RPAREN: any;
declare var COLON: any;
declare var COMMA: any;
declare var CLASS: any;
declare var LBRACE: any;
declare var RBRACE: any;
declare var IF: any;
declare var ELSE: any;
declare var WHILE: any;
declare var RETURN: any;
declare var LET: any;
declare var ASSIGN: any;
declare var OR: any;
declare var AND: any;
declare var NOT: any;
declare var EQ: any;
declare var NE: any;
declare var GT: any;
declare var GTE: any;
declare var LT: any;
declare var LTE: any;
declare var ADD: any;
declare var SUB: any;
declare var MOD: any;
declare var MUL: any;
declare var DIV: any;
declare var EXP: any;
declare var NUMBER_LITERAL: any;
declare var LBRACKET: any;
declare var RBRACKET: any;
declare var DOT: any;
declare var NEW: any;

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


interface NearleyToken {
  value: any;
  [key: string]: any;
};

interface NearleyLexer {
  reset: (chunk: string, info: any) => void;
  next: () => NearleyToken | undefined;
  save: () => any;
  formatError: (token: never) => string;
  has: (tokenType: string) => boolean;
};

interface NearleyRule {
  name: string;
  symbols: NearleySymbol[];
  postprocess?: (d: any[], loc?: number, reject?: {}) => any;
};

type NearleySymbol = string | { literal: any } | { test: (token: any) => boolean };

interface Grammar {
  Lexer: NearleyLexer | undefined;
  ParserRules: NearleyRule[];
  ParserStart: string;
};

const grammar: Grammar = {
  Lexer: lexer,
  ParserRules: [
    {"name": "module", "symbols": [], "postprocess": 
        (): ModuleDef => ({
          name: 'source',
          importDecls: [],
          classDefs: [],
          fnDefs: [],
          varDecls: [],
        })
              },
    {"name": "module", "symbols": ["module", "importDecl"], "postprocess": 
        ([$1, $2]): ModuleDef => ({
          ...$1,
          importDecls: [...$1.importDecls, $2],
        })
              },
    {"name": "module", "symbols": ["module", "classDef"], "postprocess": 
        ([$1, $2]): ModuleDef => ({
          ...$1,
          classDefs: [...$1.classDefs, $2],
        })
              },
    {"name": "module", "symbols": ["module", "fnDef"], "postprocess": 
        ([$1, $2]): ModuleDef => ({
          ...$1,
          fnDefs: [...$1.fnDefs, $2],
        })
              },
    {"name": "module", "symbols": ["module", "varDeclStmt"], "postprocess": 
        ([$1, $2]): ModuleDef => ({
          ...$1,
          varDecls: [...$1.varDecls, ...$2.varDecls],
        })
              },
    {"name": "importDecl", "symbols": [(lexer.has("IMPORT") ? {type: "IMPORT"} : IMPORT), (lexer.has("STRING_LITERAL") ? {type: "STRING_LITERAL"} : STRING_LITERAL), (lexer.has("SEMICOLON") ? {type: "SEMICOLON"} : SEMICOLON)], "postprocess": 
        ([$1, $2, $3]): ImportDecl => ({
          moduleName: $2.value,
        })
            },
    {"name": "fnDef$ebnf$1$subexpression$1", "symbols": [(lexer.has("COLON") ? {type: "COLON"} : COLON), "typeString"]},
    {"name": "fnDef$ebnf$1", "symbols": ["fnDef$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "fnDef$ebnf$1", "symbols": [], "postprocess": () => null},
    {"name": "fnDef", "symbols": [(lexer.has("FN") ? {type: "FN"} : FN), (lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER), (lexer.has("LPAREN") ? {type: "LPAREN"} : LPAREN), "params", (lexer.has("RPAREN") ? {type: "RPAREN"} : RPAREN), "fnDef$ebnf$1", "block"], "postprocess": 
                ([$1, $2, $3, $4, $5, $6, $7]): FnDef => ({
                  name: $2.value,
                  params: $4,
        returnTypeString: $6 ? $6[1] : null,
                  body: $7,
                })
            },
    {"name": "params", "symbols": [], "postprocess": (): Array<VarDecl> => []},
    {"name": "params$ebnf$1", "symbols": []},
    {"name": "params$ebnf$1$subexpression$1", "symbols": ["param", (lexer.has("COMMA") ? {type: "COMMA"} : COMMA)]},
    {"name": "params$ebnf$1", "symbols": ["params$ebnf$1", "params$ebnf$1$subexpression$1"], "postprocess": (d) => d[0].concat([d[1]])},
    {"name": "params", "symbols": ["params$ebnf$1", "param"], "postprocess": 
        ([$1, $2]) => [...($1 ? $1.map(([$1_1, $1_2]: Array<any>) => $1_1) : []), $2]
            },
    {"name": "param", "symbols": [(lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER), (lexer.has("COLON") ? {type: "COLON"} : COLON), "typeString"], "postprocess": 
        ([$1, $2, $3]): VarDecl => ({
          name: $1.value,
          typeString: $3,
          initExpr: null,
        })
            },
    {"name": "classDef$ebnf$1", "symbols": []},
    {"name": "classDef$ebnf$1", "symbols": ["classDef$ebnf$1", "propDecl"], "postprocess": (d) => d[0].concat([d[1]])},
    {"name": "classDef", "symbols": [(lexer.has("CLASS") ? {type: "CLASS"} : CLASS), (lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER), (lexer.has("LBRACE") ? {type: "LBRACE"} : LBRACE), "classDef$ebnf$1", (lexer.has("RBRACE") ? {type: "RBRACE"} : RBRACE)], "postprocess": 
        ([$1, $2, $3, $4, $5]): ClassDef => ({
          name: $2.value,
          props: $4,
        })
            },
    {"name": "propDecl", "symbols": ["varDecl", (lexer.has("SEMICOLON") ? {type: "SEMICOLON"} : SEMICOLON)], "postprocess": id},
    {"name": "block", "symbols": [(lexer.has("LBRACE") ? {type: "LBRACE"} : LBRACE), "stmts", (lexer.has("RBRACE") ? {type: "RBRACE"} : RBRACE)], "postprocess": ([$1, $2, $3]) => $2},
    {"name": "stmts$ebnf$1", "symbols": []},
    {"name": "stmts$ebnf$1", "symbols": ["stmts$ebnf$1", "stmt"], "postprocess": (d) => d[0].concat([d[1]])},
    {"name": "stmts", "symbols": ["stmts$ebnf$1"], "postprocess": id},
    {"name": "stmt", "symbols": ["exprStmt"], "postprocess": id},
    {"name": "stmt", "symbols": ["ifStmt"], "postprocess": id},
    {"name": "stmt", "symbols": ["whileStmt"], "postprocess": id},
    {"name": "stmt", "symbols": ["returnStmt"], "postprocess": id},
    {"name": "stmt", "symbols": ["varDeclStmt"], "postprocess": id},
    {"name": "exprStmt", "symbols": ["expr", (lexer.has("SEMICOLON") ? {type: "SEMICOLON"} : SEMICOLON)], "postprocess": 
        ([$1, $2]): ExprStmt =>
        				    ({ type: StmtType.EXPR, expr: $1 })
        		},
    {"name": "ifStmt$ebnf$1$subexpression$1", "symbols": [(lexer.has("ELSE") ? {type: "ELSE"} : ELSE), "block"]},
    {"name": "ifStmt$ebnf$1", "symbols": ["ifStmt$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "ifStmt$ebnf$1", "symbols": [], "postprocess": () => null},
    {"name": "ifStmt", "symbols": [(lexer.has("IF") ? {type: "IF"} : IF), (lexer.has("LPAREN") ? {type: "LPAREN"} : LPAREN), "expr", (lexer.has("RPAREN") ? {type: "RPAREN"} : RPAREN), "block", "ifStmt$ebnf$1"], "postprocess": 
        		    ([$1, $2, $3, $4, $5, $6]): IfStmt => ({
        	type: StmtType.IF,
        	condExpr: $3,
        	ifBlock: $5,
        	elseBlock: $6 ? $6[1] : [],
        })
        		},
    {"name": "whileStmt", "symbols": [(lexer.has("WHILE") ? {type: "WHILE"} : WHILE), (lexer.has("LPAREN") ? {type: "LPAREN"} : LPAREN), "expr", (lexer.has("RPAREN") ? {type: "RPAREN"} : RPAREN), "block"], "postprocess": 
        		    ([$1, $2, $3, $4, $5]): WhileStmt => ({
        	type: StmtType.WHILE,
        	condExpr: $3,
        	block: $5,
        })
        		},
    {"name": "returnStmt$ebnf$1$subexpression$1", "symbols": ["expr"]},
    {"name": "returnStmt$ebnf$1", "symbols": ["returnStmt$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "returnStmt$ebnf$1", "symbols": [], "postprocess": () => null},
    {"name": "returnStmt", "symbols": [(lexer.has("RETURN") ? {type: "RETURN"} : RETURN), "returnStmt$ebnf$1", (lexer.has("SEMICOLON") ? {type: "SEMICOLON"} : SEMICOLON)], "postprocess": 
        ([$1, $2, $3]): ReturnStmt =>
            ({ type: StmtType.RETURN, valueExpr: $2 ? $2[0] : null })
            },
    {"name": "varDeclStmt", "symbols": [(lexer.has("LET") ? {type: "LET"} : LET), "varDecls", (lexer.has("SEMICOLON") ? {type: "SEMICOLON"} : SEMICOLON)], "postprocess": 
        ([$1, $2, $3]): VarDeclStmt => ({
          type: StmtType.VAR_DECL,
          varDecls: $2,
        })
            },
    {"name": "varDecls$ebnf$1", "symbols": []},
    {"name": "varDecls$ebnf$1$subexpression$1", "symbols": ["varDecl", (lexer.has("COMMA") ? {type: "COMMA"} : COMMA)]},
    {"name": "varDecls$ebnf$1", "symbols": ["varDecls$ebnf$1", "varDecls$ebnf$1$subexpression$1"], "postprocess": (d) => d[0].concat([d[1]])},
    {"name": "varDecls", "symbols": ["varDecls$ebnf$1", "varDecl"], "postprocess": 
        ([$1, $2]): Array<VarDecl> => [
          ...($1 ? $1.map(([$1_1, $1_2]: Array<any>) => $1_1) : []),
          $2,
        ]
            },
    {"name": "varDecl$ebnf$1$subexpression$1", "symbols": [(lexer.has("ASSIGN") ? {type: "ASSIGN"} : ASSIGN), "expr"]},
    {"name": "varDecl$ebnf$1", "symbols": ["varDecl$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "varDecl$ebnf$1", "symbols": [], "postprocess": () => null},
    {"name": "varDecl", "symbols": [(lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER), (lexer.has("COLON") ? {type: "COLON"} : COLON), "typeString", "varDecl$ebnf$1"], "postprocess": 
        ([$1, $2, $3, $4]): VarDecl => ({
          name: $1.value,
          typeString: $3,
          initExpr: $4 ? $4[1] : null,
        })
            },
    {"name": "expr", "symbols": ["expr10"], "postprocess": id},
    {"name": "expr10", "symbols": ["expr9"], "postprocess": id},
    {"name": "expr10", "symbols": ["lhsExpr", (lexer.has("ASSIGN") ? {type: "ASSIGN"} : ASSIGN), "expr"], "postprocess": 
        ([$1, $2, $3]): AssignExpr => ({
          type: ExprType.ASSIGN,
          leftExpr: $1,
          rightExpr: $3,
        })
             },
    {"name": "expr9", "symbols": ["expr8"], "postprocess": id},
    {"name": "expr9$subexpression$1", "symbols": [(lexer.has("OR") ? {type: "OR"} : OR)]},
    {"name": "expr9", "symbols": ["expr9", "expr9$subexpression$1", "expr8"], "postprocess": buildBinaryOpExpr},
    {"name": "expr8", "symbols": ["expr7"], "postprocess": id},
    {"name": "expr8$subexpression$1", "symbols": [(lexer.has("AND") ? {type: "AND"} : AND)]},
    {"name": "expr8", "symbols": ["expr8", "expr8$subexpression$1", "expr7"], "postprocess": buildBinaryOpExpr},
    {"name": "expr7", "symbols": ["expr6"], "postprocess": id},
    {"name": "expr7$subexpression$1", "symbols": [(lexer.has("NOT") ? {type: "NOT"} : NOT)]},
    {"name": "expr7", "symbols": ["expr7$subexpression$1", "expr6"], "postprocess": buildUnaryOpExpr},
    {"name": "expr6", "symbols": ["expr5"], "postprocess": id},
    {"name": "expr6$subexpression$1", "symbols": [(lexer.has("EQ") ? {type: "EQ"} : EQ)]},
    {"name": "expr6$subexpression$1", "symbols": [(lexer.has("NE") ? {type: "NE"} : NE)]},
    {"name": "expr6$subexpression$1", "symbols": [(lexer.has("GT") ? {type: "GT"} : GT)]},
    {"name": "expr6$subexpression$1", "symbols": [(lexer.has("GTE") ? {type: "GTE"} : GTE)]},
    {"name": "expr6$subexpression$1", "symbols": [(lexer.has("LT") ? {type: "LT"} : LT)]},
    {"name": "expr6$subexpression$1", "symbols": [(lexer.has("LTE") ? {type: "LTE"} : LTE)]},
    {"name": "expr6", "symbols": ["expr6", "expr6$subexpression$1", "expr5"], "postprocess": buildBinaryOpExpr},
    {"name": "expr5", "symbols": ["expr4"], "postprocess": id},
    {"name": "expr5$subexpression$1", "symbols": [(lexer.has("ADD") ? {type: "ADD"} : ADD)]},
    {"name": "expr5$subexpression$1", "symbols": [(lexer.has("SUB") ? {type: "SUB"} : SUB)]},
    {"name": "expr5", "symbols": ["expr5", "expr5$subexpression$1", "expr4"], "postprocess": buildBinaryOpExpr},
    {"name": "expr4", "symbols": ["expr3"], "postprocess": id},
    {"name": "expr4$subexpression$1", "symbols": [(lexer.has("MOD") ? {type: "MOD"} : MOD)]},
    {"name": "expr4", "symbols": ["expr4", "expr4$subexpression$1", "expr3"], "postprocess": buildBinaryOpExpr},
    {"name": "expr3", "symbols": ["expr2"], "postprocess": id},
    {"name": "expr3$subexpression$1", "symbols": [(lexer.has("MUL") ? {type: "MUL"} : MUL)]},
    {"name": "expr3$subexpression$1", "symbols": [(lexer.has("DIV") ? {type: "DIV"} : DIV)]},
    {"name": "expr3", "symbols": ["expr3", "expr3$subexpression$1", "expr2"], "postprocess": buildBinaryOpExpr},
    {"name": "expr2", "symbols": ["expr1"], "postprocess": id},
    {"name": "expr2$subexpression$1", "symbols": [(lexer.has("SUB") ? {type: "SUB"} : SUB)]},
    {"name": "expr2", "symbols": ["expr2$subexpression$1", "expr1"], "postprocess": buildUnaryOpExpr},
    {"name": "expr1", "symbols": ["expr0"], "postprocess": id},
    {"name": "expr1$subexpression$1", "symbols": [(lexer.has("EXP") ? {type: "EXP"} : EXP)]},
    {"name": "expr1", "symbols": ["expr1", "expr1$subexpression$1", "expr0"], "postprocess": buildBinaryOpExpr},
    {"name": "expr0", "symbols": ["varRefExpr"], "postprocess": id},
    {"name": "expr0", "symbols": ["fnCallExpr"], "postprocess": id},
    {"name": "expr0", "symbols": ["literalExpr"], "postprocess": id},
    {"name": "expr0", "symbols": ["memberExpr"], "postprocess": id},
    {"name": "expr0", "symbols": ["subscriptExpr"], "postprocess": id},
    {"name": "expr0", "symbols": [(lexer.has("LPAREN") ? {type: "LPAREN"} : LPAREN), "expr", (lexer.has("RPAREN") ? {type: "RPAREN"} : RPAREN)], "postprocess": ([$1, $2, $3]): Expr => $2},
    {"name": "expr0", "symbols": ["newExpr"], "postprocess": id},
    {"name": "lhsExpr", "symbols": ["varRefExpr"], "postprocess": id},
    {"name": "lhsExpr", "symbols": ["memberExpr"], "postprocess": id},
    {"name": "lhsExpr", "symbols": ["subscriptExpr"], "postprocess": id},
    {"name": "varRefExpr", "symbols": [(lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER)], "postprocess": 
        ([$1]): VarRefExpr =>
            ({ type: ExprType.VAR_REF, name: $1.value })
            },
    {"name": "fnCallExpr", "symbols": [(lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER), (lexer.has("LPAREN") ? {type: "LPAREN"} : LPAREN), "exprs", (lexer.has("RPAREN") ? {type: "RPAREN"} : RPAREN)], "postprocess": 
        ([$1, $2, $3, $4]): FnCallExpr => ({
          type: ExprType.FN_CALL,
          name: $1.value,
          argExprs: $3,
        })
            },
    {"name": "literalExpr", "symbols": ["stringLiteralExpr"], "postprocess": id},
    {"name": "literalExpr", "symbols": ["numberLiteralExpr"], "postprocess": id},
    {"name": "stringLiteralExpr", "symbols": [(lexer.has("STRING_LITERAL") ? {type: "STRING_LITERAL"} : STRING_LITERAL)], "postprocess": 
        ([$1]): StringLiteralExpr =>
            ({ type: ExprType.STRING_LITERAL, value: $1.value })
            },
    {"name": "numberLiteralExpr", "symbols": [(lexer.has("NUMBER_LITERAL") ? {type: "NUMBER_LITERAL"} : NUMBER_LITERAL)], "postprocess": 
        ([$1]): NumberLiteralExpr =>
            ({ type: ExprType.NUMBER_LITERAL, value: $1.value })
            },
    {"name": "subscriptExpr$ebnf$1$subexpression$1", "symbols": [(lexer.has("LBRACKET") ? {type: "LBRACKET"} : LBRACKET), "expr", (lexer.has("RBRACKET") ? {type: "RBRACKET"} : RBRACKET)]},
    {"name": "subscriptExpr$ebnf$1", "symbols": ["subscriptExpr$ebnf$1$subexpression$1"]},
    {"name": "subscriptExpr$ebnf$1$subexpression$2", "symbols": [(lexer.has("LBRACKET") ? {type: "LBRACKET"} : LBRACKET), "expr", (lexer.has("RBRACKET") ? {type: "RBRACKET"} : RBRACKET)]},
    {"name": "subscriptExpr$ebnf$1", "symbols": ["subscriptExpr$ebnf$1", "subscriptExpr$ebnf$1$subexpression$2"], "postprocess": (d) => d[0].concat([d[1]])},
    {"name": "subscriptExpr", "symbols": ["expr0", "subscriptExpr$ebnf$1"], "postprocess": 
        ([$1, $2, $3, $4]): SubscriptExpr => ({
          type: ExprType.SUBSCRIPT,
          arrayExpr: $1,
          indexExprs: $3.map(([$3_1, $3_2, $3_3]: Array<any>) => $3_2),
        })
            },
    {"name": "memberExpr", "symbols": ["expr0", (lexer.has("DOT") ? {type: "DOT"} : DOT), (lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER)], "postprocess": 
        ([$1, $2, $3]): MemberExpr => ({
          type: ExprType.MEMBER,
          objExpr: $1,
          fieldName: $3.value,
        })
            },
    {"name": "newExpr", "symbols": [(lexer.has("NEW") ? {type: "NEW"} : NEW), "typeString", (lexer.has("LPAREN") ? {type: "LPAREN"} : LPAREN), (lexer.has("RPAREN") ? {type: "RPAREN"} : RPAREN)], "postprocess": 
        ([$1, $2, $3, $4]): NewExpr => ({
          type: ExprType.NEW,
          typeString: $2,
        })
            },
    {"name": "exprs", "symbols": [], "postprocess": (): Array<Expr> => []},
    {"name": "exprs$ebnf$1", "symbols": []},
    {"name": "exprs$ebnf$1$subexpression$1", "symbols": ["expr", (lexer.has("COMMA") ? {type: "COMMA"} : COMMA)]},
    {"name": "exprs$ebnf$1", "symbols": ["exprs$ebnf$1", "exprs$ebnf$1$subexpression$1"], "postprocess": (d) => d[0].concat([d[1]])},
    {"name": "exprs", "symbols": ["exprs$ebnf$1", "expr"], "postprocess": 
        ([$1, $2]): Array<Expr> =>
            [...($1 ? $1.map(([$1_1, $1_2]: Array<any>) => $1_1) : []), $2]
              },
    {"name": "typeString", "symbols": [(lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER)], "postprocess": ([$1]) => $1.value},
    {"name": "typeString", "symbols": [(lexer.has("IDENTIFIER") ? {type: "IDENTIFIER"} : IDENTIFIER), (lexer.has("LT") ? {type: "LT"} : LT), "typeString", (lexer.has("GT") ? {type: "GT"} : GT)], "postprocess": 
        ([$1, $2, $3, $4]) => `${$1.value}<${$3}*>`
        		}
  ],
  ParserStart: "module",
};

export default grammar;
