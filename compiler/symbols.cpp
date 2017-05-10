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
#include "glog/logging.h"
#include "compiler/builtins.hpp"
#include "compiler/exceptions.hpp"

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

unique_ptr<Symbols> Symbols::Create(
    ::llvm::Module* module,
    const Builtins* builtins,
    const ModuleDef& module_def) {
  unique_ptr<Symbols> symbols(new Symbols(module, builtins));
  symbols->SetupFnDefs(module_def);
  symbols->SetupClassDefs(module_def);
  return symbols;
}

Symbols::Symbols(::llvm::Module* module, const Builtins* builtins)
    : ctx_(module->getContext()),
      module_(module),
      builtins_(builtins) {}

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

void Symbols::SetupClassDefs(const ModuleDef& module_def) {
  vector<const ClassDef*> class_defs;
  for (const auto& member : module_def.members()) {
    if (member.type_case() == ModuleDef::Member::kClassDef) {
      class_defs.push_back(&member.class_def());
    }
  }
  for (const ClassDef* class_def : class_defs) {
    if (class_types_by_name_.count(class_def->name())) {
      throw Exception(
          "Duplicate definition for class %s",
          class_def->name().c_str());
    }
    class_types_by_name_[class_def->name()] = {
      class_def,
      ::llvm::StructType::create(ctx_, class_def->name()),
      nullptr,
      {},
      {},
    };
  }
  for (const ClassDef* class_def : class_defs) {
    ClassType& class_type = class_types_by_name_.at(class_def->name());
    vector<::llvm::Type*> field_tys;
    for (const auto& class_member : class_def->members()) {
      try {
        switch (class_member.type_case()) {
          case ClassDef::Member::kVarDecl: {
            const VarDeclStmt& var_decl = class_member.var_decl();
            if (class_type.fields_by_name.count(var_decl.name())) {
              throw Exception(
                  "Duplicate member field '%s' in class %s",
                  var_decl.name().c_str(),
                  class_def->name().c_str());
            }
            if (!var_decl.has_type_spec()) {
              throw Exception(
                  "Missing type spec in field decl: %s",
                  var_decl.DebugString().c_str());
            }
            class_type.fields.push_back({
                var_decl.name(),
                var_decl.type_spec(),
                var_decl.ref_mode(),
                static_cast<int>(class_type.fields.size()),
            });
            class_type.fields_by_name[var_decl.name()] =
                &class_type.fields.back();
            field_tys.push_back(
                ::llvm::PointerType::getUnqual(
                    LookupType(var_decl.type_spec())));
            break;
          }
          default:
            throw Exception(
                "Unsupported class member type: %d",
                class_member.type_case());
        }
      } catch (const Exception& e) {
        throw e.withDefault(class_member.line());
      }
    }
    class_type.ty->setBody(field_tys, false);
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

::llvm::Type* Symbols::LookupType(const TypeSpec& type_spec) const {
  ::llvm::Type* builtin_ty = builtins_->LookupType(type_spec);
  if (builtin_ty != nullptr) {
    return builtin_ty;
  }
  const auto it = class_types_by_name_.find(type_spec.name());
  if (it == class_types_by_name_.end()) {
    throw Exception(
        "Unknown type %s", type_spec.ShortDebugString().c_str());
  }
  return it->second.ty;
}

::llvm::GlobalVariable* Symbols::LookupDescriptor(const TypeSpec& type_spec)
    const {
  return builtins_->LookupDescriptor(type_spec);
}

}  // namespace quo

