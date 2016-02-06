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
modules. It will read from files supplied on the command line, or stdin if no
files are specified.
"""

import ctypes
import datetime
import jinja2
import os
import logging
import quo_ast
import quo_lexer
import quo_parser
import shutil
import string
import subprocess
import tempfile


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
    if node.op in ('BORROW', 'MOVE') and args['expr'][0] == '*':
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
      if (isinstance(node.init_expr, quo_ast.UnaryOpExpr) and
          node.init_expr.op == 'MOVE'):
        local_decl = '::std::unique_ptr<%s> %s = %s;' % (
            type_spec, node.name, init_expr)
        class_member_init_expr = '%s = %s;' % (
            node.name, init_expr);
      else:
        local_decl = '::std::unique_ptr<%s> %s(new %s(%s));' % (
            type_spec, node.name, type_spec, init_expr)
        class_member_init_expr = '%s.reset(new %s(%s));' % (
            node.name, type_spec, init_expr);
      class_member_decl = '::std::unique_ptr<%s> %s;' % (
          type_spec, node.name)
    elif node.mode == 'BORROW':
      init_expr = ' = %s' % args['init_expr'] if args['init_expr'] else ''
      local_decl = '%s* %s%s;' % (
          type_spec, node.name, init_expr)
      class_member_decl = '%s* %s;' % (type_spec, node.name)
      class_member_init_expr = (
          '%s%s;' % (node.name, init_expr) if args['init_expr'] else None)
    else:
      raise NotImplementedError('Unknown var mode: %s' % node.mode)

    return (local_decl, class_member_decl, class_member_init_expr)

  def visit_func_param(self, node, args):
    base_type = args['type_spec'] if args['type_spec'] else 'Object'
    if node.mode == 'COPY':
      name = '_' + node.name
      type_spec = 'const %s&' % base_type
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
    return_base_type = (
        args['return_type_spec'] if args['return_type_spec'] else 'Object')
    if node.return_mode == 'COPY':
      return_type = return_base_type
    elif node.return_mode == 'BORROW':
      return_type = '%s*' % return_base_type
    elif node.return_mode == 'MOVE':
      return_type = '::std::unique_ptr<%s>' % return_base_type
    else:
      raise NotImplementedError('Unknown return mode: %s' % node.returnmode)
    stmts = [
        param[1] for param in args['params'] if param[1] is not None
    ]
    for i in range(len(node.stmts)):
      if isinstance(node.stmts[i], quo_ast.VarDeclStmt):
        stmt = args['stmts'][i][0]
      else:
        stmt = args['stmts'][i]
      stmts.append(stmt)
    if node.cc == 'DEFAULT':
      cc = ''
    elif node.cc == 'C':
      cc = 'extern "C" '
    else:
      raise NotImplementedError('Unknown calling convention: %s' % node.cc)
    return '%s%s%s %s%s(%s) {\n%s\n}' % (
        cc,
        self.translate_template(node), return_type, node.name,
        self.translate_type_params(node), ', '.join(
            param[0]
            for param in args['params']),
        self.indent_stmts(stmts))

  def visit_extern_func(self, node, args):
    return_type_spec = (
        args['return_type_spec'] if args['return_type_spec'] else 'Object')
    return 'extern "C" %s %s(%s);' % (
        return_type_spec,
        node.name,
        ', '.join(param[0] for param in args['params']))

  def visit_class(self, node, args):
    inheritance = (
        ' : %s' % ', '.join(
            'public ' + super_class for super_class in args['super_classes'])
        if args['super_classes'] else '')
    members = args['members'][:]
    constructor_stmts = []
    for i in range(len(node.members)):
      if isinstance(node.members[i], quo_ast.Func):
        members[i] = 'virtual ' + members[i]
      elif isinstance(node.members[i], quo_ast.VarDeclStmt):
        constructor_stmts.append(members[i][2])
        members[i] = members[i][1]
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
    constructor = 'public: %s() {\n%s\n}' % (
        node.name, self.indent_stmts(constructor_stmts))
    return '%sclass %s%s%s {\n%s\n};' % (
        self.translate_template(node), node.name,
        self.translate_type_params(node), inheritance,
        self.indent_stmts([constructor] + members))

  def visit_module(self, node, args):
    members = args['members'][:]
    for i in range(len(node.members)):
      if isinstance(node.members[i], quo_ast.VarDeclStmt):
        members[i] = members[i][0]

      if (isinstance(node.members[i], quo_ast.ExternFunc) or
          self.is_public(node.members[i]) or
          isinstance(node.members[i], quo_ast.Func) and
          node.members[i].name == 'main'):
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


def quo_to_cpp(quo_str):
  """Translates a Quo module to C++."""
  parser = quo_parser.create_parser()
  lexer = quo_lexer.create_lexer()
  ast = parser.parse(quo_str, lexer=lexer)
  translator_visitor = CppTranslatorVisitor()
  content = ast.accept(translator_visitor)

  jinja_env = jinja2.Environment(
      loader=jinja2.FileSystemLoader(os.path.dirname(__file__)))
  template = jinja_env.get_template('quo_module_template.cpp')
  return template.render(
      ts=datetime.datetime.now().isoformat(), content=content)


def get_cpp_compiler():
  """Returns the C++ compiler binary name based on availability."""
  if 'CXX' in os.environ:
    return os.environ['CXX']
  candidates = ['clang++', 'g++']
  for candidate in candidates:
    path = shutil.which(candidate)
    if path is not None:
      return path
  raise IOError('Cannot find C++ compiler')


# Directory containing C++ runtime code.
_CPP_RUNTIME_DIR = 'cpp_runtime'


def compile_cpp(cpp_str, dest_dir=None):
  """Compiles a C++ module to a shared lib and returns a path to the result."""
  if dest_dir is None:
    dest_dir = tempfile.gettempdir()
  fd, output_file_path = tempfile.mkstemp(suffix='.so', dir=dest_dir)
  os.close(fd)
  cpp_runtime_dir_path = os.path.join(
      os.path.realpath(os.path.dirname(__file__)),
      _CPP_RUNTIME_DIR)
  cmd = [
      get_cpp_compiler(),
      '-Wall',
      '-shared',
      '-fPIC',
      '-xc++',
      '-std=c++0x',
      '-I%s' % cpp_runtime_dir_path,
      '-o', output_file_path,
      '-',
  ]
  logging.info('Spawning compiler: %s', ' '.join(cmd))
  compiler_proc = subprocess.run(cmd, input=cpp_str.encode('utf-8'), check=True)
  return output_file_path


def compile_and_load(quo_str, dest_dir=None):
  """Compiles a Quo module and returns it as a loaded shared lib."""
  logging.info('Compiling Quo module:\n%s', quo_str)

  cpp_str = quo_to_cpp(quo_str)
  logging.info('Generated C++ code:\n%s', cpp_str)

  lib_path = compile_cpp(cpp_str)

  logging.info('Loading shared library %s', lib_path)
  lib = ctypes.cdll.LoadLibrary(lib_path)
  os.remove(lib_path)
  return lib


if __name__ == '__main__':
  import fileinput

  # Set up logging.
  logging.basicConfig(
      level=logging.INFO,
      style='{',
      format='{levelname:.1}{asctime} {filename}:{lineno}] {message}')

  lib = compile_and_load(''.join(fileinput.input()))

  logging.info('Executing main()')
  exit_code = lib.main()
  logging.info('Return code: %d', exit_code)
