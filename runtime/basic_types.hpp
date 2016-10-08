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

#include <cstdint>

struct QClassDescriptor;

// Base class for Quo objects.
struct QObject {
  QClassDescriptor* descriptor;
};

// A 32-bit integer.
struct QInt32 : public QObject {
  int32_t value;
};
typedef QInt32 QInt;

// A string.
struct QString : public QObject {
  // The size (in bytes) of the string.
  int32_t size;
  char* value;
};

#endif  // BASIC_TYPES_HPP_

