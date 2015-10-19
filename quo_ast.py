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

import inflection


class Node(object):
  """Base class for an abstract syntax tree node."""

  def accept(self, visitor):
    return visitor.visit(self)


class Expr(Node):
  """Base class for expressions."""
  pass


class ConstantExpr(Expr):
  """A constant."""

  def __init__(self, value):
    self.value = value


class VarExpr(Expr):
  """A variable reference."""

  def __init__(self, var):
    self.var = var


class MemberExpr(Expr):
  """A member reference."""

  def __init__(self, expr, member):
    self.expr = expr
    self.member = member

  def accept(self, visitor):
    return visitor.visit(self, {'expr': self.expr.accept(visitor), })


class IndexExpr(Expr):
  """An index expression."""

  def __init__(self, expr, index_expr):
    self.expr = expr
    self.index_expr = index_expr

  def accept(self, visitor):
    return visitor.visit(self, {
        'expr': self.expr.accept(visitor),
        'index_expr': self.index_expr.accept(visitor),
    })


class CallExpr(Expr):
  """A function call."""

  def __init__(self, expr, arg_exprs):
    self.expr = expr
    self.arg_exprs = arg_exprs

  def accept(self, visitor):
    return visitor.visit(self, {
        'expr': self.expr.accept(visitor),
        'arg_exprs': [
            arg_expr.accept(visitor) for arg_expr in self.arg_exprs
        ],
    })


class UnaryOpExpr(Expr):
  """A unary operation."""

  def __init__(self, op, expr):
    self.op = op
    self.expr = expr

  def accept(self, visitor):
    return visitor.visit(self, {'expr': self.expr.accept(visitor), })


class BinaryOpExpr(Expr):
  """A binary operation."""

  def __init__(self, op, left_expr, right_expr):
    self.op = op
    self.left_expr = left_expr
    self.right_expr = right_expr

  def accept(self, visitor):
    return visitor.visit(self, {
        'left_expr': self.left_expr.accept(visitor),
        'right_expr': self.right_expr.accept(visitor),
    })


class AssignExpr(Expr):
  """Assignment expression."""

  def __init__(self, dest_expr, expr):
    self.dest_expr = dest_expr
    self.expr = expr

  def accept(self, visitor):
    return visitor.visit(self, {
        'dest_expr': self.dest_expr.accept(visitor),
        'expr': self.expr.accept(visitor),
    })


class Stmt(Node):
  """Base class of statements."""
  pass


class ExprStmt(Stmt):
  """Expression statement."""

  def __init__(self, expr):
    self.expr = expr

  def accept(self, visitor):
    return visitor.visit(self, {'expr': self.expr.accept(visitor), })


class ReturnStmt(Stmt):
  """return statement."""

  def __init__(self, expr):
    self.expr = expr

  def accept(self, visitor):
    return visitor.visit(self, {'expr': self.expr.accept(visitor), })


class BreakStmt(Stmt):
  """break statement."""
  pass


class ContinueStmt(Stmt):
  """continue statement."""
  pass


class CondStmt(Stmt):
  """if-else statements."""

  def __init__(self, cond_expr, true_stmts, false_stmts):
    self.cond_expr = cond_expr
    self.true_stmts = true_stmts
    self.false_stmts = false_stmts

  def accept(self, visitor):
    return visitor.visit(self, {
        'cond_expr': self.cond_expr.accept(visitor),
        'true_stmts': [
            stmt.accept(visitor) for stmt in self.true_stmts
        ],
        'false_stmts': [
            stmt.accept(visitor) for stmt in self.false_stmts
        ],
    })


class CondLoopStmt(Stmt):
  """Conditional loop statements."""

  def __init__(self, cond_expr, stmts):
    self.cond_expr = cond_expr
    self.stmts = stmts

  def accept(self, visitor):
    return visitor.visit(self, {
        'cond_expr': self.cond_expr.accept(visitor),
        'stmts': [
            stmt.accept(visitor) for stmt in self.stmts
        ],
    })


class TypeSpec(Node):
  """Base class for type specifications."""

  def __init__(self, name, params):
    self.name = name
    self.params = params

  def accept(self, visitor):
    return visitor.visit(self, {
        'params': [param.accept(visitor) for param in self.params],
    })


class MemberTypeSpec(TypeSpec):
  """A type nested in another type."""

  def __init__(self, parent_type_spec, name, params):
    super().__init__(name, params)
    self.parent_type_spec = parent_type_spec

  def accept(self, visitor):
    return visitor.visit(self, {
        'params': [param.accept(visitor) for param in self.params],
        'parent_type_spec': self.parent_type_spec.accept(visitor),
    })


class VarDeclStmt(Stmt):
  """A variable declaration statement."""

  def __init__(self, name, type_spec, mode, init_expr):
    self.name = name
    self.type_spec = type_spec
    self.mode = mode
    self.init_expr = init_expr

  def accept(self, visitor):
    return visitor.visit(self, {
        'type_spec': (
            self.type_spec.accept(visitor) if self.type_spec else None),
        'init_expr': (
            self.init_expr.accept(visitor) if self.init_expr else None),
    })


class FuncParam(Node):
  """A function parameter."""

  def __init__(self, name, mode, type_spec, init_expr):
    self.name = name
    self.mode = mode
    self.type_spec = type_spec
    self.init_expr = init_expr

  def accept(self, visitor):
    return visitor.visit(self, {
        'type_spec': (
            self.type_spec.accept(visitor) if self.type_spec else None),
        'init_expr': (
            self.init_expr.accept(visitor) if self.init_expr else None),
    })


