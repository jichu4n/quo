export enum ExprType {
  STRING_LITERAL = 'stringLiteral',
  NUMBER_LITERAL = 'numberLiteral',
  VAR_REF = 'varRef',
  FN_CALL = 'fnCall',
  BINARY_OP = 'binaryOp',
  UNARY_OP = 'unaryOp',
  SUBSCRIPT = 'subscript',
  MEMBER = 'member',
  ASSIGN = 'assign',
}

export type Expr =
  | StringLiteralExpr
  | NumberLiteralExpr
  | VarRefExpr
  | FnCallExpr
  | BinaryOpExpr
  | UnaryOpExpr
  | MemberExpr
  | SubscriptExpr
  | AssignExpr;

export interface StringLiteralExpr {
  type: ExprType.STRING_LITERAL;
  value: string;
}

export interface NumberLiteralExpr {
  type: ExprType.NUMBER_LITERAL;
  value: number;
}

export interface VarRefExpr {
  type: ExprType.VAR_REF;
  name: string;
}

export interface FnCallExpr {
  type: ExprType.FN_CALL;
  name: string;
  argExprs: Array<Expr>;
}

export interface MemberExpr {
  type: ExprType.MEMBER;
  objExpr: Expr;
  fieldName: string;
}

export interface SubscriptExpr {
  type: ExprType.SUBSCRIPT;
  arrayExpr: Expr;
  indexExprs: Array<Expr>;
}

export interface BinaryOpExpr {
  type: ExprType.BINARY_OP;
  leftExpr: Expr;
  op: string;
  rightExpr: Expr;
}

export interface UnaryOpExpr {
  type: ExprType.UNARY_OP;
  op: string;
  rightExpr: Expr;
}

export interface AssignExpr {
  type: ExprType.ASSIGN;
  leftExpr: Expr;
  rightExpr: Expr;
}

export enum StmtType {
  EXPR = 'expr',
  RETURN = 'return',
  IF = 'if',
  WHILE = 'while',
  VAR_DECL = 'varDecl',
}

export interface ExprStmt {
  type: StmtType.EXPR;
  expr: Expr;
}

export interface ReturnStmt {
  type: StmtType.RETURN;
  valueExpr: Expr;
}

export interface IfStmt {
  type: StmtType.IF;
  condExpr: Expr;
  ifBlock: Stmts;
  elseBlock: Stmts;
}

export interface WhileStmt {
  type: StmtType.WHILE;
  condExpr: Expr;
  block: Stmts;
}

export interface VarDecl {
  name: string;
  typeString: string;
  initExpr: Expr | null;
}

export interface VarDeclStmt {
  type: StmtType.VAR_DECL;
  varDecls: Array<VarDecl>;
}

export type Stmt = ExprStmt | ReturnStmt | IfStmt | WhileStmt | VarDeclStmt;

export type Stmts = Array<Stmt>;

export interface ImportDecl {
  moduleName: string;
}

export interface FnDef {
  name: string;
  params: Array<VarDecl>;
  returnTypeString: string;
  body: Stmts;
}

export interface ClassDef {
  name: string;
  props: Array<VarDecl>;
}

export interface ModuleDef {
  importDecls: Array<ImportDecl>;
  classDefs: Array<ClassDef>;
  fnDefs: Array<FnDef>;
}
