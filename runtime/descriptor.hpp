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

#ifndef DESCRIPTOR_HPP_
#define DESCRIPTOR_HPP_

#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include "ast/ast.pb.h"

struct QClassView;
struct QFieldDescriptor;
struct QObject;

template <typename T>
struct QConstArray {
  int32_t size;
  const T* array_ptr;

  const T& operator[] (const int32_t index) const {
    return array_ptr[index];
  }
};

// Descriptor for a class.
struct QClassDescriptor {
  // Name of this class.
  const char* name;
  // Pointer to null-terminated array of QClassViews. Each QClassView defines
  // how to access an object of this class as a superclass (or as itself).
  const QConstArray<QClassView> views;
};

// Defines how to access an object as a superclass (or as itself).
struct QClassView {
  // The superclass this view is defined for, or the target class itself.
  QClassDescriptor* view_class;
  // Array of field descriptors.
  const QConstArray<QFieldDescriptor> fields;
};

// Descriptor for a field (instance variable).
struct QFieldDescriptor {
  // Index of this field within the object.
  int32_t index;
  // Name of this field.
  const char* name;
  // Type of this field.
  QClassDescriptor* type;
  // Reference mode of this field.
  int8_t ref_mode;
};

#endif  // DESCRIPTOR_HPP_

