import fs from 'fs-extra';
import {lines, LinesBuilder} from 'lines-builder';
import path from 'path';
import {
  AssignExpr,
  BinaryOpExpr,
  ClassDef,
  Expr,
  ExprStmt,
  ExprType,
  FnCallExpr,
  FnDef,
  IfStmt,
  MemberExpr,
  ModuleDef,
  NewExpr,
  NumberLiteralExpr,
  NullExpr,
  ReturnStmt,
  Stmt,
  StmtType,
  StringLiteralExpr,
  SubscriptExpr,
  UnaryOpExpr,
  VarDecl,
  VarDeclStmt,
  VarRefExpr,
  WhileStmt,
} from './ast';
import {parse} from './parser';

export function getFileNameWithoutExt(moduleName: string) {
  return path.basename(moduleName).replace(/\.quo$/i, '');
}

export function getHeaderFileName(moduleName: string) {
  return getFileNameWithoutExt(moduleName) + '.h';
}

export function getCppFileName(moduleName: string) {
  return getFileNameWithoutExt(moduleName) + '.cpp';
}

export function generateHeaderCode(module: ModuleDef): string {
  const l = lines();

  const includeGuard = `__${module.name
    .toUpperCase()
    .replace(/[^A-Z0-9]+/g, '_')}_H__`;
  l.append(
    `#ifndef ${includeGuard}`,
    `#define ${includeGuard}`,
    null,
    '#include "quo-rt.h"'
  );
  l.append(
    ...module.importDecls.map(({moduleName}) => `#include "${moduleName}.h"`),
    null
  );

  if (module.classDefs.length) {
    l.append(...module.classDefs.map(generateClassDecl), null);
    l.append(...module.classDefs.map(generateClassDef), null);
  }

  if (module.varDecls.length) {
    l.append(...module.varDecls.map(generateVarDecl), null);
  }

  if (module.fnDefs.length) {
    l.append(
      'extern "C" {',
      null,
      ...module.fnDefs.map(generateFnDecl),
      null,
      '}'
    );
  }

  l.append(null, `#endif`);

  return l.toString().trim();
}

function generateClassDecl(classDef: ClassDef) {
  return `struct ${classDef.name};`;
}

function generateClassDef(classDef: ClassDef) {
  return lines(
    `struct ${classDef.name} {`,
    lines(
      {indent: 2},
      ...classDef.props.map(({typeString, name}) => `${typeString}* ${name};`)
    ),
    `};`
  );
}

function generateFnSig(fnDef: FnDef) {
  return [
    fnDef.returnTypeString ? `${fnDef.returnTypeString}*` : 'void',
    ' ',
    fnDef.name,
    '(',
    fnDef.params
      .map(({typeString, name}) => `${typeString}* ${name}`)
      .join(', '),
    ')',
  ].join('');
}

function generateFnDecl(fnDef: FnDef) {
  return `${generateFnSig(fnDef)};`;
}

function generateVarDecl(varDecl: VarDecl) {
  return `extern ${varDecl.typeString}* ${varDecl.name};`;
}

export function generateCppCode(moduleDef: ModuleDef): string {
  const l = lines();
  l.append(`#include "${getHeaderFileName(moduleDef.name)}"`, null);
  if (moduleDef.varDecls.length) {
    l.append(
      generateVarDeclStmt({
        type: StmtType.VAR_DECL,
        varDecls: moduleDef.varDecls,
      }),
      null
    );
  }
  l.append(...moduleDef.fnDefs.map(generateFnDef));
  return l.toString();
}

function generateFnDef(fnDef: FnDef): LinesBuilder {
  return lines(
    `${generateFnSig(fnDef)} {`,
    generateBlock(fnDef.body),
    '}',
    null
  );
}

function generateBlock(stmts: Array<Stmt>): LinesBuilder {
  return lines({indent: 2}, ...stmts.map(generateStmt));
}

function generateStmt(stmt: Stmt): LinesBuilder {
  switch (stmt.type) {
    case StmtType.EXPR:
      return generateExprStmt(stmt);
    case StmtType.RETURN:
      return generateReturnStmt(stmt);
    case StmtType.IF:
      return generateIfStmt(stmt);
    case StmtType.WHILE:
      return generateWhileStmt(stmt);
    case StmtType.VAR_DECL:
      return generateVarDeclStmt(stmt);
    default:
      throw new Error(JSON.stringify(stmt));
  }
}

function generateExprStmt(stmt: ExprStmt): LinesBuilder {
  return lines(`${generateExpr(stmt.expr)};`);
}

function generateReturnStmt(stmt: ReturnStmt): LinesBuilder {
  return lines(
    `return${stmt.valueExpr ? ` ${generateExpr(stmt.valueExpr)}` : ''};`
  );
}

function generateIfStmt(stmt: IfStmt): LinesBuilder {
  const l = lines(
    `if (${generateValueExpr(stmt.condExpr)}) {`,
    generateBlock(stmt.ifBlock)
  );
  if (stmt.elseBlock.length) {
    l.append('} else {', generateBlock(stmt.elseBlock));
  }
  l.append('}');
  return l;
}

function generateWhileStmt(stmt: WhileStmt): LinesBuilder {
  return lines(
    `while (${generateValueExpr(stmt.condExpr)}) {`,
    generateBlock(stmt.block),
    '}'
  );
}

