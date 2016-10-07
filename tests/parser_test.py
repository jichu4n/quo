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
from google.protobuf import json_format
from ast.ast_pb2 import *
from parser import parser


class QuoParserTest(unittest.TestCase):

  maxDiff = None

  def assert_ast_match(self, input_str, start, expected_ast):
    parse = parser.create_parser(start=start)
    actual_ast = parse.parse(input_str)
    if isinstance(actual_ast, list):
      actual_ast_serialized = [
          json_format.MessageToDict(ast_node)
          for ast_node in actual_ast
      ]
    else:
      actual_ast_serialized = json_format.MessageToDict(actual_ast)
    if isinstance(expected_ast, list):
      expected_ast_serialized = [
          json_format.MessageToDict(ast_node)
          for ast_node in expected_ast
      ]
    else:
      expected_ast_serialized = json_format.MessageToDict(expected_ast)
    self.assertEqual(actual_ast_serialized, expected_ast_serialized)

  def test_primary(self):
    self.assert_ast_match(
        '''a[b.foo[0]['hello']].bar''',
        'expr',
        Expr(member=MemberExpr(
            expr=Expr(index=IndexExpr(
                expr=Expr(var=VarExpr(var='a')),
                index_expr=Expr(index=IndexExpr(
                    expr=Expr(index=IndexExpr(
                        expr=Expr(member=MemberExpr(
                            expr=Expr(var=VarExpr(var='b')),
                            member='foo')),
                        index_expr=Expr(constant=ConstantExpr(intValue=0)))),
                    index_expr=Expr(
                        constant=ConstantExpr(stringValue='hello')))))),
            member='bar')))
    self.assert_ast_match(
        '(((a)()(c,)(d()))(e.f()[1],g,h[i].j(),))',
        'expr',
        Expr(call=CallExpr(
            expr=Expr(call=CallExpr(
                expr=Expr(call=CallExpr(
                    expr=Expr(call=CallExpr(
                        expr=Expr(var=VarExpr(var='a')))),
                    arg_exprs=[Expr(var=VarExpr(var='c'))])),
                arg_exprs=[
                    Expr(call=CallExpr(
                        expr=Expr(var=VarExpr(var='d')))),
                ])),
            arg_exprs=[
                Expr(index=IndexExpr(
                    expr=Expr(call=CallExpr(
                        expr=Expr(member=MemberExpr(
                            expr=Expr(var=VarExpr(var='e')),
                            member='f')))),
                    index_expr=Expr(constant=ConstantExpr(intValue=1)))),
                Expr(var=VarExpr(var='g')),
                Expr(call=CallExpr(
                    expr=Expr(member=MemberExpr(
                        expr=Expr(index=IndexExpr(
                            expr=Expr(var=VarExpr(var='h')),
                            index_expr=Expr(var=VarExpr(var='i')))),
                        member='j')))),
            ])))

  def test_binary_arith(self):
    self.assert_ast_match(
        'a+b*c-d==e--f/g%~-+-h',
        'expr',
        Expr(binary_op=BinaryOpExpr(
            op=BinaryOpExpr.EQ,
            left_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.SUB,
                left_expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.ADD,
                    left_expr=Expr(var=VarExpr(var='a')),
                    right_expr=Expr(binary_op=BinaryOpExpr(
                        op=BinaryOpExpr.MUL,
                        left_expr=Expr(var=VarExpr(var='b')),
                        right_expr=Expr(var=VarExpr(var='c')))))),
                right_expr=Expr(var=VarExpr(var='d')))),
            right_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.SUB,
                left_expr=Expr(var=VarExpr(var='e')),
                right_expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.MOD,
                    left_expr=Expr(binary_op=BinaryOpExpr(
                        op=BinaryOpExpr.DIV,
                        left_expr=Expr(unary_op=UnaryOpExpr(
                            op=UnaryOpExpr.SUB,
                            expr=Expr(var=VarExpr(var='f')))),
                        right_expr=Expr(var=VarExpr(var='g')))),
                    right_expr=Expr(unary_op=UnaryOpExpr(
                        op=UnaryOpExpr.MOVE,
                        expr=Expr(unary_op=UnaryOpExpr(
                            op=UnaryOpExpr.SUB,
                            expr=Expr(unary_op=UnaryOpExpr(
                                op=UnaryOpExpr.ADD,
                                expr=Expr(unary_op=UnaryOpExpr(
                                    op=UnaryOpExpr.SUB,
                                    expr=Expr(var=VarExpr(var='h'))))))))))))))
        )))

