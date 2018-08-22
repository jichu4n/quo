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

using ::std::string;
using ::std::vector;
using ::std::unique_ptr;
using ::std::unordered_map;

void Scope::AddVar(const Var& var) {
  vars.push_back(var);
  Var* varp = &vars.back();
  vars_by_name.insert({ var.name, varp});
  vars_by_ref_address.insert({ var.ref_address, varp });
}

void Scope::AddTemp(::llvm::Value* address) {
  temps.push_back(address);
}

const Var* Scope::Lookup(const string& name) const {
  auto it = vars_by_name.find(name);
  return it == vars_by_name.end() ? nullptr : it->second;
}

const Var* Scope::Lookup(::llvm::Value* address) const {
  auto it = vars_by_ref_address.find(address);
  return it == vars_by_ref_address.end() ? nullptr : it->second;
}

FieldType* ClassType::LookupFieldOrThrow(const string& name) {
  auto field_it = fields_by_name.find(name);
  if (field_it == fields_by_name.end()) {
    throw Exception(
        "Class %s does not have member field '%s'",
        type_spec.ShortDebugString().c_str(),
        name.c_str());
  }
  return field_it->second;
}

FnType* ClassType::LookupMethodOrThrow(const string& name) {
  auto method_it = methods_by_name.find(name);
  if (method_it == methods_by_name.end()) {
    throw Exception(
        "Class %s does not have method '%s'",
        type_spec.ShortDebugString().c_str(),
        name.c_str());
  }
  return method_it->second;
}

ClassType::MemberType ClassType::LookupMemberOrThrow(
    const ::std::string& name) {
  auto field_it = fields_by_name.find(name);
  if (field_it != fields_by_name.end()) {
    return {ClassDef::Member::kVarDecl, field_it->second, nullptr};
  }
  auto method_it = methods_by_name.find(name);
  if (method_it != methods_by_name.end()) {
    return {ClassDef::Member::kFnDef, nullptr, method_it->second};
  }
  throw Exception(
      "Class %s does not have member field or method '%s'",
      type_spec.ShortDebugString().c_str(),
      name.c_str());
}

unique_ptr<Symbols> Symbols::Create(
    ::llvm::Module* module,
    Builtins* builtins,
    const ModuleDef& module_def) {
  unique_ptr<Symbols> symbols(new Symbols(module, builtins));
  symbols->SetupBuiltinClassTypes();
  symbols->SetupFnDefs(module_def);
  symbols->SetupClassTypes(module_def);
  return symbols;
}

Symbols::Symbols(::llvm::Module* module, Builtins* builtins)
    : ctx_(module->getContext()),
      module_(module),
      builtins_(builtins) {}

void Symbols::SetupBuiltinClassTypes() {
  class_types_map_.insert({
      builtins_->types.object_type.type_spec.SerializeAsString(),
      &builtins_->types.object_type,
  });
  class_types_map_.insert({
      builtins_->types.int32_type.type_spec.SerializeAsString(),
      &builtins_->types.int32_type,
  });
  class_types_map_.insert({
      builtins_->types.int_type.type_spec.SerializeAsString(),
      &builtins_->types.int32_type,
  });
  class_types_map_.insert({
      builtins_->types.bool_type.type_spec.SerializeAsString(),
      &builtins_->types.bool_type,
  });
  class_types_map_.insert({
      builtins_->types.string_type.type_spec.SerializeAsString(),
      &builtins_->types.string_type,
  });
}

void Symbols::SetupFnDefs(const ModuleDef& module_def) {
  for (const ModuleDef::Member& member : module_def.members()) {
    // TODO(cji): Handle extern fns.
    if (member.type_case() != ModuleDef::Member::kFnDef) {
      continue;
    }
    try {
      const FnDef& fn_def = member.fn_def();
      TypeSpec type_spec;
      type_spec.set_name(fn_def.name());
      if (fn_types_by_name_.count(fn_def.name()) +
          class_types_map_.count(type_spec.SerializeAsString())) {
        throw Exception(
            "Duplicate function or class '%s'",
            fn_def.name().c_str());
      }
      fn_types_.emplace_back();
      FnType* fn_type = &fn_types_.back();
      SetupFnType(fn_type, fn_def, nullptr);
      fn_types_by_name_[fn_def.name()] = fn_type;
    } catch (const Exception& e) {
      throw e.withDefault(member.line());
    }
  }
}

void Symbols::SetupFnType(
    FnType* fn_type, const FnDef& fn_def, ClassType* class_type) {
  fn_type->fn_def = &fn_def;
  fn_type->name = fn_def.name();
  vector<::llvm::Type*> param_tys;
  if (class_type != nullptr) {
    param_tys.push_back(
        ::llvm::PointerType::getUnqual(class_type->ty));
  }
  for (const FnParam& param : fn_def.params()) {
    param_tys.push_back(
        ::llvm::PointerType::getUnqual(
            LookupTypeOrDie(param.type_spec())->ty));
  }
  ::llvm::Type* const raw_return_ty =
      LookupTypeOrDie(fn_def.return_type_spec())->ty;
  ::llvm::Type* const return_ty = raw_return_ty->isVoidTy() ?
      raw_return_ty :
      ::llvm::PointerType::getUnqual(raw_return_ty);
  fn_type->fn_ty = ::llvm::FunctionType::get(
      return_ty,
      param_tys,
      false /* isVarArg */);
  fn_type->fn = ::llvm::Function::Create(
      fn_type->fn_ty,
      ::llvm::Function::ExternalLinkage,
      fn_type->name,
      module_);
  int i = 0;
  auto fn_params_it = fn_def.params().begin();
  for (::llvm::Argument& arg : fn_type->fn->args()) {
    if (i == 0 && class_type != nullptr) {
      arg.setName("this");
    } else {
      arg.setName(fn_params_it->name());
      ++fn_params_it;
    }
    ++i;
  }
}

