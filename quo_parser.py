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

  def p_expr_1_constant(self, p):
    '''expr_1 : STRING_CONSTANT
              | INTEGER_CONSTANT
    '''
    p[0] = quo_ast.ConstantExpr(p[1])

  def p_expr_1_var(self, p):
    '''expr_1 : IDENTIFIER'''
    p[0] = quo_ast.VarExpr(p[1])

  def p_expr_1_paren(self, p):
    '''expr_1 : L_PAREN expr R_PAREN'''
    p[0] = p[2]

  def p_expr_1_member(self, p):
    '''expr_1 : expr_1 DOT IDENTIFIER'''
    p[0] = quo_ast.MemberExpr(p[1], p[3])

  def p_expr_1_index(self, p):
    '''expr_1 : expr_1 L_BRACKET expr R_BRACKET'''
    p[0] = quo_ast.IndexExpr(p[1], p[3])

  def p_expr_1_call(self, p):
    '''expr_1 : expr_1 L_PAREN expr_list R_PAREN'''
    p[0] = quo_ast.CallExpr(p[1], p[3])

  def p_expr(self, p):
    '''expr : expr_1'''
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


def create_parser(**kwargs):
  return ply.yacc.yacc(module=QuoParser(), **kwargs)
