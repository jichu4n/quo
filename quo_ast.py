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


class AssignExpr(Expr):
  """Assignment expression."""

  def __init__(self, dest_expr, expr):
    self.dest_expr = dest_expr
    self.expr = expr

  def to_json(self):
    return {
        'type': 'AssignExpr',
        'dest_expr': self.dest_expr.to_json(),
        'expr': self.expr.to_json(),
    }


class Stmt(Node):
  """Base class of statements."""
  pass


class ExprStmt(Stmt):
  """Expression statement."""

  def __init__(self, expr):
    self.expr = expr

  def to_json(self):
    return {'type': 'ExprStmt', 'expr': self.expr.to_json()}


class ReturnStmt(Stmt):
  """return statement."""

  def __init__(self, expr):
    self.expr = expr

  def to_json(self):
    return {'type': 'ReturnStmt', 'expr': self.expr.to_json()}


class BreakStmt(Stmt):
  """break statement."""

  def to_json(self):
    return {'type': 'BreakStmt'}


class ContinueStmt(Stmt):
  """continue statement."""

  def to_json(self):
    return {'type': 'ContinueStmt'}


class CondStmt(Stmt):
  """if-else statements."""

  def __init__(self, cond_expr, true_stmts, false_stmts):
    self.cond_expr = cond_expr
    self.true_stmts = true_stmts
    self.false_stmts = false_stmts

  def to_json(self):
    return {
        'type': 'CondStmt',
        'cond_expr': self.cond_expr.to_json(),
        'true_stmts': [stmt.to_json() for stmt in self.true_stmts],
        'false_stmts': [stmt.to_json() for stmt in self.false_stmts],
    }


class CondLoopStmt(Stmt):
  """Conditional loop statements."""

  def __init__(self, cond_expr, stmts):
    self.cond_expr = cond_expr
    self.stmts = stmts

  def to_json(self):
    return {
        'type': 'CondStmt',
        'cond_expr': self.cond_expr.to_json(),
        'stmts': [stmt.to_json() for stmt in self.stmts],
    }


class TypeSpec(Node):
  """Base class for type specifications."""

  def __init__(self, name, params):
    self.name = name
    self.params = params

  def to_json(self):
    return {
        'type': 'TypeSpec',
        'name': self.name,
        'params': [param.to_json() for param in self.params],
    }


class MemberTypeSpec(TypeSpec):
  """A type nested in another type."""

  def __init__(self, parent_type_spec, name, params):
    super().__init__(name, params)
    self.parent_type_spec = parent_type_spec

  def to_json(self):
    result = super().to_json()
    result.update({
        'type': 'MemberTypeSpec',
        'parent_type_spec': self.parent_type_spec.to_json(),
    })
    return result
