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
"""Unit test for C++ translator."""
# pylint: disable=missing-docstring

from ctypes import byref, c_int
import unittest
import cpp_translator


class CppTranslatorTest(unittest.TestCase):

  def compile_and_run(self, quo_str, func, args):
    """Compiles a Quo module, runs a function within and returns the result."""
    return getattr(cpp_translator.compile_and_load(quo_str), func)(*args)

  def assert_result_equal(self, quo_str, func, args, expected_result):
    self.assertEqual(
        self.compile_and_run(quo_str, func, args),
        expected_result)

  def test_sanity(self):
    self.assert_result_equal(
        'extern fn Sum(a Int, b Int) Int { return a + b; }',
        'Sum', [byref(c_int(1)), byref(c_int(100))],
        101)

  def test_arg_passing(self):
    self.assert_result_equal('''
        fn sumCopy(a Int, b Int) Int { return a + b; }
        extern fn TestCopy() Int {
          return sumCopy(1, 100);
        }
    ''', 'TestCopy', [], 101)
    self.assert_result_equal('''
        fn sumBorrow(&a Int, &b Int) Int { return a + b; }
        extern fn TestBorrow() Int {
          var a = 1, b = 100 Int;
          var r1 = sumBorrow(&a, &b) Int;
          var r2 = sumBorrow(&a, &b) Int;
          return r1 + r2;
        }
    ''', 'TestBorrow', [], 202)
    self.assert_result_equal('''
        fn sumMove(~a Int, ~b Int) Int { return a + b; }
        extern fn TestMove() Int {
          var a = 1, b = 100 Int;
          return sumMove(~a, ~b);
        }
    ''', 'TestMove', [], 101)

  def test_class_inheritance(self):
    quo_str = '''
        class Base1 {
          fn F() Int {
            return 101;
          }
        }
        class Base2 {
          fn G() Int {
            return 102;
          }
        }
        class Derived extends Base1, Base2 {
          fn F() Int {
            return 103;
          }
        }
        extern fn F() Int {
          var a Derived;
          return a.F();
        }
        extern fn G() Int {
          var a Derived;
          return a.G();
        }
    '''
    self.assert_result_equal(quo_str, 'F', [], 103)
    self.assert_result_equal(quo_str, 'G', [], 102)

  def test_class_members(self):
    quo_str = '''
    class A {
      var X = 42 Int;
      fn F() Int {
        return X;
      }
    }
    extern fn F() Int {
      var a A;
      return a.F();
    }
    extern fn X() Int {
      var a A;
      return a.X;
    }
    '''
    self.assert_result_equal(quo_str, 'F', [], 42);
    self.assert_result_equal(quo_str, 'X', [], 42);

  def test_array(self):
    self.assert_result_equal('''
        extern fn Test() Int {
          var a Array<Int>;
          a.Append(4);
          a.Append(10);
          a.Append(20);
          a[0] = 9;
          return a[0] * a[2] + a.Length();
        }
    ''', 'Test', [], 183)
