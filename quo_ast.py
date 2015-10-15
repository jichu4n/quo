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
"""Quo abstract syntax tree."""


class Node(object):
  """Base class for an abstract syntax tree node."""

  def to_json(self):
    raise NotImplementedError()


class Expr(Node):
  """Base class for expressions."""
  pass


class ConstantExpr(Expr):
  """A constant."""

  def __init__(self, value):
    self.value = value

  def to_json(self):
    return {'type': 'ConstantExpr', 'value': self.value}


class VarExpr(Expr):
  """A variable reference."""

  def __init__(self, var):
    self.var = var

  def to_json(self):
    return {'type': 'VarExpr', 'var': self.var}


class MemberExpr(Expr):
  """A member reference."""

  def __init__(self, expr, member):
    self.expr = expr
    self.member = member

  def to_json(self):
    return {
        'type': 'MemberExpr',
        'expr': self.expr.to_json(),
        'member': self.member,
    }


class IndexExpr(Expr):
  """An index expression."""

  def __init__(self, expr, index_expr):
    self.expr = expr
    self.index_expr = index_expr

  def to_json(self):
    return {
        'type': 'IndexExpr',
        'expr': self.expr.to_json(),
        'index_expr': self.index_expr.to_json(),
    }


class CallExpr(Expr):
  """A function call."""

  def __init__(self, expr, arg_exprs):
    self.expr = expr
    self.arg_exprs = arg_exprs

  def to_json(self):
    return {
        'type': 'CallExpr',
        'expr': self.expr.to_json(),
        'arg_exprs': [arg_expr.to_json() for arg_expr in self.arg_exprs],
    }


class UnaryOpExpr(Expr):
  """A unary operation."""

  def __init__(self, op, expr):
    self.op = op
    self.expr = expr

  def to_json(self):
    return {
        'type': 'UnaryOpExpr',
        'op': self.op,
        'expr': self.expr.to_json(),
    }


class BinaryOpExpr(Expr):
  """A binary operation."""

  def __init__(self, op, left_expr, right_expr):
    self.op = op
    self.left_expr = left_expr
    self.right_expr = right_expr

  def to_json(self):
    return {
        'type': 'BinaryOpExpr',
        'op': self.op,
        'left_expr': self.left_expr.to_json(),
        'right_expr': self.right_expr.to_json(),
    }
