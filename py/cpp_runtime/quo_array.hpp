/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2015 Chuan Ji <jichuan89@gmail.com>                        *
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

#ifndef QUO_ARRAY_HPP
#define QUO_ARRAY_HPP

#include "quo_basic_types.hpp"
#include <memory>
#include <vector>

// Array class implementation. A Quo Array is basically a wrapper around
// ::std::vector.
template<typename T>
class Array {
  public:
    T* operator [] (int idx) {
      return vector_[idx].get();
    }
    void Append(const T& t) {
      vector_.emplace_back(new T(t));
    }
    void Append(::std::unique_ptr<T> t) {
      vector_.emplace_back(std::move(t));
    }
    Int Length() {
      return vector_.size();
    }
  private:
    ::std::vector<::std::unique_ptr<T>> vector_;
};

#endif
