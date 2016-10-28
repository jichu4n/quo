/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2016 Chuan Ji <jichu4n@gmail.com>                          *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *   http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "runtime/memory.hpp"
#include <cstdlib>
#include <cstring>
#include <gflags/gflags.h>
#include <glog/logging.h>

DEFINE_bool(
    debug_mm, false,
    "Log memory management debug information");

extern "C" {

void* __quo_alloc(int32_t size) {
  void* p = malloc(size);
  LOG_IF(INFO, FLAGS_debug_mm) << "[MM] " << p << " = ALLOC(" << size << " )";
  return p;
}

void __quo_free(void* p) {
  LOG_IF(INFO, FLAGS_debug_mm) << "[MM] FREE(" << p << ")";
  free(p);
}

void* __quo_copy(void* dest, const void* src, int32_t size) {
  return memcpy(dest, src, size);
}

}