#  def test_binary_bool(self):
#    self.assert_ast_match('a>b or not c<=d and not false or true', 'expr', {
#        'type': 'BinaryOpExpr',
#        'op': 'OR',
#        'left_expr': {
#            'type': 'BinaryOpExpr',
#            'op': 'OR',
#            'left_expr': {
#                'type': 'BinaryOpExpr',
#                'op': 'GT',
#                'left_expr': {'type': 'VarExpr',
#                              'var': 'a'},
#                'right_expr': {'type': 'VarExpr',
#                               'var': 'b'},
#            },
#            'right_expr': {
#                'type': 'BinaryOpExpr',
#                'op': 'AND',
#                'left_expr': {
#                    'type': 'UnaryOpExpr',
#                    'op': 'NOT',
#                    'expr': {
#                        'type': 'BinaryOpExpr',
#                        'op': 'LE',
#                        'left_expr': {'type': 'VarExpr',
#                                      'var': 'c'},
#                        'right_expr': {'type': 'VarExpr',
#                                       'var': 'd'},
#                    },
#                },
#                'right_expr': {
#                    'type': 'UnaryOpExpr',
#                    'op': 'NOT',
#                    'expr': {'type': 'ConstantExpr',
#                             'value': False},
#                },
#            },
#        },
#        'right_expr': {'type': 'ConstantExpr',
#                       'value': True},
#    })
 
  def test_assign(self):
    self.assert_ast_match(
        'a += b = c -= d',
        'expr',
        Expr(assign=AssignExpr(
            dest_expr=Expr(var=VarExpr(var='a')),
            value_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.ADD,
                left_expr=Expr(var=VarExpr(var='a')),
                right_expr=Expr(assign=AssignExpr(
                    dest_expr=Expr(var=VarExpr(var='b')),
                    value_expr=Expr(assign=AssignExpr(
                        dest_expr=Expr(var=VarExpr(var='c')),
                        value_expr=Expr(binary_op=BinaryOpExpr(
                            op=BinaryOpExpr.SUB,
                            left_expr=Expr(var=VarExpr(var='c')),
                            right_expr=Expr(var=VarExpr(var='d')))))))))))))

  def test_cond_stmt(self):
    self.assert_ast_match('''
    if a != b { if 3 {} if 4 {}} else if true { hello; } else { bar; }
    ''',
    'stmt',
    Stmt(cond=CondStmt(
        cond_expr=Expr(binary_op=BinaryOpExpr(
            op=BinaryOpExpr.NE,
            left_expr=Expr(var=VarExpr(var='a')),
            right_expr=Expr(var=VarExpr(var='b')))),
        true_block=Block(stmts=[
            Stmt(cond=CondStmt(
                cond_expr=Expr(constant=ConstantExpr(intValue=3)),
                true_block=Block())),
            Stmt(cond=CondStmt(
                cond_expr=Expr(constant=ConstantExpr(intValue=4)),
                true_block=Block()))
        ]),
        false_block=Block(stmts=[Stmt(cond=CondStmt(
            cond_expr=Expr(constant=ConstantExpr(boolValue=True)),
            true_block=Block(stmts=[
                Stmt(expr=ExprStmt(expr=Expr(var=VarExpr(var='hello')))),
            ]),
            false_block=Block(stmts=[
                Stmt(expr=ExprStmt(expr=Expr(var=VarExpr(var='bar')))),
            ])))]))))

