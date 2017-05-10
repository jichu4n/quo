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

add_library(
  runtime
  STATIC
  builtin_types.cpp
  memory.cpp
  main.cpp)
add_custom_command(
  TARGET runtime
  POST_BUILD
  COMMAND
  ${CMAKE_COMMAND} -E remove_directory deps_objs
  COMMAND
  ${CMAKE_COMMAND} -E make_directory deps_objs)
add_custom_command(
  TARGET runtime
  POST_BUILD
  COMMAND
  ${CMAKE_AR}
    x
    ${gflags_LIB}
  COMMAND
  ${CMAKE_AR}
    x
    ${glog_LIB}
  COMMAND
  ${CMAKE_AR}
    qs
    $<TARGET_FILE:runtime>
    *
  WORKING_DIRECTORY
  deps_objs)
