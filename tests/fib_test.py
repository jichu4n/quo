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
"""Compiles and runs a simple Fibonacci sequence algorithm.

Tests compilation of simple functions, integer arithmetic, and branching.
"""
# pylint: disable=missing-docstring

import logging
import os
import quoc
import subprocess
import unittest


class FibTest(unittest.TestCase):

  TEMPLATE = '''
fn Fib(x Int) Int {
   if x <= 2 {
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
    input_file = os.path.join(
        os.path.dirname(__file__), 'testdata', 'fib.quo')
    output_file = os.path.join(
        os.path.dirname(__file__), 'testdata', 'fib')
    for i in range(1, len(self.FIB)):
      logging.info('Testing Fib(%d)', i)
      with open(input_file, 'w') as file_obj:
        file_obj.write(self.TEMPLATE % i)
      r = quoc.compile_file(input_file, output_file=output_file)
      self.assertEqual(r, 0)
      p = subprocess.run([output_file])
      self.assertEqual(p.returncode, self.FIB[i])