void Symbols::SetupClassTypes(const ModuleDef& module_def) {
  unordered_map<string, const ClassDef*> class_defs;
  for (const auto& member : module_def.members()) {
    if (member.type_case() == ModuleDef::Member::kClassDef) {
      const ClassDef* class_def = &member.class_def();
      TypeSpec type_spec;
      type_spec.set_name(class_def->name());
      const string& key = type_spec.SerializeAsString();
      if (class_defs.count(key) + fn_types_by_name_.count(class_def->name())) {
        throw Exception(
            member.line(),
            "Duplicate function or class '%s'",
            class_def->name().c_str());
      }
      class_defs[key] = class_def;
    }
  }
  // 1. First, we insert a placeholder for all classes into
  // class_types_by_name_, which will allow LookupDescriptor and LookupType to
  // work for all classes.
  for (const auto& it : class_defs) {
    TypeSpec type_spec;
    type_spec.ParseFromString(it.first);
    const ClassDef* class_def = it.second;
    class_types_.push_back({
      type_spec,
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
      {},
      {},
    });
    class_types_map_[it.first] = &class_types_.back();
  }
  // 2. Actually populate class types in class_types_by_name_.
  for (const auto& it : class_defs) {
    ClassType* class_type = class_types_map_.at(it.first);
    SetupClassFields(class_type, *it.second);
    SetupClassMethods(class_type, *it.second);
  }
}

void Symbols::SetupClassFields(
    ClassType* class_type, const ClassDef& class_def) {
  vector<::llvm::Type*> field_tys;
  field_tys.push_back(builtins_->types.object_type.ty);
  vector<::llvm::Constant*> field_descs;
  for (const auto& class_member : class_def.members()) {
    if (class_member.type_case() != ClassDef::Member::kVarDecl) {
      continue;
    }
    try {
      const VarDeclStmt& var_decl = class_member.var_decl();
      if (class_type->fields_by_name.count(var_decl.name()) +
          class_type->methods_by_name.count(var_decl.name())) {
        throw Exception(
            "Duplicate member field or method '%s' in class %s",
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
              LookupTypeOrDie(var_decl.type_spec())->ty));
      ::llvm::Constant* field_name_array =
          ::llvm::ConstantDataArray::getString(ctx_, var_decl.name());
      field_descs.push_back(
          ::llvm::ConstantStruct::get(
              builtins_->types.field_desc_ty,
              {
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
                LookupTypeOrDie(var_decl.type_spec())->desc,
                ::llvm::ConstantInt::getSigned(
                    ::llvm::Type::getInt8Ty(ctx_), var_decl.ref_mode()),
              }));
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
                      {
                        class_type->desc,
                        ::llvm::ConstantInt::getSigned(
                            ::llvm::Type::getInt32Ty(ctx_), field_descs.size()),
                        ::llvm::ConstantExpr::getPointerCast(
                            field_desc_array,
                            ::llvm::PointerType::getUnqual(
                                builtins_->types.field_desc_ty)),
                      }),
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
          {
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
          }));
}

void Symbols::SetupClassMethods(
    ClassType* class_type, const ClassDef& class_def) {
  for (const auto& class_member : class_def.members()) {
    if (class_member.type_case() != ClassDef::Member::kFnDef) {
      continue;
    }
    try {
      const FnDef& fn_def = class_member.fn_def();
      if (class_type->fields_by_name.count(fn_def.name()) +
          class_type->methods_by_name.count(fn_def.name())) {
        throw Exception(
            "Duplicate member field or method '%s' in class %s",
            fn_def.name().c_str(),
            class_def.name().c_str());
      }
      class_type->methods.emplace_back();
      FnType* fn_type = &class_type->methods.back();
      SetupFnType(fn_type, fn_def, class_type);
      class_type->methods_by_name[fn_def.name()] = fn_type;
    } catch (const Exception& e) {
      throw e.withDefault(class_member.line());
    }
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

const Var* Symbols::LookupVar(const string& name) const {
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

const FnType* Symbols::LookupFn(const string& name) const {
  const auto it = fn_types_by_name_.find(name);
  return (it == fn_types_by_name_.end()) ? nullptr : it->second;
}

const FnType* Symbols::LookupFnOrThrow(const string& name) const {
  const FnType* fn_type = LookupFn(name);
  if (fn_type == nullptr) {
    throw Exception(
        "Unknown function '%s'", name.c_str());
  }
  return fn_type;
}

ClassType* Symbols::LookupType(const TypeSpec& type_spec) const {
  const auto it = class_types_map_.find(type_spec.SerializeAsString());
  if (it == class_types_map_.end()) {
    return nullptr;
  }
  return it->second;
}

ClassType* Symbols::LookupTypeOrDie(const TypeSpec& type_spec) const {
  ClassType* class_type = LookupType(type_spec);
  if (class_type == nullptr) {
    throw Exception(
        "Unknown type '%s'", type_spec.ShortDebugString().c_str());
  }
  return class_type;
}

}  // namespace quo

