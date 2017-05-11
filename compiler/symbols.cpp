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
#include "llvm/IR/Constants.h"
#include "compiler/builtins.hpp"
#include "compiler/exceptions.hpp"
#include "compiler/utils.hpp"

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
  // 1. First, we insert a placeholder for all classes into
  // class_types_by_name_, which will allow LookupDescriptor and LookupType to
  // work for all classes.
  for (const ClassDef* class_def : class_defs) {
    if (class_types_by_name_.count(class_def->name())) {
      throw Exception(
          "Duplicate definition for class %s",
          class_def->name().c_str());
    }
    class_types_by_name_[class_def->name()] = {
      class_def,
      ::llvm::StructType::create(ctx_, class_def->name()),
      new ::llvm::GlobalVariable(
          *module_,
          builtins_->types.class_desc_ty,
          true,  // isConstant
          ::llvm::GlobalVariable::ExternalLinkage,
          nullptr,  // initializer,
          StringPrintf("__%s_Descriptor", class_def->name().c_str()),
          nullptr,  // insertBefore
          ::llvm::GlobalVariable::NotThreadLocal,
          0,  // address space
          false),  // isExternallyInitialized
      {},
      {},
    };
  }
  // 2. Actually populate class types in class_types_by_name_.
  for (const ClassDef* class_def : class_defs) {
    SetupClassDef(&class_types_by_name_.at(class_def->name()), *class_def);
  }
}

void Symbols::SetupClassDef(ClassType* class_type, const ClassDef& class_def) {
  vector<::llvm::Type*> field_tys;
  vector<::llvm::Constant*> field_descs;
  for (const auto& class_member : class_def.members()) {
    try {
      switch (class_member.type_case()) {
        case ClassDef::Member::kVarDecl: {
          const VarDeclStmt& var_decl = class_member.var_decl();
          if (class_type->fields_by_name.count(var_decl.name())) {
            throw Exception(
                "Duplicate member field '%s' in class %s",
                var_decl.name().c_str(),
                class_def.name().c_str());
          }
          if (!var_decl.has_type_spec()) {
            throw Exception(
                "Missing type spec in field decl: %s",
                var_decl.DebugString().c_str());
          }
          int field_index = class_type->fields.size();
          class_type->fields.push_back({
              var_decl.name(),
              var_decl.type_spec(),
              var_decl.ref_mode(),
              static_cast<int>(field_index),
          });
          class_type->fields_by_name[var_decl.name()] =
              &class_type->fields.back();
          field_tys.push_back(
              ::llvm::PointerType::getUnqual(
                  LookupType(var_decl.type_spec())));
          ::llvm::Constant* field_name_array =
              ::llvm::ConstantDataArray::getString(ctx_, var_decl.name());
          field_descs.push_back(
              ::llvm::ConstantStruct::get(
                  builtins_->types.field_desc_ty,
                  ::llvm::ConstantInt::getSigned(
                      ::llvm::Type::getInt32Ty(ctx_), field_index),
                  ::llvm::ConstantExpr::getPointerCast(
                      new ::llvm::GlobalVariable(
                          *module_,
                          field_name_array->getType(),
                          true,  // isConstant
                          ::llvm::GlobalVariable::PrivateLinkage,
                          field_name_array,
                          StringPrintf(
                            "__%s_FieldName_%s",
                            class_type->class_def->name().c_str(),
                            var_decl.name().c_str()),
                          nullptr,  // insertBefore
                          ::llvm::GlobalVariable::NotThreadLocal,
                          0,  // address space
                          false),  // isExternallyInitialized
                      ::llvm::Type::getInt8PtrTy(ctx_)),
                  LookupDescriptor(var_decl.type_spec()),
                  nullptr));
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
  class_type->ty->setBody(field_tys, false);
  ::llvm::ArrayType* field_desc_array_ty =
      ::llvm::ArrayType::get(
          builtins_->types.field_desc_ty, field_descs.size());
  ::llvm::GlobalVariable* field_desc_array =
      new ::llvm::GlobalVariable(
          *module_,
          field_desc_array_ty,
          true,  // isConstant
          ::llvm::GlobalVariable::PrivateLinkage,
          ::llvm::ConstantArray::get(field_desc_array_ty, field_descs),
          StringPrintf(
            "__%s_FieldDescriptors", class_type->class_def->name().c_str()),
          nullptr,  // insertBefore
          ::llvm::GlobalVariable::NotThreadLocal,
          0,  // address space
          false);  // isExternallyInitialized
  ::llvm::ArrayType* view_array_ty =
      ::llvm::ArrayType::get(
          builtins_->types.class_view_ty, 1);
  ::llvm::GlobalVariable* view_array =
      new ::llvm::GlobalVariable(
          *module_,
          view_array_ty,
          true,  // isConstant
          ::llvm::GlobalVariable::PrivateLinkage,
          ::llvm::ConstantArray::get(
              view_array_ty,
              {
                  ::llvm::ConstantStruct::get(
                      builtins_->types.class_view_ty,
                      class_type->desc,
                      ::llvm::ConstantInt::getSigned(
                          ::llvm::Type::getInt32Ty(ctx_), field_descs.size()),
                      ::llvm::ConstantExpr::getPointerCast(
                          field_desc_array,
                          ::llvm::PointerType::getUnqual(
                              builtins_->types.field_desc_ty)),
                      nullptr),
              }),
          StringPrintf(
            "__%s_View", class_type->class_def->name().c_str()),
          nullptr,  // insertBefore
          ::llvm::GlobalVariable::NotThreadLocal,
          0,  // address space
          false);  // isExternallyInitialized
  ::llvm::Constant* class_name_array =
      ::llvm::ConstantDataArray::getString(ctx_, class_type->class_def->name());
  class_type->desc->setInitializer(
      ::llvm::ConstantStruct::get(
          builtins_->types.class_desc_ty,
          ::llvm::ConstantExpr::getPointerCast(
              new ::llvm::GlobalVariable(
                  *module_,
                  class_name_array->getType(),
                  true,  // isConstant
                  ::llvm::GlobalVariable::PrivateLinkage,
                  class_name_array,
                  StringPrintf(
                    "__%s_ClassName", class_type->class_def->name().c_str()),
                  nullptr,  // insertBefore
                  ::llvm::GlobalVariable::NotThreadLocal,
                  0,  // address space
                  false),  // isExternallyInitialized
              ::llvm::Type::getInt8PtrTy(ctx_)),
          ::llvm::ConstantInt::getSigned(
              ::llvm::Type::getInt32Ty(ctx_), 1),
          ::llvm::ConstantExpr::getPointerCast(
              view_array,
              ::llvm::PointerType::getUnqual(
                  builtins_->types.class_view_ty)),
          nullptr));
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
  ::llvm::GlobalVariable* builtin_desc = builtins_->LookupDescriptor(type_spec);
  if (builtin_desc != nullptr) {
    return builtin_desc;
  }
  const auto it = class_types_by_name_.find(type_spec.name());
  if (it == class_types_by_name_.end()) {
    throw Exception(
        "Unknown type %s", type_spec.ShortDebugString().c_str());
  }
  return it->second.desc;
}

}  // namespace quo