#  def test_type_spec(self):
#    self.assert_ast_match('A<B,C<D.E<F>.G,>,H<I>.J<>>', 'type_spec', {
#        'type': 'TypeSpec',
#        'name': 'A',
#        'params': [
#            {
#                'type': 'TypeSpec',
#                'name': 'B',
#                'params': [],
#            },
#            {
#                'type': 'TypeSpec',
#                'name': 'C',
#                'params': [{
#                    'type': 'MemberTypeSpec',
#                    'name': 'G',
#                    'params': [],
#                    'parent_type_spec': {
#                        'type': 'MemberTypeSpec',
#                        'name': 'E',
#                        'params': [{
#                            'type': 'TypeSpec',
#                            'name': 'F',
#                            'params': [],
#                        }],
#                        'parent_type_spec': {
#                            'type': 'TypeSpec',
#                            'name': 'D',
#                            'params': [],
#                        },
#                    },
#                }],
#            },
#            {
#                'type': 'MemberTypeSpec',
#                'name': 'J',
#                'params': [],
#                'parent_type_spec': {
#                    'type': 'TypeSpec',
#                    'name': 'H',
#                    'params': [{
#                        'type': 'TypeSpec',
#                        'name': 'I',
#                        'params': [],
#                    }],
#                },
#            },
#        ],
#    })
#
#  def test_var_decl_stmts(self):
#    self.assert_ast_match('''
#    var a Array<Int>;
#    var {
#      b = 3, c, d = 5 + 2 Int;
#      e = '' String;
#      f, &g;
#    }
#    var h;
#    ''', 'stmts', [
#        {
#            'type': 'VarDeclStmt',
#            'name': 'a',
#            'type_spec': {
#                'type': 'TypeSpec',
#                'name': 'Array',
#                'params': [{
#                    'type': 'TypeSpec',
#                    'name': 'Int',
#                    'params': [],
#                }],
#            },
#            'mode': 'OWN',
#            'init_expr': None,
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'b',
#            'type_spec': {
#                'type': 'TypeSpec',
#                'name': 'Int',
#                'params': [],
#            },
#            'mode': 'OWN',
#            'init_expr': {
#                'type': 'ConstantExpr',
#                'value': 3,
#            },
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'c',
#            'type_spec': {
#                'type': 'TypeSpec',
#                'name': 'Int',
#                'params': [],
#            },
#            'mode': 'OWN',
#            'init_expr': None,
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'd',
#            'type_spec': {
#                'type': 'TypeSpec',
#                'name': 'Int',
#                'params': [],
#            },
#            'mode': 'OWN',
#            'init_expr': {
#                'type': 'BinaryOpExpr',
#                'op': 'ADD',
#                'left_expr': {'type': 'ConstantExpr',
#                              'value': 5},
#                'right_expr': {'type': 'ConstantExpr',
#                               'value': 2},
#            },
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'e',
#            'type_spec': {
#                'type': 'TypeSpec',
#                'name': 'String',
#                'params': [],
#            },
#            'mode': 'OWN',
#            'init_expr': {
#                'type': 'ConstantExpr',
#                'value': '',
#            },
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'f',
#            'type_spec': None,
#            'mode': 'OWN',
#            'init_expr': None,
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'g',
#            'mode': 'BORROW',
#            'type_spec': None,
#            'init_expr': None,
#        },
#        {
#            'type': 'VarDeclStmt',
#            'name': 'h',
#            'type_spec': None,
#            'mode': 'OWN',
#            'init_expr': None,
#        },
#    ])
#
#  def test_func(self):
#    self.assert_ast_match('''
#    function foo(a, &b Int, ~c = 0, d = 0 Int) {
#      return a + b + c + d;
#    }
#    ''', 'func', {
#        'type': 'Func',
#        'name': 'foo',
#        'type_params': [],
#        'params': [
#            {
#                'type': 'FuncParam',
#                'name': 'a',
#                'mode': 'COPY',
#                'type_spec': None,
#                'init_expr': None,
#            },
#            {
#                'type': 'FuncParam',
#                'name': 'b',
#                'mode': 'BORROW',
#                'type_spec': {
#                    'type': 'TypeSpec',
#                    'name': 'Int',
#                    'params': [],
#                },
#                'init_expr': None,
#            },
#            {
#                'type': 'FuncParam',
#                'name': 'c',
#                'mode': 'MOVE',
#                'type_spec': None,
#                'init_expr': {
#                    'type': 'ConstantExpr',
#                    'value': 0,
#                },
#            },
#            {
#                'type': 'FuncParam',
#                'name': 'd',
#                'mode': 'COPY',
#                'type_spec': {
#                    'type': 'TypeSpec',
#                    'name': 'Int',
#                    'params': [],
#                },
#                'init_expr': {
#                    'type': 'ConstantExpr',
#                    'value': 0,
#                },
#            },
#        ],
#        'return_type_spec': None,
#        'return_mode': 'COPY',
#        'cc': 'DEFAULT',
#        'stmts': [
#            {
#                'type': 'ReturnStmt',
#                'expr': {
#                    'type': 'BinaryOpExpr',
#                    'op': 'ADD',
#                    'left_expr': {
#                        'type': 'BinaryOpExpr',
#                        'op': 'ADD',
#                        'left_expr': {
#                            'type': 'BinaryOpExpr',
#                            'op': 'ADD',
#                            'left_expr': {'type': 'VarExpr',
#                                          'var': 'a'},
#                            'right_expr': {'type': 'VarExpr',
#                                           'var': 'b'},
#                        },
#                        'right_expr': {'type': 'VarExpr',
#                                       'var': 'c'},
#                    },
#                    'right_expr': {'type': 'VarExpr',
#                                   'var': 'd'},
#                },
#            },
#        ],
#    })
#    self.assert_ast_match('''
#    function Foo<A, B,>() &Array<Int> {}
#    ''', 'func', {
#        'type': 'Func',
#        'name': 'Foo',
#        'type_params': ['A', 'B'],
#        'params': [],
#        'return_type_spec': {
#            'type': 'TypeSpec',
#            'name': 'Array',
#            'params': [{
#                'type': 'TypeSpec',
#                'name': 'Int',
#                'params': [],
#            }],
#        },
#        'return_mode': 'BORROW',
#        'cc': 'DEFAULT',
#        'stmts': [],
#    })
#
#  def test_class(self):
#    self.assert_ast_match('class A {}', 'class', {
#        'type': 'Class',
#        'name': 'A',
#        'type_params': [],
#        'super_classes': [],
#        'members': [],
#    })
#    self.assert_ast_match('class A<> extends B<T, Int>, C {}', 'class', {
#        'type': 'Class',
#        'name': 'A',
#        'type_params': [],
#        'super_classes': [
#            {
#                'type': 'TypeSpec',
#                'name': 'B',
#                'params': [
#                    {
#                        'type': 'TypeSpec',
#                        'name': 'T',
#                        'params': [],
#                    },
#                    {
#                        'type': 'TypeSpec',
#                        'name': 'Int',
#                        'params': [],
#                    },
#                ],
#            },
#            {
#                'type': 'TypeSpec',
#                'name': 'C',
#                'params': [],
#            },
#        ],
#        'members': [],
#    })
#    self.assert_ast_match('''
#    class MyClass<T> extends Array<T> {
#      var a;
#      function f() Int { return 42; }
#      var &b = &foo, c Int;
#    }''', 'class', {
#        'type': 'Class',
#        'name': 'MyClass',
#        'type_params': ['T'],
#        'super_classes': [{
#            'type': 'TypeSpec',
#            'name': 'Array',
#            'params': [{
#                'type': 'TypeSpec',
#                'name': 'T',
#                'params': [],
#            }],
#        }],
#        'members': [
#            {
#                'type': 'VarDeclStmt',
#                'name': 'a',
#                'type_spec': None,
#                'mode': 'OWN',
#                'init_expr': None,
#            },
#            {
#                'type': 'Func',
#                'name': 'f',
#                'type_params': [],
#                'params': [],
#                'return_type_spec': {
#                    'type': 'TypeSpec',
#                    'name': 'Int',
#                    'params': [],
#                },
#                'return_mode': 'COPY',
#                'cc': 'DEFAULT',
#                'stmts': [{
#                    'type': 'ReturnStmt',
#                    'expr': {
#                        'type': 'ConstantExpr',
#                        'value': 42,
#                    },
#                }],
#            },
#            {
#                'type': 'VarDeclStmt',
#                'name': 'b',
#                'type_spec': {
#                    'type': 'TypeSpec',
#                    'name': 'Int',
#                    'params': [],
#                },
#                'mode': 'BORROW',
#                'init_expr': {
#                    'type': 'UnaryOpExpr',
#                    'op': 'BORROW',
#                    'expr': {
#                        'type': 'VarExpr',
#                        'var': 'foo',
#                    },
#                },
#            },
#            {
#                'type': 'VarDeclStmt',
#                'name': 'c',
#                'type_spec': {
#                    'type': 'TypeSpec',
#                    'name': 'Int',
#                    'params': [],
#                },
#                'mode': 'OWN',
#                'init_expr': None,
#            },
#        ],
#    })
