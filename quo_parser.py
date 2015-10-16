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
"""Quo parser."""

import ply.yacc
import quo_lexer
import quo_ast


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
    p[0] = [
        quo_ast.VarDeclStmt(var[0], None, var[1])
        for var in p[1]
    ]

  def p_var_decls_typed(self, p):
    '''var_decls : var_list type_spec SEMICOLON'''
    p[0] = [
        quo_ast.VarDeclStmt(var[0], p[2], var[1])
        for var in p[1]
    ]

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

  # Operator precedence.
  precedence = (
      ('left', 'OR'),
      ('left', 'AND'),
      ('nonassoc', 'EQ', 'NE', 'GT', 'GE', 'LT', 'LE'),
      ('left', 'ADD', 'SUB'),
      ('left', 'MUL', 'DIV', 'MOD'), )


def create_parser(**kwargs):
  return ply.yacc.yacc(module=QuoParser(), **kwargs)