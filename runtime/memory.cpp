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
#include <functional>
#include <iterator>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "runtime/basic_types.hpp"
#include "runtime/descriptor.hpp"

DEFINE_bool(
    debug_mm, false,
    "Log memory management debug information");

using ::std::endl;
using ::std::function;
using ::std::getline;
using ::std::istringstream;
using ::std::istream_iterator;
using ::std::ostringstream;
using ::std::string;
using ::std::vector;
namespace google {
namespace glog_internal_namespace_ {
extern void DumpStackTraceToString(string* stacktrace);
}
}
using ::google::glog_internal_namespace_::DumpStackTraceToString;

namespace {

string GetStackTraceString() {
  string raw_st;
  DumpStackTraceToString(&raw_st);
  istringstream raw_st_stream(raw_st);

  // Drop first two lines, which correspond to this function and the memory
  // management function.
  string line;
  getline(raw_st_stream, line);
  getline(raw_st_stream, line);

  ostringstream result_stream;
  while (raw_st_stream) {
    getline(raw_st_stream, line);
    // Split line by whitespaces.
    istringstream line_stream(line);
    const string fn_name(vector<string>(
        istream_iterator<string>(line_stream),
        istream_iterator<string>()).back());
    result_stream << fn_name;
    if (fn_name == "main") {
      break;
    } else {
      result_stream << " ";
    }
  }
  return result_stream.str();
}

}

void* __quo_alloc(int32_t size) {
  void* p = malloc(size);
  if (FLAGS_debug_mm) {
    LOG(INFO) << p << " ALLOC(" << size << ") ["
        << GetStackTraceString() << "]";
  }
  return p;
}

void __quo_free(QObject* p) {
  if (FLAGS_debug_mm) {
    LOG(INFO)
        << p << " FREE " << p->descriptor->name
        << " [" << GetStackTraceString() << "]";
  }
  QClassDescriptor *dp = p->descriptor;
  if (dp->destroy) {
    dp->destroy(p);
  }
  free(p);
}

void* __quo_copy(void* dest, const void* src, int32_t size) {
  return memcpy(dest, src, size);
}

