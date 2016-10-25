#!/usr/bin/env python3
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2016 Chuan Ji <jichu4n@gmail.com>                          #
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
"""Quo compiler.

Example usage:

    quoc.py foo.quo && ./a.out
"""

import argparse
import os
import subprocess
import shutil
import sys
from parser import lexer, parser


if __name__ == '__main__':
  arg_parser = argparse.ArgumentParser()
  arg_parser.add_argument('-o', help='Output file', metavar='output_file')
  arg_parser.add_argument('input_file', help='Input file')
  args = vars(arg_parser.parse_args())

  with open(args['input_file'], 'r') as file_obj:
    lines = file_obj.readlines()
  input_file_root = os.path.splitext(args['input_file'])[0]

  # 1. Parse source code into AST.
  parse = parser.create_parser()
  lex = lexer.create_lexer()
  ast = parse.parse('\n'.join(lines), lexer=lex)
  ast_str = str(ast)

  # 2. Convert AST into LLVM IR.
  build_dir = os.path.join(
      os.path.dirname(__file__),
      'build')
  quoc = os.path.join(build_dir, 'compiler', 'quoc')
  if not os.path.exists(quoc):
    print('Cannot find quoc at %s, exiting' % quoc, file=sys.stderr)
    sys.exit(1)
  bc_file = input_file_root + '.bc'
  p1 = subprocess.run(
      [quoc],
      input=ast_str,
      universal_newlines=True,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  if p1.returncode != 0:
    print(p1.stdout, file=sys.stdout)
    print(p1.stderr, file=sys.stderr)
    sys.exit(p1.returncode)
  with open(bc_file, 'w') as file_obj:
    file_obj.write(p1.stdout)

  # 3. Compile.
  llc = os.path.join(
      os.path.dirname(__file__),
      'deps',
      'bin',
      'llc')
  if not os.path.exists(llc):
    print('Cannot find llc at %s, exiting' % llc, file=sys.stderr)
    sys.exit(1)
  input_file_root = os.path.splitext(args['input_file'])[0]
  asm_file = input_file_root + '.s'
  p2 = subprocess.run(
      [llc, '-o', asm_file],
      input=p1.stdout,
      universal_newlines=True)
  if p2.returncode != 0:
    sys.exit(p2.returncode)

  # 4. Link.
  if 'CXX' in os.environ:
    cxx = shutil.which(os.environ['CXX'])
  else:
    for cxx_candidate in ['c++', 'g++', 'clang++']:
      cxx = shutil.which(cxx_candidate)
      if cxx is not None:
        break
  if cxx is None:
    print('Cannot find C++ compiler, exiting', file=sys.stderr)
    sys.exit(1)
  runtime_dir = os.path.join(
      build_dir,
      'runtime')
  cxx_args = [
      cxx,
      '-o', args['o'] if args['o'] else input_file_root,
      asm_file,
      '-L', runtime_dir,
      '-lruntime',
  ]
  if sys.platform != 'darwin':
    cxx_args.append('-static')
  p3 = subprocess.run(cxx_args)
  sys.exit(p3.returncode)

