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

#ifndef BASIC_TYPES_HPP_
#define BASIC_TYPES_HPP_

#include <string>
#include <cstdint>
#include "runtime/descriptor.hpp"

// Base class for Quo objects.
struct QObject {
  const QClassDescriptor* descriptor;
};
extern const QClassDescriptor __quo_ObjectDescriptor;

// A 32-bit integer.
struct QInt32 : public QObject {
  int32_t value;
};
extern const QClassDescriptor __quo_Int32Descriptor;

// A boolean.
struct QBool : public QObject {
  int8_t value;
};
extern const QClassDescriptor __quo_BoolDescriptor;

// A string.
struct QString : public QObject {
  ::std::string* value;
};
extern const QClassDescriptor __quo_StringDescriptor;
extern "C" QString* __quo_alloc_string(const char* value, int32_t length);
extern "C" QString* __quo_string_concat(QString* left, QString* right);

// Basic printing.
extern "C" QInt32* __quo_print(const QString* s);

#endif  // BASIC_TYPES_HPP_

