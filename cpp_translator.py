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
"""Quo to C++ translator.

For debugging, run this module to print out the translated C++ module for Quo
modules. It Will read from files supplied on the command line, or stdin if no
files are specified.
"""

import quo_ast
import string


class CppTranslatorVisitor(quo_ast.Visitor):
  """AST visitor that converts an AST to C++ code.

  Each visitor method returns a C++ code snippet as a string.
  """

  def visit_constant_expr(self, node, args):
    if node.value is None:
      return 'nullptr'
    elif isinstance(node.value, str):
      return '"%s"' % node.value
    elif isinstance(node.value, bool):
      return str(node.value).lower()
    elif isinstance(node.value, int):
      return str(node.value)
    else:
      raise NotImplementedError(
          'Unknown constant type: %s' % type(node.value).__name__)

  def visit_var_expr(self, node, args):
    return '*%s' % node.var

  def visit_member_expr(self, node, args):
    return '*(%s).%s' % (args['expr'], node.member)

  def visit_index_expr(self, node, args):
    return '*(%s)[%s]' % (args['expr'], args['index_expr'])

  def visit_call_expr(self, node, args):
    if args['expr'].startswith('*'):
      expr = args['expr'][1:]
    else:
      expr = '(%s)' % args['expr']
    return '%s(%s)' % (expr, ', '.join(args['arg_exprs']))

  def visit_unary_op_expr(self, node, args):
    op_map = {
        'ADD': '+(%s)',
        'SUB': '-(%s)',
        'NOT': '!(%s)',
        'BORROW': '&(*(%s))',
        'MOVE': 'std::move(%s)',
    }
    if node.op not in op_map:
      raise NotImplementedError('Unknown unary op: %s' % node.op)
    if node.op in ('BORROW', 'MOVE'):
      if args['expr'][0] != '*':
        raise ValueError('Invalid borrow / move operand: %s' % args['expr'])
      expr = args['expr'][1:]
    else:
      expr = args['expr']
    return op_map[node.op] % expr

  def visit_binary_op_expr(self, node, args):
    op_map = {
        'ADD': '+',
        'SUB': '-',
        'MUL': '*',
        'DIV': '/',
        'MOD': '%',
        'EQ': '==',
        'NE': '!=',
        'GT': '>',
        'GE': '>=',
        'LT': '<',
        'LE': '<=',
        'AND': '&&',
        'OR': '||',
    }
    if node.op not in op_map:
      raise NotImplementedError('Unknown binary op: %s' % node.op)
    return '(%s) %s (%s)' % (
        args['left_expr'], op_map[node.op], args['right_expr'])

  def visit_assign_expr(self, node, args):
    if args['dest_expr'][0] != '*':
      raise ValueError('Invalid LHS in assignment: %s' % args['dest_expr'])
    if (isinstance(node.expr, quo_ast.UnaryOpExpr) and
        node.expr.op == 'BORROW'):
      dest_expr = args['dest_expr'][1:]
    elif (isinstance(node.expr, quo_ast.UnaryOpExpr) and
          node.expr.op == 'MOVE'):
      dest_expr = args['dest_expr'][1:]
    else:
      dest_expr = args['dest_expr']
    return '%s = %s' % (dest_expr, args['expr'])

  def visit_expr_stmt(self, node, args):
    return '%s;' % (args['expr'])

  def visit_return_stmt(self, node, args):
    if args['expr'] is None:
      return 'return;'
    return 'return %s;' % args['expr']

  def visit_break_stmt(self, node, args):
    return 'break;'

  def visit_continue_stmt(self, node, args):
    return 'continue;'

  def visit_cond_stmt(self, node, args):
    stmt = 'if (%s) {\n%s\n}' % (
        args['cond_expr'], self.indent_stmts(args['true_stmts']))
    if node.false_stmts:
      stmt += ' else {\n%s\n}' % self.indent_stmts(args['false_stmts'])
    return stmt

  def visit_cond_loop_stmt(self, node, args):
    return 'while (%s) {\n%s\n}' % (
        args['cond_expr'], self.indent_stmts(args['stmts']))

  def visit_type_spec(self, node, args):
    return self.translate_base_type(node, args)

  def visit_member_type_spec(self, node, args):
    return '%s::%s' % (
        args['parent_type_spec'], self.translate_base_type(node, args))

  def visit_var_decl_stmt(self, node, args):
    type_spec = args['type_spec'] if args['type_spec'] else 'Object'
    if node.mode == 'OWN':
      init_expr = args['init_expr'] if args['init_expr'] else ''
      return '::std::unique_ptr<%s> %s(new %s(%s));' % (
          type_spec, node.name, type_spec, init_expr)
    elif node.mode == 'BORROW':
      init_expr = ' = %s' % args['init_expr'] if args['init_expr'] else ''
      return '%s* %s%s;' % (
          type_spec, node.name, init_expr)
    else:
      raise NotImplementedError('Unknown var mode: %s' % node.mode)

  def visit_func_param(self, node, args):
    base_type = args['type_spec'] if args['type_spec'] else 'Object'
    if node.mode == 'COPY':
      name = '_' + node.name
      type_spec = base_type
      init_stmt = '::std::unique_ptr<%s> %s(new %s(%s));' % (
          base_type, node.name, base_type, name)
    elif node.mode == 'BORROW':
      name = node.name
      type_spec = '%s*' % base_type
      init_stmt = None
    elif node.mode == 'MOVE':
      name = node.name
      type_spec = '::std::unique_ptr<%s>' % base_type
      init_stmt = None
    else:
      raise NotImplementedError('Unknown param mode: %s' % node.mode)
    init_expr = ' = %s' % args['init_expr'] if args['init_expr'] else ''
    return ('%s %s%s' % (type_spec, name, init_expr), init_stmt)

  def visit_func(self, node, args):
    return_type_spec = (
        args['return_type_spec'] if args['return_type_spec'] else 'Object')
    stmts = [
        param[1] for param in args['params'] if param[1] is not None
    ] + args['stmts']
    return '%s%s %s%s(%s) {\n%s\n}' % (
        self.translate_template(node), return_type_spec, node.name,
        self.translate_type_params(node), ', '.join(
            param[0]
            for param in args['params']),
        self.indent_stmts(stmts))

  def visit_class(self, node, args):
    inheritance = (
        ' : public %s' % ', '.join(args['super_classes']) if
        args['super_classes'] else '')
    members = args['members'][:]
    for i in range(len(node.members)):
      if isinstance(node.members[i], quo_ast.Func):
        members[i] = 'virtual ' + members[i]
      if self.is_public(node.members[i]):
        members[i] = 'public: ' + members[i]
      elif self.is_protected(node.members[i]):
        members[i] = 'protected: ' + members[i]
      elif self.is_private(node.members[i]):
        members[i] = 'private: ' + members[i]
      else:
        raise ValueError(
            'Cannot determine visibility of class member: %s' %
            node.members[i].name)
    return '%sclass %s%s%s {\n%s\n}' % (
        self.translate_template(node), node.name,
        self.translate_type_params(node), inheritance,
        self.indent_stmts(members))

  def visit_module(self, node, args):
    members = args['members'][:]
    for i in range(len(node.members)):
      if self.is_public(node.members[i]):
        pass
      elif self.is_protected(node.members[i]):
        raise ValueError(
            'Protected class member at module scope: %s' % node.members[i].name)
      elif self.is_private(node.members[i]):
        members[i] = 'static ' + members[i]
    return '\n\n'.join(members)

  @staticmethod
  def indent_stmts(stmts, indent_str='  '):
    lines = []
    for stmt in stmts:
      lines.extend(stmt.split('\n'))
    return '\n'.join((indent_str + line) for line in lines)

  @staticmethod
  def translate_base_type(node, args):
    params = '<%s>' % ', '.join(args['params']) if args['params'] else ''
    return node.name + params

  @staticmethod
  def translate_type_params(node):
    return '<%s>' % (', '.join(node.type_params)) if node.type_params else ''

  @staticmethod
  def translate_template(node):
    return 'template<%s>\n' % (', '.join(
        'typename ' + type_param
        for type_param in node.type_params)) if node.type_params else ''

  @staticmethod
  def is_public(node):
    return node.name[0] in string.ascii_uppercase

  @staticmethod
  def is_protected(node):
    return node.name[0] == '_'

  @staticmethod
  def is_private(node):
    return node.name[0] in string.ascii_lowercase


if __name__ == '__main__':
  import fileinput
  import quo_lexer
  import quo_parser
  import yaml

  parser = quo_parser.create_parser()
  lexer = quo_lexer.create_lexer()
  ast = parser.parse('\n'.join(fileinput.input()), lexer=lexer)
  translator_visitor = CppTranslatorVisitor()
  print(ast.accept(translator_visitor))
