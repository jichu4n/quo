/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2017 Chuan Ji <ji@chu4n.com>                               *
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

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <cstdarg>
#include <string>

namespace google {

namespace protobuf {

extern ::std::string StringPrintf(const char* format, ...);
extern const ::std::string& SStringPrintf(
    ::std::string* dst, const char* format, ...);
extern void StringAppendV(::std::string* dst, const char* format, va_list ap);

}  // namespace protobuf

namespace glog_internal_namespace_ {

extern void DumpStackTraceToString(::std::string* stacktrace);

}  // namespace glog_internal_namespace_

}  // namespace google

namespace quo {

using ::google::glog_internal_namespace_::DumpStackTraceToString;
using ::google::protobuf::SStringPrintf;
using ::google::protobuf::StringAppendV;
using ::google::protobuf::StringPrintf;

}  // namespace quo

#endif  // UTILS_HPP_
