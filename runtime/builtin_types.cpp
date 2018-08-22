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

#include "runtime/builtin_types.hpp"
#include <cstdio>
#include "runtime/memory.hpp"

const QClassDescriptor __quo_ObjectDescriptor = {
    "Object",
    {
        0,
        nullptr,
    },
};

const QClassDescriptor __quo_Int32Descriptor = {
    "Int32",
    {
        0,
        nullptr,
    },
};

const QClassDescriptor __quo_BoolDescriptor = {
    "Bool",
    {
        0,
        nullptr,
    },
};

const QClassDescriptor __quo_StringDescriptor = {
    "String",
    {
        0,
        nullptr,
    },
};

QString* __quo_alloc_string(const char* value, int32_t length) {
  QString* s = static_cast<QString*>(
      __quo_alloc(&__quo_StringDescriptor, sizeof(QString)));
  s->value->assign(value, length);
  return s;
}

QString* __quo_string_concat(QString* left, QString* right) {
  QString* s = static_cast<QString*>(
      __quo_alloc(&__quo_StringDescriptor, sizeof(QString)));
  s->value->assign(*left->value);
  s->value->append(*right->value);
  return s;
}

QInt32* __quo_print(const QString* s) {
  QInt32* r =
      static_cast<QInt32*>(__quo_alloc(&__quo_Int32Descriptor, sizeof(QInt32)));
  r->value = printf("%s", s->value->c_str());
  return r;
}
