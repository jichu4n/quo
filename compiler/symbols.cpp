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

#include "compiler/symbols.hpp"

namespace quo {

void Scope::AddVar(const Var& var) {
  vars.push_back(var);
  Var* varp = &vars.back();
  vars_by_name.insert({ var.name, varp});
  vars_by_ref_address.insert({ var.ref_address, varp });
}

void Scope::AddTemp(::llvm::Value* address) {
  temps.push_back(address);
}

const Var* Scope::Lookup(const ::std::string& name) {
  auto it = vars_by_name.find(name);
  return it == vars_by_name.end() ? nullptr : it->second;
}

const Var* Scope::Lookup(::llvm::Value* address) {
  auto it = vars_by_ref_address.find(address);
  return it == vars_by_ref_address.end() ? nullptr : it->second;
}

}  // namespace quo

