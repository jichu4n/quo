# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2016-2017 Chuan Ji <ji@chu4n.com>                          #
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
"""Compiles and runs test Quo programs."""
# pylint: disable=missing-docstring

import logging
import os
import quoc
import subprocess
import unittest


class CompileTest(unittest.TestCase):

  DATA_DIR = os.path.join(os.path.dirname(__file__), 'testdata')

  def compile_and_check_exit_code(self, input_file, exit_code):
    logging.info('[%s] Compiling...', input_file)
    input_file = os.path.join(self.DATA_DIR, input_file)
    output_file = os.path.splitext(input_file)[0]
    r = quoc.compile_file(input_file, output_file=output_file)
    self.assertEqual(r, 0)
    logging.info('[%s] Running...', output_file)
    input_file = os.path.join(self.DATA_DIR, input_file)
    p = subprocess.run([output_file])
    self.assertEqual(p.returncode, exit_code)
    logging.info('[%s] OK', input_file)

  FIB_TEMPLATE = '''
fn Fib(x Int) Int {
   if x == 0 {
     return 0;
   } else if x <= 2 {
     return 1;
   } else {
     return Fib(x - 1) + Fib(x - 2);
   }
}

fn Main() Int {
  return Fib(%d);
}
'''
  FIB = [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233]

  def test_fib(self):
    input_file = 'fib.quo'
    #for i in range(len(self.FIB)):
    for i in range(1):
      logging.info('Testing Fib(%d)', i)
      with open(os.path.join(self.DATA_DIR, input_file), 'w') as file_obj:
        file_obj.write(self.FIB_TEMPLATE % i)
      self.compile_and_check_exit_code(input_file, self.FIB[i])

  COMPILE_TESTS = {
      'assign.quo': 94,
      'string.quo': 12,
      'ref_mode.quo': 42,
  }
  def test_compile(self):
    for input_file, exit_code in self.COMPILE_TESTS.items():
      self.compile_and_check_exit_code(input_file, exit_code)

