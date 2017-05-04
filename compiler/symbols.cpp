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

using ::std::unique_ptr;
using ::std::vector;

void Scope::AddVar(const Var& var) {
  vars.push_back(var);
  Var* varp = &vars.back();
  vars_by_name.insert({ var.name, varp});
  vars_by_ref_address.insert({ var.ref_address, varp });
}

void Scope::AddTemp(::llvm::Value* address) {
  temps.push_back(address);
}

const Var* Scope::Lookup(const ::std::string& name) const {
  auto it = vars_by_name.find(name);
  return it == vars_by_name.end() ? nullptr : it->second;
}

const Var* Scope::Lookup(::llvm::Value* address) const {
  auto it = vars_by_ref_address.find(address);
  return it == vars_by_ref_address.end() ? nullptr : it->second;
}

unique_ptr<Symbols> Symbols::Create(const ModuleDef& module_def) {
  unique_ptr<Symbols> symbols(new Symbols());
  symbols->SetupFnDefs(module_def);
  return symbols;
}

Symbols::Symbols() {}

void Symbols::SetupFnDefs(const ModuleDef& module_def) {
  for (const auto& member : module_def.members()) {
    // TODO(cji): Handle extern fns.
    if (member.type_case() != ModuleDef::Member::kFnDef) {
      continue;
    }
    const FnDef* fn_def = &member.fn_def();
    fn_defs_by_name_.insert({ fn_def->name(), fn_def });
  }
}

Scope* Symbols::PushScope() {
  scopes_.emplace_back();
  return GetScope();
}

void Symbols::PopScope() {
  scopes_.pop_back();
}

Scope* Symbols::GetScope() {
  return &(scopes_.back());
}

const vector<Scope*> Symbols::GetScopesInFn() {
  vector<Scope*> scopes_in_fn;
  for (auto scope_it = scopes_.rbegin();
      scope_it != scopes_.rend();
      ++scope_it) {
    scopes_in_fn.push_back(&(*scope_it));
    if (scope_it->is_fn_scope) {
      break;
    }
  }
  return scopes_in_fn;
}

const Var* Symbols::LookupVar(const ::std::string& name) const {
  for (auto scope_it = scopes_.rbegin();
      scope_it != scopes_.rend();
      ++scope_it) {
    const Var* var = scope_it->Lookup(name);
    if (var != nullptr) {
      return var;
    }
  }
  return nullptr;
}

const FnDef* Symbols::LookupFnDef(const ::std::string& name) const {
  const auto it = fn_defs_by_name_.find(name);
  return (it == fn_defs_by_name_.end()) ? nullptr : it->second;
}

}  // namespace quo

