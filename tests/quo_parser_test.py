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
