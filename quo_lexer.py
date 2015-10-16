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
"""Quo lexical analyzer."""

import ply.lex


class UnknownCharacterError(Exception):
  """Indicates an unknown character was encountered during lexical analysis."""

  def __init__(self, line_num, char):
    super().__init__()
    self.line_num = line_num
    self.char = char

  def __str__(self):
    return '[Line %d] Illegal character \'%s\' (0x%x)' % (
        self.line_num, self.char, ord(self.char))


class QuoLexer(object):
  """Quo lexer, implemented via PLY."""

  # pylint: disable=invalid-name,no-self-use
  tokens = (
      # Program structure.
      'CLASS',
      'VAR',
      'FUNCTION',
      # Control structures.
      'IF',
      'ELSE',
      'FOR',
      'WHILE',
      'CONTINUE',
      'BREAK',
      'RETURN',
      # Other classes of tokens.
      'IDENTIFIER',
      'INTEGER_CONSTANT',
      'STRING_CONSTANT',
      'BOOLEAN_CONSTANT',
      'THIS',
      # Operators.
      'AND',
      'OR',
      'NOT',
      'GE',
      'GT',
      'LE',
      'LT',
      'NE',
      'ASSIGN',
      'EQ',
      'ADD_EQ',
      'SUB_EQ',
      'MUL_EQ',
      'DIV_EQ',
      'ADD',
      'SUB',
      'MUL',
      'DIV',
      'MOD',
      'L_PAREN',
      'R_PAREN',
      'L_BRACKET',
      'R_BRACKET',
      'L_BRACE',
      'R_BRACE',
      'DOT',
      'COLON',
      'COMMA',
      'SEMICOLON', )

  _reserved_words = {
      'class': ('CLASS', None),
      'var': ('VAR', None),
      'function': ('FUNCTION', None),
      'if': ('IF', None),
      'else': ('ELSE', None),
      'for': ('FOR', None),
      'while': ('WHILE', None),
      'continue': ('CONTINUE', None),
      'break': ('BREAK', None),
      'return': ('RETURN', None),
      'and': ('AND', 'AND'),
      'or': ('OR', 'OR',),
      'not': ('NOT', 'NOT'),
      'true': ('BOOLEAN_CONSTANT', True),
      'false': ('BOOLEAN_CONSTANT', False),
      'this': ('THIS', None),
  }

  t_ignore_COMMENT = r'//[^\n]*'

  def t_IDENTIFIER(self, t):
    r'[a-zA-Z_][a-zA-Z0-9_]*'
    if t.value in self._reserved_words:
      t.type, t.value = self._reserved_words[t.value]
    return t

  t_INTEGER_CONSTANT = r'[0-9]+'

  def t_STRING_CONSTANT(self, t):
    r"'([^']|\\')*'"
    t.value = t.value[1:len(t.value) - 1]
    return t

  def t_GE(self, t):
    r'>='
    t.value = t.type
    return t

  def t_GT(self, t):
    r'>'
    t.value = t.type
    return t

  def t_LE(self, t):
    r'<='
    t.value = t.type
    return t

  def t_LT(self, t):
    r'<'
    t.value = t.type
    return t

  def t_NE(self, t):
    r'!='
    t.value = t.type
    return t

  def t_EQ(self, t):
    r'=='
    t.value = t.type
    return t

  t_ASSIGN = r'='

  def t_ADD_EQ(self, t):
    r'\+='
    t.value = t.type
    return t

  def t_SUB_EQ(self, t):
    r'-='
    t.value = t.type
    return t

  def t_MUL_EQ(self, t):
    r'\*='
    t.value = t.type
    return t

  def t_DIV_EQ(self, t):
    r'/='
    t.value = t.type
    return t

  def t_ADD(self, t):
    r'\+'
    t.value = t.type
    return t

  def t_SUB(self, t):
    r'-'
    t.value = t.type
    return t

  def t_MUL(self, t):
    r'\*'
    t.value = t.type
    return t

  def t_DIV(self, t):
    r'/(?!/)'
    t.value = t.type
    return t

  def t_MOD(self, t):
    r'%'
    t.value = t.type
    return t

  t_L_PAREN = r'\('
  t_R_PAREN = r'\)'
  t_L_BRACKET = r'\['
  t_R_BRACKET = r'\]'
  t_L_BRACE = r'\{'
  t_R_BRACE = r'\}'
  t_DOT = r'\.'
  t_COLON = r':'
  t_COMMA = r','
  t_SEMICOLON = r';'

  t_ignore_WHITESPACE = r'[ \t]+'

  def t_newline(self, t):
    r'\n+'
    t.lexer.lineno += len(t.value)

  def t_error(self, t):
    """Report error."""
    raise UnknownCharacterError(t.lexer.lineno, t.value[0])


def create_lexer():
  """Creates and returns a new PLY lexer instance."""
  return ply.lex.lex(module=QuoLexer())
