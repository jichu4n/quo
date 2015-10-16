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
    '''primary : IDENTIFIER'''
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

  def p_expr(self, p):
    '''expr : binary_bool'''
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

  # Operator precedence.
  precedence = (
      ('left', 'OR'),
      ('left', 'AND'),
      ('nonassoc', 'EQ', 'NE', 'GT', 'GE', 'LT', 'LE'),
      ('left', 'ADD', 'SUB'),
      ('left', 'MUL', 'DIV', 'MOD'), )


def create_parser(**kwargs):
  return ply.yacc.yacc(module=QuoParser(), **kwargs)
