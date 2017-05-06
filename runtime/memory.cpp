/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2016-2017 Chuan Ji <ji@chu4n.com>                          *
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
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "ast/ast.pb.h"
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
using ::quo::STRONG_REF;
using ::quo::WEAK_REF;

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
  for (;;) {
    getline(raw_st_stream, line);
    // Split line by whitespaces.
    istringstream line_stream(line);
    vector<string> columns {
        istream_iterator<string>(line_stream),
        istream_iterator<string>()
    };
    if (columns.empty()) {
      break;
    }
    const string fn_name(columns.back());
    result_stream << fn_name << " ";
  }
  string result = result_stream.str();
  if (!result.empty() && result.back() == ' ') {
    result.resize(result.length() - 1);
  }
  return result;
}

}  // namespace

QObject* __quo_alloc(const QClassDescriptor* dp, int32_t size) {
  QObject* p = static_cast<QObject*>(malloc(size));
  if (FLAGS_debug_mm) {
    LOG(INFO) << p << " " << dp->name << " ALLOC(" << size << ") ["
        << GetStackTraceString() << "]";
  }
  p->descriptor = dp;
  p->refs = 1;
  if (dp->init) {
    dp->init(p);
  }
  return p;
}

void __quo_inc_refs(QObject* p) {
  ++(p->refs);
  if (FLAGS_debug_mm) {
    const QClassDescriptor *dp = p->descriptor;
    LOG(INFO) << p << " " << dp->name << " ++REF=" << p->refs << " ["
        << GetStackTraceString() << "]";
  }
}

void __quo_dec_refs(QObject* p) {
  const QClassDescriptor *dp = p->descriptor;
  --(p->refs);
  if (p->refs < 0) {
    LOG(FATAL) << p << " " << dp->name << " INVALID REFS: " << p->refs << " ["
      << GetStackTraceString() << "]";
  } else if (p->refs == 0) {
    if (FLAGS_debug_mm) {
      LOG(INFO) << p << " " << dp->name << " FREE " << " ["
          << GetStackTraceString() << "]";
    }
    if (dp->destroy) {
      dp->destroy(p);
    }
    free(p);
  } else {
    if (FLAGS_debug_mm) {
      LOG(INFO) << p << " " << dp->name << " --REF=" << p->refs << " ["
          << GetStackTraceString() << "]";
    }
  }
}

void __quo_assign(QObject** dest, QObject* src, int8_t ref_mode) {
  CHECK(ref_mode == STRONG_REF || ref_mode == WEAK_REF)
      << "Invalid ref mode: " << ref_mode << " ["
      << GetStackTraceString() << "]";
  if (ref_mode == STRONG_REF) {
    if (src != nullptr) {
      __quo_inc_refs(src);
    }
    if (*dest != nullptr) {
      __quo_dec_refs(*dest);
    }
  }
  *dest = src;
}

