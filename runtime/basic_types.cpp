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

#include "runtime/basic_types.hpp"

namespace {

void QStringInit(QObject* o) {
  static_cast<QString*>(o)->value = new ::std::string();
}

void QStringDestroy(QObject* o) {
  QString* s = static_cast<QString*>(o);
  delete s->value;
  s->value = nullptr;
}

void QStringCopy(QObject* dest, const QObject* src) {
  *(static_cast<QString*>(dest)->value) =
      *(static_cast<const QString*>(src)->value);
}

}

const QClassDescriptor __quo_ObjectDescriptor = {
  "Object",
};

const QClassDescriptor __quo_Int32Descriptor = {
  "Int32",
};

const QClassDescriptor __quo_BoolDescriptor = {
  "Bool",
};

const QClassDescriptor __quo_StringDescriptor = {
  "String",
  &QStringInit,
  &QStringDestroy,
  &QStringCopy,
};

