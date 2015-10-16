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
"""Unit test for parser."""
# pylint: disable=missing-docstring

import unittest
import quo_parser


class QuoParserTest(unittest.TestCase):

  maxDiff = None

  def assert_ast_match(self, input_str, start, expected_ast_json):
    parser = quo_parser.create_parser(start=start, write_tables=False)
    actual_ast = parser.parse(input_str)
    if isinstance(actual_ast, list):
      actual_ast_json = [ast_node.to_json() for ast_node in actual_ast]
    else:
      actual_ast_json = actual_ast.to_json()
    self.assertEqual(actual_ast_json, expected_ast_json)

  def test_primary(self):
    self.assert_ast_match('''a[b.foo[0]['hello']].bar''', 'expr', {
        'type': 'MemberExpr',
        'expr': {
            'type': 'IndexExpr',
            'expr': {'type': 'VarExpr',
                     'var': 'a'},
            'index_expr': {
                'type': 'IndexExpr',
                'expr': {
                    'type': 'IndexExpr',
                    'expr': {
                        'type': 'MemberExpr',
                        'expr': {'type': 'VarExpr',
                                 'var': 'b'},
                        'member': 'foo',
                    },
                    'index_expr': {'type': 'ConstantExpr',
                                   'value': '0'}
                },
                'index_expr': {'type': 'ConstantExpr',
                               'value': 'hello'}
            }
        },
        'member': 'bar',
    })
    self.assert_ast_match('(((a)()(c,)(d()))(e.f()[1],g,h[i].j(),))', 'expr', {
        'type': 'CallExpr',
        'expr': {
            'type': 'CallExpr',
            'expr': {
                'type': 'CallExpr',
                'expr': {
                    'type': 'CallExpr',
                    'expr': {'type': 'VarExpr',
                             'var': 'a'},
                    'arg_exprs': [],
                },
                'arg_exprs': [{'type': 'VarExpr',
                               'var': 'c'}],
            },
            'arg_exprs': [{
                'type': 'CallExpr',
                'expr': {'type': 'VarExpr',
                         'var': 'd'},
                'arg_exprs': [],
            }],
        },
        'arg_exprs': [
            {
                'type': 'IndexExpr',
                'expr': {
                    'type': 'CallExpr',
                    'expr': {
                        'type': 'MemberExpr',
                        'expr': {'type': 'VarExpr',
                                 'var': 'e'},
                        'member': 'f',
                    },
                    'arg_exprs': [],
                },
                'index_expr': {'type': 'ConstantExpr',
                               'value': '1'},
            },
            {'type': 'VarExpr',
             'var': 'g'},
            {
                'type': 'CallExpr',
                'expr': {
                    'type': 'MemberExpr',
                    'expr': {
                        'type': 'IndexExpr',
                        'expr': {'type': 'VarExpr',
                                 'var': 'h'},
                        'index_expr': {'type': 'VarExpr',
                                       'var': 'i'},
                    },
                    'member': 'j',
                },
                'arg_exprs': []
            },
        ],
    })

  def test_binary_arith(self):
    self.assert_ast_match('a+b*c-d==e--f/g%-+-h', 'expr', {
        'type': 'BinaryOpExpr',
        'op': 'EQ',
        'left_expr': {
            'type': 'BinaryOpExpr',
            'op': 'SUB',
            'left_expr': {
                'type': 'BinaryOpExpr',
                'op': 'ADD',
                'left_expr': {'type': 'VarExpr',
                              'var': 'a'},
                'right_expr': {
                    'type': 'BinaryOpExpr',
                    'op': 'MUL',
                    'left_expr': {'type': 'VarExpr',
                                  'var': 'b'},
                    'right_expr': {'type': 'VarExpr',
                                   'var': 'c'},
                },
            },
            'right_expr': {'type': 'VarExpr',
                           'var': 'd'},
        },
        'right_expr': {
            'type': 'BinaryOpExpr',
            'op': 'SUB',
            'left_expr': {'type': 'VarExpr',
                          'var': 'e'},
            'right_expr': {
                'type': 'BinaryOpExpr',
                'op': 'MOD',
                'left_expr': {
                    'type': 'BinaryOpExpr',
                    'op': 'DIV',
                    'left_expr': {
                        'type': 'UnaryOpExpr',
                        'op': 'SUB',
                        'expr': {'type': 'VarExpr',
                                 'var': 'f'},
                    },
                    'right_expr': {'type': 'VarExpr',
                                   'var': 'g'},
                },
                'right_expr': {
                    'type': 'UnaryOpExpr',
                    'op': 'SUB',
                    'expr': {
                        'type': 'UnaryOpExpr',
                        'op': 'ADD',
                        'expr': {
                            'type': 'UnaryOpExpr',
                            'op': 'SUB',
                            'expr': {'type': 'VarExpr',
                                     'var': 'h'},
                        },
                    },
                },
            },
        },
    })

  def test_binary_bool(self):
    self.assert_ast_match('a>b or not c<=d and not false or true', 'expr', {
        'type': 'BinaryOpExpr',
        'op': 'OR',
        'left_expr': {
            'type': 'BinaryOpExpr',
            'op': 'OR',
            'left_expr': {
                'type': 'BinaryOpExpr',
                'op': 'GT',
                'left_expr': {'type': 'VarExpr',
                              'var': 'a'},
                'right_expr': {'type': 'VarExpr',
                               'var': 'b'},
            },
            'right_expr': {
                'type': 'BinaryOpExpr',
                'op': 'AND',
                'left_expr': {
                    'type': 'UnaryOpExpr',
                    'op': 'NOT',
                    'expr': {
                        'type': 'BinaryOpExpr',
                        'op': 'LE',
                        'left_expr': {'type': 'VarExpr',
                                      'var': 'c'},
                        'right_expr': {'type': 'VarExpr',
                                       'var': 'd'},
                    },
                },
                'right_expr': {
                    'type': 'UnaryOpExpr',
                    'op': 'NOT',
                    'expr': {'type': 'ConstantExpr',
                             'value': False},
                },
            },
        },
        'right_expr': {'type': 'ConstantExpr',
                       'value': True},
    })

  def test_assign(self):
    self.assert_ast_match('a += b = c -= d', 'expr', {
        'type': 'AssignExpr',
        'dest_expr': {'type': 'VarExpr',
                      'var': 'a'},
        'expr': {
            'type': 'BinaryOpExpr',
            'op': 'ADD',
            'left_expr': {'type': 'VarExpr',
                          'var': 'a'},
            'right_expr': {
                'type': 'AssignExpr',
                'dest_expr': {'type': 'VarExpr',
                              'var': 'b'},
                'expr': {
                    'type': 'AssignExpr',
                    'dest_expr': {'type': 'VarExpr',
                                  'var': 'c'},
                    'expr': {
                        'type': 'BinaryOpExpr',
                        'op': 'SUB',
                        'left_expr': {'type': 'VarExpr',
                                      'var': 'c'},
                        'right_expr': {'type': 'VarExpr',
                                       'var': 'd'},
                    },
                },
            },
        },
    })

  def test_cond_stmt(self):
    self.assert_ast_match('''
    if a != b { if 3 {} if 4 {}} else if true { hello; } else { bar; }
    ''', 'stmt', {
        'type': 'CondStmt',
        'cond_expr': {
            'type': 'BinaryOpExpr',
            'op': 'NE',
            'left_expr': {'type': 'VarExpr',
                          'var': 'a'},
            'right_expr': {'type': 'VarExpr',
                           'var': 'b'},
        },
        'true_stmts': [
            {
                'type': 'CondStmt',
                'cond_expr': {'type': 'ConstantExpr',
                              'value': '3'},
                'true_stmts': [],
                'false_stmts': [],
            },
            {
                'type': 'CondStmt',
                'cond_expr': {'type': 'ConstantExpr',
                              'value': '4'},
                'true_stmts': [],
                'false_stmts': [],
            },
        ],
        'false_stmts': [{
            'type': 'CondStmt',
            'cond_expr': {'type': 'ConstantExpr',
                          'value': True},
            'true_stmts': [{
                'type': 'ExprStmt',
                'expr': {'type': 'VarExpr',
                         'var': 'hello'},
            }],
            'false_stmts': [{
                'type': 'ExprStmt',
                'expr': {'type': 'VarExpr',
                         'var': 'bar'},
            }],
        }],
    })

  def test_type_spec(self):
    self.assert_ast_match('A<B,C<D.E<F>.G,>,H<I>.J<>>', 'type_spec', {
        'type': 'TypeSpec',
        'name': 'A',
        'params': [
            {
                'type': 'TypeSpec',
                'name': 'B',
                'params': [],
            },
            {
                'type': 'TypeSpec',
                'name': 'C',
                'params': [{
                    'type': 'MemberTypeSpec',
                    'name': 'G',
                    'params': [],
                    'parent_type_spec': {
                        'type': 'MemberTypeSpec',
                        'name': 'E',
                        'params': [{
                            'type': 'TypeSpec',
                            'name': 'F',
                            'params': [],
                        }],
                        'parent_type_spec': {
                            'type': 'TypeSpec',
                            'name': 'D',
                            'params': [],
                        },
                    },
                }],
            },
            {
                'type': 'MemberTypeSpec',
                'name': 'J',
                'params': [],
                'parent_type_spec': {
                    'type': 'TypeSpec',
                    'name': 'H',
                    'params': [{
                        'type': 'TypeSpec',
                        'name': 'I',
                        'params': [],
                    }],
                },
            },
        ],
    })

  def test_var_decl_stmts(self):
    self.assert_ast_match('''
    var a Array<Int>;
    var {
      b = 3, c, d = 5 + 2 Int;
      e = '' String;
      f, g;
    }
    var h;
    ''', 'stmts', [
        {
            'type': 'VarDeclStmt',
            'name': 'a',
            'type_spec': {
                'type': 'TypeSpec',
                'name': 'Array',
                'params': [{
                    'type': 'TypeSpec',
                    'name': 'Int',
                    'params': [],
                }],
            },
            'init_expr': None,
        },
        {
            'type': 'VarDeclStmt',
            'name': 'b',
            'type_spec': {
                'type': 'TypeSpec',
                'name': 'Int',
                'params': [],
            },
            'init_expr': {
                'type': 'ConstantExpr',
                'value': '3',
            },
        },
        {
            'type': 'VarDeclStmt',
            'name': 'c',
            'type_spec': {
                'type': 'TypeSpec',
                'name': 'Int',
                'params': [],
            },
            'init_expr': None,
        },
        {
            'type': 'VarDeclStmt',
            'name': 'd',
            'type_spec': {
                'type': 'TypeSpec',
                'name': 'Int',
                'params': [],
            },
            'init_expr': {
                'type': 'BinaryOpExpr',
                'op': 'ADD',
                'left_expr': {'type': 'ConstantExpr',
                              'value': '5'},
                'right_expr': {'type': 'ConstantExpr',
                               'value': '2'},
            },
        },
        {
            'type': 'VarDeclStmt',
            'name': 'e',
            'type_spec': {
                'type': 'TypeSpec',
                'name': 'String',
                'params': [],
            },
            'init_expr': {
                'type': 'ConstantExpr',
                'value': '',
            },
        },
        {
            'type': 'VarDeclStmt',
            'name': 'f',
            'type_spec': None,
            'init_expr': None,
        },
        {
            'type': 'VarDeclStmt',
            'name': 'g',
            'type_spec': None,
            'init_expr': None,
        },
        {
            'type': 'VarDeclStmt',
            'name': 'h',
            'type_spec': None,
            'init_expr': None,
        },
    ])

  def test_func(self):
    self.assert_ast_match('''
    function foo(a, b Int, c = 0, d = 0 Int) {
      return a + b + c + d;
    }
    ''', 'func', {
        'type': 'Func',
        'name': 'foo',
        'params': [
            {
                'type': 'Param',
                'name': 'a',
                'type_spec': None,
                'init_expr': None,
            },
            {
                'type': 'Param',
                'name': 'b',
                'type_spec': {
                    'type': 'TypeSpec',
                    'name': 'Int',
                    'params': [],
                },
                'init_expr': None,
            },
            {
                'type': 'Param',
                'name': 'c',
                'type_spec': None,
                'init_expr': {
                    'type': 'ConstantExpr',
                    'value': '0',
                },
            },
            {
                'type': 'Param',
                'name': 'd',
                'type_spec': {
                    'type': 'TypeSpec',
                    'name': 'Int',
                    'params': [],
                },
                'init_expr': {
                    'type': 'ConstantExpr',
                    'value': '0',
                },
            },
        ],
        'return_type_spec': None,
        'stmts': [
            {
                'type': 'ReturnStmt',
                'expr': {
                    'type': 'BinaryOpExpr',
                    'op': 'ADD',
                    'left_expr': {
                        'type': 'BinaryOpExpr',
                        'op': 'ADD',
                        'left_expr': {
                            'type': 'BinaryOpExpr',
                            'op': 'ADD',
                            'left_expr': {'type': 'VarExpr',
                                          'var': 'a'},
                            'right_expr': {'type': 'VarExpr',
                                           'var': 'b'},
                        },
                        'right_expr': {'type': 'VarExpr',
                                       'var': 'c'},
                    },
                    'right_expr': {'type': 'VarExpr',
                                   'var': 'd'},
                },
            },
        ],
    })
    self.assert_ast_match('''
    function foo() Array<Int> {}
    ''', 'func', {
        'type': 'Func',
        'name': 'foo',
        'params': [],
        'return_type_spec': {
            'type': 'TypeSpec',
            'name': 'Array',
            'params': [{
                'type': 'TypeSpec',
                'name': 'Int',
                'params': [],
            }],
        },
        'stmts': [],
    })
