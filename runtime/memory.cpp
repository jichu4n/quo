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

QObject* __quo_alloc(const QClassDescriptor* dp, int32_t size) {
  QObject* p = static_cast<QObject*>(malloc(size));
  if (FLAGS_debug_mm) {
    LOG(INFO) << p << " " << dp->name << " ALLOC(" << size << ") ["
        << GetStackTraceString() << "]";
  }
  p->descriptor = dp;
  if (dp->init) {
    dp->init(p);
  }
  return p;
}

void __quo_free(QObject* p) {
  const QClassDescriptor *dp = p->descriptor;
  if (FLAGS_debug_mm) {
    LOG(INFO) << p << " " << dp->name << " FREE " << " ["
        << GetStackTraceString() << "]";
  }
  if (dp->destroy) {
    dp->destroy(p);
  }
  free(p);
}

QObject* __quo_copy(QObject* dest, const QObject* src, int32_t size) {
  const QClassDescriptor* dp = src->descriptor;
  if (dp->copy) {
    dp->copy(dest, src);
  } else  {
    memcpy(dest, src, size);
  }
  return dest;
}

