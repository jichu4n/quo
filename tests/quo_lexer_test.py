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
"""Unit test for lexer."""
# pylint: disable=missing-docstring

import unittest
import quo_lexer


class LexerTest(unittest.TestCase):
  def assert_tokens_match(self, input_str, expected_tokens):
    lexer = quo_lexer.create_lexer()
    lexer.input(input_str)
    actual_tokens = []
    while True:
      token = lexer.token()
      if token is None:
        break
      actual_tokens.append(token)
    for i in range(max([len(actual_tokens), len(expected_tokens)])):
      if i >= len(actual_tokens):
        self.fail('Missing token %d: %s' % (i, expected_tokens[i]))
      if i >= len(expected_tokens):
        self.fail('Extra token %d: %s' % (i, actual_tokens[i]))
      expected_token_type = (
          expected_tokens[i][0] if isinstance(expected_tokens[i], tuple) else
          expected_tokens[i])
      if actual_tokens[i].type != expected_token_type:
        self.fail(
            'Mismatched token %d: expected type %s, got type %s' % (
                i, expected_token_type, actual_tokens[i].type))
      if (isinstance(expected_tokens[i], tuple) and
          actual_tokens[i].value != expected_tokens[i][1]):
        self.fail(
            'Mismatched token %d: expected value %s, got value %s' % (
                i, expected_tokens[i][1], actual_tokens[i].value))

  def test_comments(self):
    self.assert_tokens_match("""
    hello // foo
    if // baz""", [
        ('IDENTIFIER', 'hello'),
        'IF',
    ])

  def test_operators(self):
    self.assert_tokens_match("""
    < > >= <= = == != ( ) [ ] { }and or not + - * / += -= *= /= . : ; ,
    """, [
        'LT',
        'GT',
        'GE',
        'LE',
        'ASSIGN',
        'EQ',
        'NE',
        'L_PAREN',
        'R_PAREN',
        'L_BRACKET',
        'R_BRACKET',
        'L_BRACE',
        'R_BRACE',
        'AND',
        'OR',
        'NOT',
        'ADD',
        'SUB',
        'MUL',
        'DIV',
        'ADD_ASSIGN',
        'SUB_ASSIGN',
        'MUL_ASSIGN',
        'DIV_ASSIGN',
        'DOT',
        'COLON',
        'SEMICOLON',
        'COMMA',
    ])

  def test_unknown_char(self):
    with self.assertRaises(quo_lexer.UnknownCharacterError):
      self.assert_tokens_match('你好', [])

  def test_sample_program_1(self):
    self.assert_tokens_match("""
    class Car {
      var make String;  // Make
      var model String;  // Model
      var year Int;  // Year of manufacture

      function FullSpec(inclYear=false Bool) String {
        var spec String;
        if inclYear {
          spec += this.year.ToString();
        }
        spec += ' ' + make + ' ' + model;
        return spec;
      }
    }""", [
        'CLASS',
        ('IDENTIFIER', 'Car'),
        'L_BRACE',
        'VAR',
        ('IDENTIFIER', 'make'),
        ('IDENTIFIER', 'String'),
        'SEMICOLON',
        'VAR',
        ('IDENTIFIER', 'model'),
        ('IDENTIFIER', 'String'),
        'SEMICOLON',
        'VAR',
        ('IDENTIFIER', 'year'),
        ('IDENTIFIER', 'Int'),
        'SEMICOLON',
        'FUNCTION',
        ('IDENTIFIER', 'FullSpec'),
        'L_PAREN',
        ('IDENTIFIER', 'inclYear'),
        'ASSIGN',
        ('BOOLEAN_CONSTANT', False),
        ('IDENTIFIER', 'Bool'),
        'R_PAREN',
        ('IDENTIFIER', 'String'),
        'L_BRACE',
        'VAR',
        ('IDENTIFIER', 'spec'),
        ('IDENTIFIER', 'String'),
        'SEMICOLON',
        'IF',
        ('IDENTIFIER', 'inclYear'),
        'L_BRACE',
        ('IDENTIFIER', 'spec'),
        'ADD_ASSIGN',
        'THIS',
        'DOT',
        ('IDENTIFIER', 'year'),
        'DOT',
        ('IDENTIFIER', 'ToString'),
        'L_PAREN',
        'R_PAREN',
        'SEMICOLON',
        'R_BRACE',
        ('IDENTIFIER', 'spec'),
        'ADD_ASSIGN',
        ('STRING_CONSTANT', ' '),
        'ADD',
        ('IDENTIFIER', 'make'),
        'ADD',
        ('STRING_CONSTANT', ' '),
        'ADD',
        ('IDENTIFIER', 'model'),
        'SEMICOLON',
        'RETURN',
        ('IDENTIFIER', 'spec'),
        'SEMICOLON',
        'R_BRACE',
        'R_BRACE',
    ])
