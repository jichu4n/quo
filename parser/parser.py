#!/usr/bin/env python3
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2016-2017 Chuan Ji <ji@chu4n.com>                          #
#                                                                             #
#    Licensed under the Apache License, Version 2.0 (the "License");          #
#    you may not use this file except in compliance with the License.         #
#    You may obtain a copy of the License at                                  #
#                                                                             #
#     http://www.apache.org/licenses/LICENSE-2.0                              #
#                                                                             #
#    Unless required by applicable law or agreed to in writing, software      #
#    distributed under the License is distributed on an "AS IS" BASIS,        #
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
#    See the License for the specific language governing permissions and      #
#    limitations under the License.                                           #
#                                                                             #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
"""Quo parser.

For debugging, run this module to print out an AST for Quo modules. It will read
from files supplied on the command line, or stdin if no files are specified.

Example:

  python3 -m parser.parser /path/to/file.quo
"""

import ply.yacc
from parser import lexer
from build.ast.ast_pb2 import *


class QuoParser(object):
  """Quo parser, implemented via PLY."""

  tokens = lexer.QuoLexer.tokens

  def p_primary_constant_str(self, p):
    '''primary : STRING_CONSTANT'''
    p[0] = Expr(constant=ConstantExpr(str_value=p[1]))

  def p_primary_constant_int(self, p):
    '''primary : INTEGER_CONSTANT'''
    p[0] = Expr(constant=ConstantExpr(int_value=p[1]))

  def p_primary_constant_bool(self, p):
    '''primary : BOOLEAN_CONSTANT'''
    p[0] = Expr(constant=ConstantExpr(bool_value=p[1]))

  def p_primary_var(self, p):
    '''primary : IDENTIFIER
               | THIS
    '''
    p[0] = Expr(var=VarExpr(name=p[1]))

  def p_primary_paren(self, p):
    '''primary : L_PAREN expr R_PAREN'''
    p[0] = p[2]

  def p_primary_member(self, p):
    '''primary : primary DOT IDENTIFIER'''
    p[0] = Expr(member=MemberExpr(parent_expr=p[1], member_name=p[3]))

  def p_primary_index(self, p):
    '''primary : primary L_BRACKET expr R_BRACKET'''
    p[0] = Expr(index=IndexExpr(array_expr=p[1], index_expr=p[3]))

  def p_primary_call(self, p):
    '''primary : primary L_PAREN expr_list R_PAREN'''
    p[0] = Expr(call=CallExpr(fn_expr=p[1], arg_exprs=p[3]))

  def p_unary_arith_primary(self, p):
    '''unary_arith : primary'''
    p[0] = p[1]

  def p_unary_arith(self, p):
    '''unary_arith : ADD unary_arith
                   | SUB unary_arith
    '''
    UNARY_OP_MAP = {
        '+': UnaryOpExpr.ADD,
        '-': UnaryOpExpr.SUB,
    }
    p[0] = Expr(unary_op=UnaryOpExpr(op=getattr(UnaryOpExpr, p[1]), expr=p[2]))

  def p_binary_arith_unary_arith(self, p):
    '''binary_arith : unary_arith'''
    p[0] = p[1]

  def p_binary_arith(self, p):
    '''binary_arith : binary_arith ADD binary_arith
                    | binary_arith SUB binary_arith
                    | binary_arith MUL binary_arith
                    | binary_arith DIV binary_arith
                    | binary_arith MOD binary_arith
                    | binary_arith EQ binary_arith
                    | binary_arith NE binary_arith
                    | binary_arith GT binary_arith
                    | binary_arith GE binary_arith
                    | binary_arith LT binary_arith
                    | binary_arith LE binary_arith
    '''
    p[0] = Expr(binary_op=BinaryOpExpr(
      op=getattr(BinaryOpExpr, p[2]), left_expr=p[1], right_expr=p[3]))

  def p_unary_bool_binary_arith(self, p):
    '''unary_bool : binary_arith'''
    p[0] = p[1]

  def p_unary_bool(self, p):
    '''unary_bool : NOT unary_bool'''
    p[0] = Expr(unary_op=UnaryOpExpr(op=UnaryOpExpr.NOT, expr=p[2]))

  def p_binary_bool_unary_bool(self, p):
    '''binary_bool : unary_bool'''
    p[0] = p[1]

  def p_binary_bool(self, p):
    '''binary_bool : binary_bool AND binary_bool
                   | binary_bool OR binary_bool
    '''
    p[0] = Expr(binary_op=BinaryOpExpr(
      op=getattr(BinaryOpExpr, p[2]), left_expr=p[1], right_expr=p[3]))

  def p_assign(self, p):
    '''assign : primary ASSIGN expr'''
    p[0] = Expr(assign=AssignExpr(dest_expr=p[1], value_expr=p[3]))

  def p_assign_op(self, p):
    '''assign : primary ADD_ASSIGN expr
              | primary SUB_ASSIGN expr
              | primary MUL_ASSIGN expr
              | primary DIV_ASSIGN expr
    '''
    # TODO: This is not technically correct as it would re-evaluate the dest
    # expr, which may have side effects.
    p[0] = Expr(assign=AssignExpr(
      dest_expr=p[1],
      value_expr=Expr(binary_op=BinaryOpExpr(
        op=getattr(BinaryOpExpr, p[2].split('_')[0]),
        left_expr=p[1],
        right_expr=p[3]))))

  def p_expr(self, p):
    '''expr : binary_bool
            | assign
    '''
    p[0] = p[1]

  def p_expr_list_empty(self, p):
    '''expr_list :'''
    p[0] = []

  def p_expr_list_one(self, p):
    '''expr_list : expr'''
    p[0] = [p[1]]

  def p_expr_list(self, p):
    '''expr_list : expr COMMA expr_list'''
    p[0] = [p[1]] + p[3]

  def p_type_spec_primary_name(self, p):
    '''type_spec_primary : IDENTIFIER'''
    p[0] = TypeSpec(name=p[1])

  def p_type_spec_primary_name_params(self, p):
    '''type_spec_primary : IDENTIFIER LT type_spec_list GT'''
    p[0] = TypeSpec(name=p[1], params=p[3])

  def p_type_spec_type_spec_primary(self, p):
    '''type_spec : type_spec_primary'''
    p[0] = p[1]

  def p_type_spec_member(self, p):
    '''type_spec : type_spec DOT type_spec_primary'''
    p[0] = TypeSpec()
    p[0].CopyFrom(p[3])
    p[0].parent.CopyFrom(p[1])

  def p_type_spec_list_empty(self, p):
    '''type_spec_list :'''
    p[0] = []

  def p_type_spec_list_one(self, p):
    '''type_spec_list : type_spec'''
    p[0] = [p[1]]

  def p_type_spec_list(self, p):
    '''type_spec_list : type_spec COMMA type_spec_list'''
    p[0] = [p[1]] + p[3]

  def p_var_mode_strong(self, p):
    '''var_mode :'''
    p[0] = STRONG_REF

  def p_var_mode_weak(self, p):
    '''var_mode : WEAK_REF'''
    p[0] = WEAK_REF

  def p_var(self, p):
    '''var : IDENTIFIER'''
    p[0] = VarDeclStmt(name=p[1])

  def p_var_init_expr(self, p):
    '''var : IDENTIFIER ASSIGN expr'''
    p[0] = VarDeclStmt(name=p[1], init_expr=p[3])

  def p_var_list_one(self, p):
    '''var_list : var_mode var'''
    p[0] = [VarDeclStmt()]
    p[0][0].CopyFrom(p[2])
    p[0][0].ref_mode = p[1]

  def p_var_list(self, p):
    '''var_list : var_mode var COMMA var_list'''
    p[0] = [VarDeclStmt()] + p[4]
    p[0][0].CopyFrom(p[2])
    p[0][0].ref_mode = p[1]

  def p_var_decls_untyped(self, p):
    '''var_decls : var_list SEMICOLON'''
    p[0] = [
        Stmt(var_decl=var_decl)
        for var_decl in p[1]
    ]

  def p_var_decls_typed(self, p):
    '''var_decls : var_list type_spec SEMICOLON'''
    p[0] = []
    for var_decl in p[1]:
      var_decl_stmt = VarDeclStmt(
          name=var_decl.name,
          ref_mode=var_decl.ref_mode,
          type_spec=p[2])
      if var_decl.HasField('init_expr'):
        var_decl_stmt.init_expr.CopyFrom(var_decl.init_expr)
      p[0].append(Stmt(var_decl=var_decl_stmt))

  def p_var_decls_list_empty(self, p):
    '''var_decls_list :'''
    p[0] = []

  def p_var_decls_list(self, p):
    '''var_decls_list : var_decls var_decls_list'''
    p[0] = p[1] + p[2]

  def p_var_decls_stmts(self, p):
    '''var_decl_stmts : VAR var_decls'''
    p[0] = p[2]

  def p_var_decls_stmts_block(self, p):
    '''var_decl_stmts : VAR L_BRACE var_decls_list R_BRACE'''
    p[0] = p[3]

  def p_expr_stmt(self, p):
    '''expr_stmt : expr SEMICOLON'''
    p[0] = Stmt(expr=ExprStmt(expr=p[1]))

  def p_return_stmt(self, p):
    '''return_stmt : RETURN SEMICOLON'''
    p[0] = Stmt(ret=RetStmt())

  def p_return_stmt_with_value(self, p):
    '''return_stmt : RETURN expr SEMICOLON'''
    p[0] = Stmt(ret=RetStmt(expr=p[2]))

  def p_break_stmt(self, p):
    '''break_stmt : BREAK SEMICOLON'''
    p[0] = Stmt(brk=BrkStmt())

  def p_continue_stmt(self, p):
    '''continue_stmt : CONTINUE SEMICOLON'''
    p[0] = Stmt(cont=ContStmt())

  def p_cond_stmt_if(self, p):
    '''cond_stmt_if : IF expr L_BRACE stmts R_BRACE'''
    p[0] = CondStmt(cond_expr=p[2], true_block=p[4])

  def p_cond_stmt_cond_stmt_if(self, p):
    '''cond_stmt : cond_stmt_if'''
    p[0] = Stmt(cond=p[1])

  def p_cond_stmt_if_else_if(self, p):
    '''cond_stmt : cond_stmt_if ELSE cond_stmt'''
    p[0] = Stmt(cond=CondStmt(
        cond_expr=p[1].cond_expr,
        true_block=p[1].true_block,
        false_block=Block(stmts=[p[3]])))

  def p_cond_stmt_if_else(self, p):
    '''cond_stmt : cond_stmt_if ELSE L_BRACE stmts R_BRACE'''
    p[0] = Stmt(cond=CondStmt(
        cond_expr=p[1].cond_expr,
        true_block=p[1].true_block,
        false_block=p[4]))

  def p_cond_loop_stmt(self, p):
    '''cond_loop_stmt : WHILE expr L_BRACE stmts R_BRACE'''
    p[0] = Stmt(cond_loop=CondLoopStmt(cond_expr=p[2], block=p[4]))

  def p_stmt(self, p):
    '''stmt : expr_stmt
            | return_stmt
            | break_stmt
            | continue_stmt
            | cond_stmt
            | cond_loop_stmt
    '''
    p[0] = p[1]

  def p_stmts_empty(self, p):
    '''stmts :'''
    p[0] = Block()

  def p_stmts_var_decl_stmts(self, p):
    '''stmts : var_decl_stmts stmts'''
    p[0] = Block(stmts=(p[1] + list(p[2].stmts)))

  def p_stmts(self, p):
    '''stmts : stmt stmts'''
    p[0] = Block(stmts=([p[1]] + list(p[2].stmts)))

  def p_type_param(self, p):
    '''type_param : IDENTIFIER'''
    p[0] = p[1]

  def p_type_param_list_empty(self, p):
    '''type_param_list :'''
    p[0] = []

  def p_type_param_list_one(self, p):
    '''type_param_list : type_param'''
    p[0] = [p[1]]

  def p_type_param_list(self, p):
    '''type_param_list : type_param COMMA type_param_list'''
    p[0] = [p[1]] + p[3]

  def p_type_params_empty(self, p):
    '''type_params :'''
    p[0] = []

  def p_type_params(self, p):
    '''type_params : LT type_param_list GT'''
    p[0] = p[2]

  def p_fn_param_mode_empty(self, p):
    '''fn_param_mode :'''
    p[0] = STRONG_REF

  def p_fn_param_mode(self, p):
    '''fn_param_mode : WEAK_REF
    '''
    p[0] = WEAK_REF

  def p_fn_param_type_spec_empty(self, p):
    '''fn_param_type_spec :'''
    p[0] = None

  def p_fn_param_type_spec(self, p):
    '''fn_param_type_spec : type_spec'''
    p[0] = p[1]

  def p_fn_param(self, p):
    '''fn_param : fn_param_mode var fn_param_type_spec'''
    p[0] = FnParam(name=p[2].name, ref_mode=p[1])
    if p[2].HasField('init_expr'):
      p[0].init_expr.CopyFrom(p[2].init_expr)
    if p[3] is not None:
      p[0].type_spec.CopyFrom(p[3])

  def p_fn_param_list_empty(self, p):
    '''fn_param_list :'''
    p[0] = []

  def p_fn_param_list_one(self, p):
    '''fn_param_list : fn_param'''
    p[0] = [p[1]]

  def p_fn_param_list(self, p):
    '''fn_param_list : fn_param COMMA fn_param_list'''
    p[0] = [p[1]] + p[3]

  def p_return_type_spec_empty(self, p):
    '''return_type_spec :'''
    p[0] = None

  def p_return_type_spec(self, p):
    '''return_type_spec : type_spec'''
    p[0] = p[1]

  def p_func(self, p):
    '''func : FUNCTION IDENTIFIER type_params \
              L_PAREN fn_param_list R_PAREN \
              return_type_spec \
              L_BRACE stmts R_BRACE'''
    p[0] = FnDef(
        name=p[2],
        params=p[5],
        cc=FnDef.DEFAULT,
        block=p[9])
    if p[7] is not None:
      p[0].return_type_spec.CopyFrom(p[7])
    if p[3]:
      p[0].type_params.extend(p[3])

  def p_super_classes_empty(self, p):
    '''super_classes :'''
    p[0] = []

  def p_super_classes(self, p):
    '''super_classes : EXTENDS type_spec_list'''
    p[0] = p[2]

  def p_class_members_empty(self, p):
    '''class_members :'''
    p[0] = []

  def p_class_members_var_decl_stmts(self, p):
    '''class_members : var_decl_stmts class_members'''
    p[0] = [
        ClassDef.Member(var_decl=stmt.var_decl)
        for stmt in p[1]
    ] + p[2]

  def p_class_members_func(self, p):
    '''class_members : func class_members'''
    p[0] = [ClassDef.Member(fn_def=p[1])] + p[2]

  def p_class_members_class(self, p):
    '''class_members : class class_members'''
    p[0] = [ClassDef.Member(class_def=p[1])] + p[2]

  def p_class(self, p):
    '''class : CLASS IDENTIFIER type_params super_classes \
               L_BRACE class_members R_BRACE
    '''
    p[0] = ClassDef(
        name=p[2],
        type_params=p[3],
        super_classes=p[4],
        members=p[6])

  def p_extern_fn(self, p):
    '''extern_fn : EXTERN FUNCTION IDENTIFIER \
                   L_PAREN fn_param_list R_PAREN \
                   return_type_spec SEMICOLON
    '''
    p[0] = ExternFn(name=p[3], params=p[5], return_type_spec=p[7])

  def p_func_cc_empty(self, p):
    '''func_cc :'''
    p[0] = FnDef.DEFAULT

  def p_func_cc_c(self, p):
    '''func_cc : EXPORT'''
    p[0] = FnDef.C

  def p_module_func(self, p):
    '''module_func : func_cc func'''
    p[0] = FnDef()
    p[0].CopyFrom(p[2])
    p[0].cc = p[1]

  def p_module_members_empty(self, p):
    '''module_members :'''
    p[0] = []

  def p_module_members_var_decl_stmts(self, p):
    '''module_members : var_decl_stmts module_members'''
    p[0] = [
        ModuleDef.Member(var_decl=stmt.var_decl)
        for stmt in p[1]
    ] + p[2]

  def p_module_members_func(self, p):
    '''module_members : module_func module_members'''
    p[0] = [ModuleDef.Member(fn_def=p[1])] + p[2]

  def p_module_members_extern_fn(self, p):
    '''module_members : extern_fn module_members'''
    p[0] = [ModuleDef.Member(extern_fn=p[1])] + p[2]

  def p_module_members_class(self, p):
    '''module_members : class module_members'''
    p[0] = [ModuleDef.Member(class_def=p[1])] + p[2]

  def p_module(self, p):
    '''module : module_members'''
    p[0] = ModuleDef(members=p[1])

  # Operator precedence.
  precedence = (
      ('left', 'OR'),
      ('left', 'AND'),
      ('nonassoc', 'EQ', 'NE', 'GT', 'GE', 'LT', 'LE'),
      ('left', 'ADD', 'SUB'),
      ('left', 'MUL', 'DIV', 'MOD'), )


def create_parser(**kwargs):
  if 'start' not in kwargs:
    kwargs['start'] = 'module'
  return ply.yacc.yacc(module=QuoParser(), **kwargs)


if __name__ == '__main__':
  import fileinput

  parse = create_parser()
  lex = lexer.create_lexer()
  ast = parse.parse('\n'.join(fileinput.input()), lexer=lex)
  print(ast)
