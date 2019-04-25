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

#ifndef EXCEPTIONS_HPP_
#define EXCEPTIONS_HPP_

#include <exception>
#include <memory>
#include <string>

namespace quo {

struct Exception : public ::std::exception {
  int line;
  ::std::string message;
  ::std::string stacktrace;

  Exception(const Exception& e);
  Exception(int line, const char* format, ...);
  Exception(const char* format, ...);

  // Returns a copy of this exception with a default line number. If this
  // exception already has a line number, the default line number is ignored and
  // *this is returned.
  Exception withDefault(int defaultLine) const;

  virtual const char* what() const throw();

 private:
  ::std::unique_ptr<::std::string> what_;
};

}  // namespace quo

#endif  // EXCEPTIONS_HPP_
