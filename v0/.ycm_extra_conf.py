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


import os
import subprocess
import distutils.spawn

PWD = os.path.dirname(os.path.abspath(__file__))
EXTRA_FLAGS = [
    '-I%s' % os.path.join(PWD),
    '-I%s' % os.path.join(PWD, 'deps', 'include'),
    '-I%s' % os.path.join(PWD, 'build'),
]


def GetLLVMConfig():
  LLVM_CONFIG_BINARY = [
      'llvm-config',
      'llvm-config-3.9',
      'llvm-config-3.8',
      'llvm-config-3.7',
  ]
  if 'LLVM_CONFIG' in os.environ:
    return os.environ['LLVM_CONFIG']
  for binary in LLVM_CONFIG_BINARY:
    binary_path = distutils.spawn.find_executable(binary)
    if binary_path is not None:
      return binary_path
  raise IOError(
      'Cannot find llvm-config binary among the following candidates:\n'
      '%s\n'
      'Please install LLVM or provide a LLVM_CONFIG environment variable.' % (
          '\n'.join(LLVM_CONFIG_BINARY)))


LLVM_CONFIG = GetLLVMConfig()
LLVM_CXXFLAGS = subprocess.check_output([LLVM_CONFIG, '--cxxflags']).split()
try:
  LLVM_CXXFLAGS.remove('-fno-exceptions')
except ValueError:
  pass
FLAGS = LLVM_CXXFLAGS + EXTRA_FLAGS


def FlagsForFile( filename, **kwargs ):
  return {
    'flags': FLAGS,
    'do_cache': True
  }

