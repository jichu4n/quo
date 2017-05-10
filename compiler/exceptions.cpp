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

#include "compiler/exceptions.hpp"
#include <cstdarg>
#include <sstream>
#include "compiler/utils.hpp"

namespace quo {

using ::std::endl;
using ::std::ostringstream;
using ::std::string;

Exception::Exception(const Exception& e)
    : line(e.line),
      message(e.message),
      what_(new string()) {
}

Exception::Exception(int line, const char* format, ...)
    : line(line),
      what_(new string()) {
  va_list args;
  va_start(args, format);
  StringAppendV(&message, format, args);
  va_end(args);
}

Exception::Exception(const char* format, ...)
    : line(-1),
      what_(new string()) {
  va_list args;
  va_start(args, format);
  StringAppendV(&message, format, args);
  va_end(args);
}

Exception Exception::withDefault(int defaultLine) const {
  if (line > 0) {
    return *this;
  }
  Exception e(*this);
  e.line = defaultLine;
  return e;
}

const char* Exception::what() const throw() {
  ostringstream out;
  if (line > 0) {
    out << ":" << line << "] ";
  }
  out << message << endl;
  what_->assign(out.str());
  return what_->c_str();
}

}  // namespace quo
