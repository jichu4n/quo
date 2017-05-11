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

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <cstdint>
#include "runtime/builtin_types.hpp"

// A user-defined class instance.
struct QCustomObject : public QObject {
  // Array of member fields.
  QObject* fields[1];
};

extern "C" {

// Allocate "size"-bytes of memory.
extern QObject* __quo_alloc(const QClassDescriptor* dp, int32_t size);
// Increment reference count.
extern void __quo_inc_refs(QObject* ptr);
// Decrement reference count, and dealloc if reference count becomes zero.
extern void __quo_dec_refs(QObject* ptr);
// Assign src to dest.
extern void __quo_assign(QObject** dest, QObject* src, int8_t ref_mode);

// Object field access.
//
// First, this casts the object to the given class (via its class view table)
// and return the address within the object of the index-th field of the given
// class.
extern QObject** __quo_get_field(
    QObject* obj, QClassDescriptor* view_class, int32_t index);

}

#endif  // MEMORY_HPP_