class Func(Node):
  """A function definition."""

  def __init__(self, name, type_params, params, return_type_spec, stmts):
    self.name = name
    self.type_params = type_params
    self.params = params
    self.return_type_spec = return_type_spec
    self.stmts = stmts

  def accept(self, visitor):
    return visitor.visit(self, {
        'params': [param.accept(visitor) for param in self.params],
        'return_type_spec': (
            self.return_type_spec.accept(visitor) if self.return_type_spec else
            None),
        'stmts': [stmt.accept(visitor) for stmt in self.stmts],
    })


class ExternFunc(Node):
  """An external (C/C++) function declaration."""

  def __init__(self, name, params, return_type_spec):
    self.name = name
    self.params = params
    self.return_type_spec = return_type_spec

  def accept(self, visitor):
    return visitor.visit(self, {
        'params': [param.accept(visitor) for param in self.params],
        'return_type_spec': (
            self.return_type_spec.accept(visitor) if self.return_type_spec else
            None),
    })


class Class(Node):
  """A class definition."""

  def __init__(self, name, type_params, super_classes, members):
    self.name = name
    self.type_params = type_params
    self.super_classes = super_classes
    self.members = members

  def accept(self, visitor):
    return visitor.visit(self, {
        'super_classes': [
            super_class.accept(visitor) for super_class in self.super_classes
        ],
        'members': [member.accept(visitor) for member in self.members],
    })


class Module(Node):
  """A module definition."""

  def __init__(self, members):
    self.members = members

  def accept(self, visitor):
    return visitor.visit(self, {
        'members': [member.accept(visitor) for member in self.members],
    })


class Visitor(object):
  """Base class for AST visitors."""

  def visit(self, node, child_node_results={}):
    type_name = type(node).__name__
    visit_fn_name = 'visit_%s' % inflection.underscore(type_name)
    visit_fn = getattr(self, visit_fn_name, self.unknown_node_type)
    return visit_fn(node, child_node_results)

  def unknown_node_type(self, node, _):
    visitor_type_name = type(self).__name__
    type_name = type(node).__name__
    error_message = 'Visitor %s has no defined visit method for class %s' % (
        visitor_type_name, type_name)
    logging.error(error_message)
    raise NotImplementedError(error_message)


class SerializerVisitor(Visitor):
  """An AST visitor that serializes an AST for debugging."""

  def visit_constant_expr(self, node, args):
    return {'type': 'ConstantExpr', 'value': node.value}

  def visit_var_expr(self, node, args):
    return {'type': 'VarExpr', 'var': node.var}

  def visit_member_expr(self, node, args):
    return {
        'type': 'MemberExpr',
        'expr': args['expr'],
        'member': node.member,
    }

  def visit_index_expr(self, node, args):
    return {
        'type': 'IndexExpr',
        'expr': args['expr'],
        'index_expr': args['index_expr'],
    }

  def visit_call_expr(self, node, args):
    return {
        'type': 'CallExpr',
        'expr': args['expr'],
        'arg_exprs': args['arg_exprs'],
    }

  def visit_unary_op_expr(self, node, args):
    return {'type': 'UnaryOpExpr', 'op': node.op, 'expr': args['expr']}

  def visit_binary_op_expr(self, node, args):
    return {
        'type': 'BinaryOpExpr',
        'op': node.op,
        'left_expr': args['left_expr'],
        'right_expr': args['right_expr'],
    }

  def visit_assign_expr(self, node, args):
    return {
        'type': 'AssignExpr',
        'dest_expr': args['dest_expr'],
        'expr': args['expr'],
    }

  def visit_expr_stmt(self, node, args):
    return {'type': 'ExprStmt', 'expr': args['expr']}

  def visit_return_stmt(self, node, args):
    return {'type': 'ReturnStmt', 'expr': args['expr']}

  def visit_break_stmt(self, node, args):
    return {'type': 'BreakStmt'}

  def visit_continue_stmt(self, node, args):
    return {'type': 'ContinueStmt'}

  def visit_cond_stmt(self, node, args):
    return {
        'type': 'CondStmt',
        'cond_expr': args['cond_expr'],
        'true_stmts': args['true_stmts'],
        'false_stmts': args['false_stmts'],
    }

  def visit_cond_loop_stmt(self, node, args):
    return {
        'type': 'CondStmt',
        'cond_expr': args['cond_expr'],
        'stmts': args['stmts'],
    }

  def visit_type_spec(self, node, args):
    return {'type': 'TypeSpec', 'name': node.name, 'params': args['params'], }

  def visit_member_type_spec(self, node, args):
    result = self.visit_type_spec(node, args)

    result.update({
        'type': 'MemberTypeSpec',
        'parent_type_spec': args['parent_type_spec'],
    })
    return result

  def visit_var_decl_stmt(self, node, args):
    return {
        'type': 'VarDeclStmt',
        'name': node.name,
        'type_spec': args['type_spec'],
        'mode': node.mode,
        'init_expr': args['init_expr'],
    }

  def visit_func_param(self, node, args):
    return {
        'type': 'FuncParam',
        'name': node.name,
        'mode': node.mode,
        'type_spec': args['type_spec'],
        'init_expr': args['init_expr'],
    }

  def visit_func(self, node, args):
    return {
        'type': 'Func',
        'name': node.name,
        'type_params': node.type_params,
        'params': args['params'],
        'return_type_spec': args['return_type_spec'],
        'stmts': args['stmts'],
    }

  def visit_extern_func(self, node, args):
    return {
        'type': 'ExternFunc',
        'name': node.name,
        'params': args['params'],
        'return_type_spec': args['return_type_spec'],
    }

  def visit_class(self, node, args):
    return {
        'type': 'Class',
        'name': node.name,
        'type_params': node.type_params,
        'super_classes': args['super_classes'],
        'members': args['members'],
    }

  def visit_module(self, node, args):
    return {'type': 'Module', 'members': args['members'], }
