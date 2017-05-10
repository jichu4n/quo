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
#include "runtime/builtin_types.hpp"
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
  CHECK_NOTNULL(dp);
  QObject* p = static_cast<QObject*>(malloc(size));
  LOG_IF(INFO, FLAGS_debug_mm) << p << " " << dp->name << " ALLOC(" << size
      << ") [" << GetStackTraceString() << "]";
  p->descriptor = dp;
  p->refs = 1;
  if (dp->init) {
    dp->init(p);
  }
  return p;
}

void __quo_inc_refs(QObject* p) {
  ++(CHECK_NOTNULL(p)->refs);
  const QClassDescriptor *dp = CHECK_NOTNULL(p->descriptor);
  LOG_IF(INFO, FLAGS_debug_mm) << p << " " << dp->name << " ++REF=" << p->refs
      << " [" << GetStackTraceString() << "]";
}

void __quo_dec_refs(QObject* p) {
  const QClassDescriptor *dp = CHECK_NOTNULL(CHECK_NOTNULL(p)->descriptor);
  --(p->refs);
  LOG_IF(FATAL, p->refs < 0) << p << " INVALID REFS: " << p->refs << " ["
    << GetStackTraceString() << "]";
  if (p->refs == 0) {
    LOG_IF(INFO, FLAGS_debug_mm) << p << " " << dp->name << " FREE " << " ["
        << GetStackTraceString() << "]";
    if (dp->destroy) {
      dp->destroy(p);
    }
    free(p);
  } else {
    LOG_IF(INFO, FLAGS_debug_mm) << p << " " << dp->name << " --REF=" << p->refs
        << " [" << GetStackTraceString() << "]";
  }
}

void __quo_assign(QObject** dest, QObject* src, int8_t ref_mode) {
  CHECK(ref_mode == STRONG_REF || ref_mode == WEAK_REF)
      << "Invalid ref mode: " << ref_mode << " ["
      << GetStackTraceString() << "]";
  if (*dest == src) {
    return;
  }
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

QObject* __quo_get_field(
    QObject* obj, QClassDescriptor* view_class, int32_t index) {
  CHECK_GE(index, 0);
  const QClassDescriptor* desc = CHECK_NOTNULL(CHECK_NOTNULL(obj)->descriptor);
  CHECK_GT(desc->views.size, 0)
      << "Attempting to access field " << index
      << " in class " << view_class->name << "  (cast from "
      << desc->name << "), but class " << desc->name
      << " has no views";
  const QClassView& self_view = desc->views[0];
  CHECK_EQ(self_view.view_class, desc)
      << "The first view in class " << desc->name
      << " is for different class " << self_view.view_class->name;
  for (int i = 0; i < desc->views.size; ++i) {
    const QClassView& view = desc->views[i];
    if (view.view_class == view_class) {
      CHECK_GT(view.fields.size, index)
          << "Attempting to access field " << index
          << " of class " << view_class->name << " (cast from "
          << desc->name << "), which has " << view.fields.size << " members";
      int32_t index_in_obj = view.fields[index].index;
      CHECK_GE(index_in_obj, 0)
          << "Field " << view.fields[index].name << " in class "
          << view_class->name << " (cast from "
          << desc->name << ") has negative index in object "
          << index_in_obj;
      CHECK_LT(index_in_obj, self_view.fields.size)
          << "Field " << view.fields[index].name << " in class "
          << view_class->name << " (cast from "
          << desc->name << ") has invalid index in object "
          << index_in_obj << ", which only has " << self_view.fields.size
          << " fields";
      return reinterpret_cast<QCustomObject*>(obj)->fields[index_in_obj];
    }
  }
  LOG(FATAL) << "Failed to cast object " << " at " << obj
      << " of class " << desc->name << " to " << view_class->name;
}