function generateVarDeclStmt(stmt: VarDeclStmt): LinesBuilder {
  return lines(
    ...stmt.varDecls.map(
      ({name, typeString, initExpr}) =>
        `${typeString}* ${name} = ${
          initExpr ? generateExpr(initExpr) : 'nullptr'
        };`
    )
  );
}

function generateExpr(expr: Expr): string {
  switch (expr.type) {
    case ExprType.STRING_LITERAL:
      return generateStringLiteralExpr(expr);
    case ExprType.NUMBER_LITERAL:
      return generateNumberLiteralExpr(expr);
    case ExprType.NULL:
      return generateNullExpr(expr);
    case ExprType.VAR_REF:
      return generateVarRefExpr(expr);
    case ExprType.FN_CALL:
      return generateFnCallExpr(expr);
    case ExprType.BINARY_OP:
      return generateBinaryOpExpr(expr);
    case ExprType.UNARY_OP:
      return generateUnaryOpExpr(expr);
    case ExprType.SUBSCRIPT:
      return generateSubscriptExpr(expr);
    case ExprType.MEMBER:
      return generateMemberExpr(expr);
    case ExprType.ASSIGN:
      return generateAssignExpr(expr);
    case ExprType.NEW:
      return generateNewExpr(expr);
    default:
      throw new Error(JSON.stringify(expr));
  }
}

function generateValueExpr(expr: Expr): string {
  const exprString = generateExpr(expr);
  switch (expr.type) {
    case ExprType.NULL:
      return exprString;
    default:
      return `*(${exprString})`;
  }
}

function generateStringLiteralExpr(expr: StringLiteralExpr) {
  return `new String("${expr.value}")`;
}

function generateNumberLiteralExpr(expr: NumberLiteralExpr) {
  return `new Int64(${expr.value})`;
}

function generateNullExpr(_: NullExpr) {
  return 'nullptr';
}

function generateVarRefExpr(expr: VarRefExpr) {
  return expr.name;
}

function generateFnCallExpr(expr: FnCallExpr) {
  return `${generateExpr(expr.fnExpr)}(${expr.argExprs.map(generateExpr).join(', ')})`;
}

function generateBinaryOpExpr(expr: BinaryOpExpr) {
  const OP_TABLE: {[key: string]: string} = {
    and: '&&',
    or: '||',
    add: '+',
    sub: '-',
    exp: '**',
    mul: '*',
    div: '/',
    eq: '==',
    ne: '!=',
    gte: '>=',
    lte: '<=',
    gt: '>',
    lt: '<',
  };
  const op = OP_TABLE[expr.op];
  if (!op) {
    throw new Error(JSON.stringify(expr));
  }
  let leftExprString: string;
  let rightExprString: string;
  if (expr.leftExpr.type === ExprType.NULL || expr.rightExpr.type === ExprType.NULL) {
    leftExprString = generateExpr(expr.leftExpr);
    rightExprString = generateExpr(expr.rightExpr);
  } else {
    leftExprString = generateValueExpr(expr.leftExpr);
    rightExprString = generateValueExpr(expr.rightExpr);
  }
  return `new Int64((${leftExprString}) ${op} (${rightExprString}))`;
}

function generateUnaryOpExpr(expr: UnaryOpExpr) {
  const OP_TABLE: {[key: string]: string} = {
    sub: '-',
    not: '!',
  };
  const op = OP_TABLE[expr.op];
  if (!op) {
    throw new Error(JSON.stringify(expr));
  }
  return `new Int64(${op}(${generateValueExpr(expr.rightExpr)}))`;
}

function generateSubscriptExpr(expr: SubscriptExpr) {
  let exprString = generateValueExpr(expr.arrayExpr);
  for (const indexExpr of expr.indexExprs) {
    exprString = `(${exprString})[${generateValueExpr(indexExpr)}]`;
  }
  return exprString;
}

function generateMemberExpr(expr: MemberExpr) {
  return `(${generateExpr(expr.objExpr)})->${expr.fieldName}`;
}

function generateAssignExpr(expr: AssignExpr) {
  return `${generateExpr(expr.leftExpr)} = ${generateExpr(expr.rightExpr)}`;
}

function generateNewExpr(expr: NewExpr) {
  return `new ${expr.typeString}()`;
}

export async function codegen(sourceFilePath: string) {
  const sourceFileName = path.basename(sourceFilePath);
  const sourceFileDirPath = path.dirname(sourceFilePath);
  const headerFilePath = path.join(
    sourceFileDirPath,
    getHeaderFileName(sourceFileName)
  );
  const cppFilePath = path.join(
    sourceFileDirPath,
    getCppFileName(sourceFileName)
  );
  const sourceCode = await fs.readFile(sourceFilePath, 'utf-8');
  const moduleDef = parse(sourceCode, {sourceFileName});
  const headerCode = generateHeaderCode(moduleDef);
  const cppCode = generateCppCode(moduleDef);
  await Promise.all([
    fs.writeFile(headerFilePath, headerCode),
    fs.writeFile(cppFilePath, cppCode),
  ]);
  return {headerFilePath, cppFilePath};
}

if (require.main === module) {
  (async () => {
    const {program} = await import('commander');

    program.argument('<input-file>', 'Quo source file');
    program.parse();

    const sourceFilePath = program.args[0];
    await codegen(sourceFilePath);
  })();
}
