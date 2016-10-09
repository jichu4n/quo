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

#include "compiler/ir_generator.hpp"
#include <algorithm>
#include <memory>
#include <vector>
#include <llvm/IR/IRBuilder.h>
#include <glog/logging.h>

namespace quo {

using ::std::unique_ptr;
using ::std::vector;
using ::std::transform;

IRGenerator::IRGenerator() {
  SetupBuiltinTypes();
}

unique_ptr<::llvm::Module> IRGenerator::ProcessModule(
    const ModuleDef& module_def) {
  unique_ptr<::llvm::Module> module(
      new ::llvm::Module(module_def.name(), ctx_));
  for (const auto& member : module_def.members()) {
    ProcessModuleMember(module.get(), member);
  }
  return module;
}

void IRGenerator::ProcessModuleMember(
    ::llvm::Module* module, const ModuleDef::Member& member) {
  switch (member.type_case()) {
    case ModuleDef::Member::kFuncDef:
      ProcessModuleFuncDef(module, member.func_def());
      break;
    default:
      LOG(FATAL) << "Unsupported module member type: " << member.type_case();
  }
}

void IRGenerator::ProcessModuleFuncDef(
    ::llvm::Module* module, const FuncDef& fn_def) {
  vector<::llvm::Type*> param_tys(fn_def.params_size());
  transform(
      fn_def.params().begin(),
      fn_def.params().end(),
      param_tys.begin(),
      [this](const FuncParam& param) {
        return LookupType(param.type_spec());
      });
  ::llvm::FunctionType* fn_ty = ::llvm::FunctionType::get(
      LookupType(fn_def.return_type_spec()), param_tys, false /* isVarArg */);
  ::llvm::Function* fn = ::llvm::Function::Create(
      fn_ty, ::llvm::Function::ExternalLinkage, fn_def.name(), module);
  int i = 0;
  for (::llvm::Argument& arg : fn->args()) {
    arg.setName(fn_def.params(i).name());
  }

  ::llvm::BasicBlock* bb = ::llvm::BasicBlock::Create(ctx_, "", fn);
  ::llvm::IRBuilder<> builder(ctx_);
  builder.SetInsertPoint(bb);

  if (fn_ty->getReturnType()->isVoidTy()) {
    builder.CreateRetVoid();
  }
}

::llvm::Type* IRGenerator::LookupType(const TypeSpec& type_spec) {
  if (type_spec.name().empty()) {
    return ::llvm::Type::getVoidTy(ctx_);
  }
  const auto it = builtin_types_map_.find(type_spec.SerializeAsString());
  if (it == builtin_types_map_.end()) {
    LOG(FATAL) << "Unknown type: " << type_spec.DebugString();
  }
  return it->second;
}

void IRGenerator::SetupBuiltinTypes() {
  TypeSpec object_type_spec;
  object_type_spec.set_name("Object");
  ::llvm::Type* const object_fields[] = {
    ::llvm::PointerType::getUnqual(::llvm::Type::getInt8Ty(ctx_)),
  };
  builtin_types_.object_ty = ::llvm::StructType::create(
      ctx_, object_fields, object_type_spec.name());
  builtin_types_map_.insert(
      {object_type_spec.SerializeAsString(), builtin_types_.object_ty});

  TypeSpec int32_type_spec;
  int32_type_spec.set_name("Int32");
  ::llvm::Type* const int32_fields[] = {
    ::llvm::PointerType::getUnqual(::llvm::Type::getInt8Ty(ctx_)),
    ::llvm::Type::getInt32Ty(ctx_),
  };
  builtin_types_.int32_ty = ::llvm::StructType::create(
      ctx_, int32_fields, int32_type_spec.name());
  TypeSpec int_type_spec;
  int_type_spec.set_name("Int");
  builtin_types_map_.insert(
      {int_type_spec.SerializeAsString(), builtin_types_.int32_ty});

  TypeSpec string_type_spec;
  int32_type_spec.set_name("String");
  ::llvm::Type* const string_fields[] = {
    ::llvm::PointerType::getUnqual(::llvm::Type::getInt8Ty(ctx_)),
    ::llvm::Type::getInt32Ty(ctx_),
    ::llvm::Type::getInt8PtrTy(ctx_),
  };
  builtin_types_.string_ty = ::llvm::StructType::create(
      ctx_, string_fields, string_type_spec.name());
  builtin_types_map_.insert(
      {string_type_spec.SerializeAsString(), builtin_types_.string_ty});
}

}  // namespace quo

