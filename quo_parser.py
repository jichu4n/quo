# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2015 Chuan Ji <jichuan89@gmail.com>                        #
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

For debugging, run this module to print out an AST for Quo modules. It Will read
from files supplied on the command line, or stdin if no files are specified.
"""

import ply.yacc
import quo_ast
import quo_lexer


class QuoParser(object):
  """Quo parser, implemented via PLY."""

  tokens = quo_lexer.QuoLexer.tokens

  def p_primary_constant(self, p):
    '''primary : STRING_CONSTANT
               | INTEGER_CONSTANT
               | BOOLEAN_CONSTANT
    '''
    p[0] = quo_ast.ConstantExpr(p[1])

  def p_primary_var(self, p):
    '''primary : IDENTIFIER
               | THIS
    '''
    p[0] = quo_ast.VarExpr(p[1])

  def p_primary_paren(self, p):
    '''primary : L_PAREN expr R_PAREN'''
    p[0] = p[2]

  def p_primary_member(self, p):
    '''primary : primary DOT IDENTIFIER'''
    p[0] = quo_ast.MemberExpr(p[1], p[3])

  def p_primary_index(self, p):
    '''primary : primary L_BRACKET expr R_BRACKET'''
    p[0] = quo_ast.IndexExpr(p[1], p[3])

  def p_primary_call(self, p):
    '''primary : primary L_PAREN expr_list R_PAREN'''
    p[0] = quo_ast.CallExpr(p[1], p[3])

  def p_unary_arith_primary(self, p):
    '''unary_arith : primary'''
    p[0] = p[1]

  def p_unary_arith(self, p):
    '''unary_arith : ADD unary_arith
                   | SUB unary_arith
                   | MOVE unary_arith
    '''
    p[0] = quo_ast.UnaryOpExpr(p[1], p[2])

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
    p[0] = quo_ast.BinaryOpExpr(p[2], p[1], p[3])

  def p_unary_bool_binary_arith(self, p):
    '''unary_bool : binary_arith'''
    p[0] = p[1]

  def p_unary_bool(self, p):
    '''unary_bool : NOT unary_bool'''
    p[0] = quo_ast.UnaryOpExpr(p[1], p[2])

  def p_binary_bool_unary_bool(self, p):
    '''binary_bool : unary_bool'''
    p[0] = p[1]

  def p_binary_bool(self, p):
    '''binary_bool : binary_bool AND binary_bool
                   | binary_bool OR binary_bool
    '''
    p[0] = quo_ast.BinaryOpExpr(p[2], p[1], p[3])

  def p_assign(self, p):
    '''assign : primary ASSIGN expr'''
    p[0] = quo_ast.AssignExpr(p[1], p[3])

  def p_assign_op(self, p):
    '''assign : primary ADD_ASSIGN expr
              | primary SUB_ASSIGN expr
              | primary MUL_ASSIGN expr
              | primary DIV_ASSIGN expr
    '''
    p[0] = quo_ast.AssignExpr(p[1], quo_ast.BinaryOpExpr(
        p[2].split('_')[0], p[1], p[3]))

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
    p[0] = quo_ast.TypeSpec(p[1], [])

  def p_type_spec_primary_name_params(self, p):
    '''type_spec_primary : IDENTIFIER LT type_spec_list GT'''
    p[0] = quo_ast.TypeSpec(p[1], p[3])

  def p_type_spec_type_spec_primary(self, p):
    '''type_spec : type_spec_primary'''
    p[0] = p[1]

  def p_type_spec_member(self, p):
    '''type_spec : type_spec DOT type_spec_primary'''
    p[0] = quo_ast.MemberTypeSpec(p[1], p[3].name, p[3].params)

  def p_type_spec_list_empty(self, p):
    '''type_spec_list :'''
    p[0] = []

  def p_type_spec_list_one(self, p):
    '''type_spec_list : type_spec'''
    p[0] = [p[1]]

  def p_type_spec_list(self, p):
    '''type_spec_list : type_spec COMMA type_spec_list'''
    p[0] = [p[1]] + p[3]

  def p_var(self, p):
    '''var : IDENTIFIER'''
    p[0] = (p[1], None)

  def p_var_init_expr(self, p):
    '''var : IDENTIFIER ASSIGN expr'''
    p[0] = (p[1], p[3])

  def p_var_list_one(self, p):
    '''var_list : var'''
    p[0] = [p[1]]

  def p_var_list(self, p):
    '''var_list : var COMMA var_list'''
    p[0] = [p[1]] + p[3]

  def p_var_decls_untyped(self, p):
    '''var_decls : var_list SEMICOLON'''
    p[0] = [quo_ast.VarDeclStmt(var[0], None, var[1]) for var in p[1]]

  def p_var_decls_typed(self, p):
    '''var_decls : var_list type_spec SEMICOLON'''
    p[0] = [quo_ast.VarDeclStmt(var[0], p[2], var[1]) for var in p[1]]

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
    p[0] = quo_ast.ExprStmt(p[1])

  def p_return_stmt(self, p):
    '''return_stmt : RETURN expr SEMICOLON'''
    p[0] = quo_ast.ReturnStmt(p[2])

  def p_break_stmt(self, p):
    '''break_stmt : BREAK SEMICOLON'''
    p[0] = quo_ast.BreakStmt()

  def p_continue_stmt(self, p):
    '''continue_stmt : CONTINUE SEMICOLON'''
    p[0] = quo_ast.ContinueStmt()

  def p_cond_stmt_if(self, p):
    '''cond_stmt_if : IF expr L_BRACE stmts R_BRACE'''
    p[0] = quo_ast.CondStmt(p[2], p[4], [])

  def p_cond_stmt_cond_stmt_if(self, p):
    '''cond_stmt : cond_stmt_if'''
    p[0] = p[1]

  def p_cond_stmt_if_else_if(self, p):
    '''cond_stmt : cond_stmt_if ELSE cond_stmt'''
    p[0] = quo_ast.CondStmt(p[1].cond_expr, p[1].true_stmts, [p[3]])

  def p_cond_stmt_if_else(self, p):
    '''cond_stmt : cond_stmt_if ELSE L_BRACE stmts R_BRACE'''
    p[0] = quo_ast.CondStmt(p[1].cond_expr, p[1].true_stmts, p[4])

  def p_cond_loop_stmt(self, p):
    '''cond_loop_stmt : WHILE expr L_BRACE stmts R_BRACE'''
    p[0] = quo_ast.CondLoopStmt(p[2], p[4])

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
    p[0] = []

  def p_stmts_var_decl_stmts(self, p):
    '''stmts : var_decl_stmts stmts'''
    p[0] = p[1] + p[2]

  def p_stmts(self, p):
    '''stmts : stmt stmts'''
    p[0] = [p[1]] + p[2]

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

  def p_func_param_untyped(self, p):
    '''func_param : var'''
    p[0] = quo_ast.FuncParam(p[1][0], None, p[1][1])

  def p_func_param_typed(self, p):
    '''func_param : var type_spec'''
    p[0] = quo_ast.FuncParam(p[1][0], p[2], p[1][1])

  def p_func_param_list_empty(self, p):
    '''func_param_list :'''
    p[0] = []

  def p_func_param_list_one(self, p):
    '''func_param_list : func_param'''
    p[0] = [p[1]]

  def p_func_param_list(self, p):
    '''func_param_list : func_param COMMA func_param_list'''
    p[0] = [p[1]] + p[3]

  def p_return_type_spec_empty(self, p):
    '''return_type_spec :'''
    p[0] = None

  def p_return_type_spec(self, p):
    '''return_type_spec : type_spec'''
    p[0] = p[1]

  def p_func(self, p):
    '''func : FUNCTION IDENTIFIER type_params \
              L_PAREN func_param_list R_PAREN \
              return_type_spec \
              L_BRACE stmts R_BRACE'''
    p[0] = quo_ast.Func(p[2], p[3], p[5], p[7], p[9])

  def p_super_classes_empty(self, p):
    '''super_classes :'''
    p[0] = []

  def p_super_classes(self, p):
    '''super_classes : EXTENDS type_spec_list'''
    p[0] = p[2]

  def p_members_empty(self, p):
    '''members :'''
    p[0] = []

  def p_members_var_decl_stmts(self, p):
    '''members : var_decl_stmts members'''
    p[0] = p[1] + p[2]

  def p_members_func_class(self, p):
    '''members : func members
               | class members
    '''
    p[0] = [p[1]] + p[2]

  def p_class(self, p):
    '''class : CLASS IDENTIFIER type_params super_classes \
               L_BRACE members R_BRACE
    '''
    p[0] = quo_ast.Class(p[2], p[3], p[4], p[6])

  def p_module(self, p):
    '''module : members'''
    p[0] = quo_ast.Module(p[1])

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
  import yaml

  parser = create_parser()
  lexer = quo_lexer.create_lexer()
  ast = parser.parse('\n'.join(fileinput.input()), lexer=lexer)
  serializer_visitor = quo_ast.SerializerVisitor()
  if isinstance(ast, list):
    ast_serialized = [ast_node.accept(serializer_visitor) for ast_node in ast]
    ast_text = yaml.dump_all(ast_serialized, explicit_start=True)
  else:
    ast_serialized = ast.accept(serializer_visitor)
    ast_text = yaml.dump(ast_serialized)
  print(ast_text)
