# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2015-2017 Chuan Ji <ji@chu4n.com>                          #
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
from build.ast.ast_pb2 import *
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
            parent_expr=Expr(index=IndexExpr(
                array_expr=Expr(var=VarExpr(name='a')),
                index_expr=Expr(index=IndexExpr(
                    array_expr=Expr(index=IndexExpr(
                        array_expr=Expr(member=MemberExpr(
                            parent_expr=Expr(var=VarExpr(name='b')),
                            member_name='foo')),
                        index_expr=Expr(constant=ConstantExpr(intValue=0)))),
                    index_expr=Expr(
                        constant=ConstantExpr(strValue='hello')))))),
            member_name='bar')))
    self.assert_ast_match(
        '(((a)()(c,)(d()))(e.f()[1],g,h[i].j(),))',
        'expr',
        Expr(call=CallExpr(
            fn_expr=Expr(call=CallExpr(
                fn_expr=Expr(call=CallExpr(
                    fn_expr=Expr(call=CallExpr(
                        fn_expr=Expr(var=VarExpr(name='a')))),
                    arg_exprs=[Expr(var=VarExpr(name='c'))])),
                arg_exprs=[
                    Expr(call=CallExpr(
                        fn_expr=Expr(var=VarExpr(name='d')))),
                ])),
            arg_exprs=[
                Expr(index=IndexExpr(
                    array_expr=Expr(call=CallExpr(
                        fn_expr=Expr(member=MemberExpr(
                            parent_expr=Expr(var=VarExpr(name='e')),
                            member_name='f')))),
                    index_expr=Expr(constant=ConstantExpr(intValue=1)))),
                Expr(var=VarExpr(name='g')),
                Expr(call=CallExpr(
                    fn_expr=Expr(member=MemberExpr(
                        parent_expr=Expr(index=IndexExpr(
                            array_expr=Expr(var=VarExpr(name='h')),
                            index_expr=Expr(var=VarExpr(name='i')))),
                        member_name='j')))),
            ])))

  def test_binary_arith(self):
    self.assert_ast_match(
        'a+b*c-d==e--f/g%-+-h',
        'expr',
        Expr(binary_op=BinaryOpExpr(
            op=BinaryOpExpr.EQ,
            left_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.SUB,
                left_expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.ADD,
                    left_expr=Expr(var=VarExpr(name='a')),
                    right_expr=Expr(binary_op=BinaryOpExpr(
                        op=BinaryOpExpr.MUL,
                        left_expr=Expr(var=VarExpr(name='b')),
                        right_expr=Expr(var=VarExpr(name='c')))))),
                right_expr=Expr(var=VarExpr(name='d')))),
            right_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.SUB,
                left_expr=Expr(var=VarExpr(name='e')),
                right_expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.MOD,
                    left_expr=Expr(binary_op=BinaryOpExpr(
                        op=BinaryOpExpr.DIV,
                        left_expr=Expr(unary_op=UnaryOpExpr(
                            op=UnaryOpExpr.SUB,
                            expr=Expr(var=VarExpr(name='f')))),
                        right_expr=Expr(var=VarExpr(name='g')))),
                    right_expr=Expr(unary_op=UnaryOpExpr(
                        op=UnaryOpExpr.SUB,
                        expr=Expr(unary_op=UnaryOpExpr(
                            op=UnaryOpExpr.ADD,
                            expr=Expr(unary_op=UnaryOpExpr(
                                op=UnaryOpExpr.SUB,
                                expr=Expr(var=VarExpr(name='h'))))))))))))))
        )

  def test_binary_bool(self):
    self.assert_ast_match(
        'a>b or not c<=d and not false or true',
        'expr',
        Expr(binary_op=BinaryOpExpr(
            op=BinaryOpExpr.OR,
            left_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.OR,
                left_expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.GT,
                    left_expr=Expr(var=VarExpr(name='a')),
                    right_expr=Expr(var=VarExpr(name='b')))),
                right_expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.AND,
                    left_expr=Expr(unary_op=UnaryOpExpr(
                        op=UnaryOpExpr.NOT,
                        expr=Expr(binary_op=BinaryOpExpr(
                            op=BinaryOpExpr.LE,
                            left_expr=Expr(var=VarExpr(name='c')),
                            right_expr=Expr(var=VarExpr(name='d')))))),
                    right_expr=Expr(unary_op=UnaryOpExpr(
                        op=UnaryOpExpr.NOT,
                        expr=Expr(constant=ConstantExpr(boolValue=False)))))))),
            right_expr=Expr(constant=ConstantExpr(boolValue=True)))))
 
  def test_assign(self):
    self.assert_ast_match(
        'a += b = c -= d',
        'expr',
        Expr(assign=AssignExpr(
            dest_expr=Expr(var=VarExpr(name='a')),
            value_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.ADD,
                left_expr=Expr(var=VarExpr(name='a')),
                right_expr=Expr(assign=AssignExpr(
                    dest_expr=Expr(var=VarExpr(name='b')),
                    value_expr=Expr(assign=AssignExpr(
                        dest_expr=Expr(var=VarExpr(name='c')),
                        value_expr=Expr(binary_op=BinaryOpExpr(
                            op=BinaryOpExpr.SUB,
                            left_expr=Expr(var=VarExpr(name='c')),
                            right_expr=Expr(var=VarExpr(name='d')))))))))))))

  def test_cond_stmt(self):
    self.assert_ast_match('''
    if a != b { if 3 {} if 4 {}} else if true { hello; } else { bar; }
    ''',
    'stmt',
    Stmt(cond=CondStmt(
        cond_expr=Expr(binary_op=BinaryOpExpr(
            op=BinaryOpExpr.NE,
            left_expr=Expr(var=VarExpr(name='a')),
            right_expr=Expr(var=VarExpr(name='b')))),
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
                Stmt(expr=ExprStmt(expr=Expr(var=VarExpr(name='hello')))),
            ]),
            false_block=Block(stmts=[
                Stmt(expr=ExprStmt(expr=Expr(var=VarExpr(name='bar')))),
            ])))]))))

  def test_type_spec(self):
    self.assert_ast_match(
        'A<B,C<D.E<F>.G,>,H<I>.J<>>',
        'type_spec',
        TypeSpec(
            name='A',
            params=[
                TypeSpec(name='B'),
                TypeSpec(
                    name='C',
                    params=[
                      TypeSpec(
                        name='G',
                        parent=TypeSpec(
                            name='E',
                            params=[TypeSpec(name='F')],
                            parent=TypeSpec(name='D')))
                    ]),
                TypeSpec(
                    name='J',
                    parent=TypeSpec(
                        name='H',
                        params=[TypeSpec(name='I')])),
            ]))

  def test_var_decl_stmts(self):
    self.assert_ast_match('''
    var a Array<Int>;
    var {
      b = 3, c, d = 5 + 2 Int;
      e = '' String;
      f, &g;
    }
    var h;
    var i, j;
    var &k, l Int;
    ''',
    'stmts',
    Block(stmts=[
        Stmt(var_decl=VarDeclStmt(
            name='a',
            type_spec=TypeSpec(
                name='Array',
                params=[TypeSpec(name='Int')]),
            mode=VarDeclStmt.OWN)),
        Stmt(var_decl=VarDeclStmt(
            name='b',
            type_spec=TypeSpec(name='Int'),
            mode=VarDeclStmt.OWN,
            init_expr=Expr(constant=ConstantExpr(intValue=3)))),
        Stmt(var_decl=VarDeclStmt(
            name='c',
            type_spec=TypeSpec(name='Int'),
            mode=VarDeclStmt.OWN)),
        Stmt(var_decl=VarDeclStmt(
            name='d',
            type_spec=TypeSpec(name='Int'),
            mode=VarDeclStmt.OWN,
            init_expr=Expr(binary_op=BinaryOpExpr(
                op=BinaryOpExpr.ADD,
                left_expr=Expr(constant=ConstantExpr(intValue=5)),
                right_expr=Expr(constant=ConstantExpr(intValue=2)))))),
        Stmt(var_decl=VarDeclStmt(
            name='e',
            type_spec=TypeSpec(name='String'),
            mode=VarDeclStmt.OWN,
            init_expr=Expr(constant=ConstantExpr(strValue='')))),
        Stmt(var_decl=VarDeclStmt(
            name='f',
            mode=VarDeclStmt.OWN)),
        Stmt(var_decl=VarDeclStmt(
            name='g',
            mode=VarDeclStmt.BORROW)),
        Stmt(var_decl=VarDeclStmt(
            name='h',
            mode=VarDeclStmt.OWN)),
        Stmt(var_decl=VarDeclStmt(
            name='i',
            mode=VarDeclStmt.OWN)),
        Stmt(var_decl=VarDeclStmt(
            name='j',
            mode=VarDeclStmt.OWN)),
        Stmt(var_decl=VarDeclStmt(
            name='k',
            mode=VarDeclStmt.BORROW,
            type_spec=TypeSpec(name='Int'))),
        Stmt(var_decl=VarDeclStmt(
            name='l',
            mode=VarDeclStmt.OWN,
            type_spec=TypeSpec(name='Int'))),
    ]))

  def test_func(self):
    self.assert_ast_match('''
    function foo(a, &b Int, c = 0, d = 0 Int) {
      return a + b + c + d;
    }
    ''',
    'func',
    FnDef(
        name='foo',
        params=[
            FnParam(name='a', mode=FnParam.OWN),
            FnParam(
                name='b',
                mode=FnParam.BORROW,
                type_spec=TypeSpec(name='Int')),
            FnParam(
                name='c',
                mode=FnParam.OWN,
                init_expr=Expr(constant=ConstantExpr(intValue=0))),
            FnParam(
                name='d',
                mode=FnParam.OWN,
                type_spec=TypeSpec(name='Int'),
                init_expr=Expr(constant=ConstantExpr(intValue=0))),
        ],
        cc=FnDef.DEFAULT,
        block=Block(stmts=[
            Stmt(ret=RetStmt(
                expr=Expr(binary_op=BinaryOpExpr(
                    op=BinaryOpExpr.ADD,
                    left_expr=Expr(binary_op=BinaryOpExpr(
                        op=BinaryOpExpr.ADD,
                        left_expr=Expr(binary_op=BinaryOpExpr(
                            op=BinaryOpExpr.ADD,
                            left_expr=Expr(var=VarExpr(name='a')),
                            right_expr=Expr(var=VarExpr(name='b')))),
                        right_expr=Expr(var=VarExpr(name='c')))),
                    right_expr=Expr(var=VarExpr(name='d')))))),
        ])))
    self.assert_ast_match('''
    function Foo<A, B,>() Array<Int> {}
    ''',
    'func',
    FnDef(
        name='Foo',
        type_params=['A', 'B'],
        return_type_spec=TypeSpec(
            name='Array',
            params=[TypeSpec(name='Int')]),
        cc=FnDef.DEFAULT,
        block=Block()))

  def test_class(self):
    self.assert_ast_match(
        'class A {}',
        'class',
        ClassDef(name='A'))
    self.assert_ast_match(
        'class A<> extends B<T, Int>, C {}',
        'class',
        ClassDef(
            name='A',
            super_classes=[
                TypeSpec(
                    name='B',
                    params=[
                        TypeSpec(name='T'),
                        TypeSpec(name='Int')
                    ]),
                TypeSpec(name='C'),
            ]))
    self.assert_ast_match('''
    class MyClass<T> extends Array<T> {
      var a;
      function f() Int { return 42; }
      var &b = foo, c Int;
    }''',
    'class',
    ClassDef(
        name='MyClass',
        type_params=['T'],
        super_classes=[TypeSpec(
            name='Array',
            params=[TypeSpec(name='T')])],
        members=[
            ClassDef.Member(var_decl=VarDeclStmt(
                name='a',
                mode=VarDeclStmt.OWN)),
            ClassDef.Member(fn_def=FnDef(
                name='f',
                return_type_spec=TypeSpec(name='Int'),
                cc=FnDef.DEFAULT,
                block=Block(stmts=[Stmt(ret=RetStmt(
                    expr=Expr(constant=ConstantExpr(intValue=42))))]))),
            ClassDef.Member(var_decl=VarDeclStmt(
                name='b',
                type_spec=TypeSpec(name='Int'),
                mode=VarDeclStmt.BORROW,
                init_expr=Expr(var=VarExpr(name='foo')))),
            ClassDef.Member(var_decl=VarDeclStmt(
                name='c',
                type_spec=TypeSpec(name='Int'),
                mode=VarDeclStmt.OWN)),
        ]))

