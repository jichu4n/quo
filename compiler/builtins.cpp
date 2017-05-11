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

#include "compiler/builtins.hpp"
#include "glog/logging.h"
#include "compiler/exceptions.hpp"

namespace quo {

using ::std::unique_ptr;

unique_ptr<Builtins> Builtins::Create(::llvm::Module* module) {
  Builtins* builtins = new Builtins(module);
  builtins->SetupBuiltinTypes();
  builtins->SetupBuiltinFunctions();
  return unique_ptr<Builtins>(builtins);
}

Builtins::Builtins(::llvm::Module* module)
    : ctx_(module->getContext()),
      module_(module) {
}

void Builtins::SetupBuiltinTypes() {
  types.class_desc_ty = ::llvm::StructType::create(
      ctx_, "__quo_ClassDescriptor");

  ::llvm::Type* const field_desc_fields[] = {
    ::llvm::Type::getInt32Ty(ctx_),
    ::llvm::Type::getInt8PtrTy(ctx_),
    ::llvm::PointerType::getUnqual(types.class_desc_ty),
  };
  types.field_desc_ty = ::llvm::StructType::create(
      ctx_, field_desc_fields, "__quo_FieldDescriptor");

  ::llvm::Type* const class_view_fields[] = {
    ::llvm::PointerType::getUnqual(types.class_desc_ty),
    ::llvm::Type::getInt32Ty(ctx_),
    ::llvm::PointerType::getUnqual(types.field_desc_ty),
  };
  types.class_view_ty = ::llvm::StructType::create(
      ctx_, class_view_fields, "__quo_ClassView");

  ::llvm::Type* const class_desc_fields[] = {
    ::llvm::Type::getInt8PtrTy(ctx_),
    ::llvm::Type::getInt32Ty(ctx_),
    ::llvm::PointerType::getUnqual(types.class_view_ty),
  };
  types.class_desc_ty->setBody(
      class_desc_fields,
      false);  // isPacked

  types.object_type.type_spec.set_name("Object");
  ::llvm::Type* const object_fields[] = {
    ::llvm::PointerType::getUnqual(types.class_desc_ty),
    ::llvm::Type::getInt32Ty(ctx_),
  };
  types.object_type.ty = ::llvm::StructType::create(
      ctx_, object_fields, types.object_type.type_spec.name());

  types.int32_type.type_spec.set_name("Int32");
  ::llvm::Type* const int32_fields[] = {
    types.object_type.ty,
    ::llvm::Type::getInt32Ty(ctx_),
  };
  types.int32_type.ty = ::llvm::StructType::create(
      ctx_, int32_fields, types.int32_type.type_spec.name());
  types.int32_type.desc = new ::llvm::GlobalVariable(
      *module_,
      types.class_desc_ty,
      true,  // isConstant
      ::llvm::GlobalVariable::ExternalLinkage,
      nullptr,  // initializer,
      "__quo_Int32Descriptor",
      nullptr,  // insertBefore
      ::llvm::GlobalVariable::NotThreadLocal,
      0,  // address space
      true);  // isExternallyInitialized
  types.int_type = types.int32_type;
  types.int_type.type_spec.set_name("Int");

  types.bool_type.type_spec.set_name("Bool");
  ::llvm::Type* const bool_fields[] = {
    types.object_type.ty,
    ::llvm::Type::getInt1Ty(ctx_),
  };
  types.bool_type.ty = ::llvm::StructType::create(
      ctx_, bool_fields, types.bool_type.type_spec.name());
  types.bool_type.desc = new ::llvm::GlobalVariable(
      *module_,
      types.class_desc_ty,
      true,  // isConstant
      ::llvm::GlobalVariable::ExternalLinkage,
      nullptr,  // initializer,
      "__quo_BoolDescriptor",
      nullptr,  // insertBefore
      ::llvm::GlobalVariable::NotThreadLocal,
      0,  // address space
      true);  // isExternallyInitialized

  types.string_type.type_spec.set_name("String");
  ::llvm::Type* const string_fields[] = {
    types.object_type.ty,
    ::llvm::Type::getInt8PtrTy(ctx_),
  };
  types.string_type.ty = ::llvm::StructType::create(
      ctx_, string_fields, types.string_type.type_spec.name());
  types.string_type.desc = new ::llvm::GlobalVariable(
      *module_,
      types.class_desc_ty,
      true,  // isConstant
      ::llvm::GlobalVariable::ExternalLinkage,
      nullptr,  // initializer,
      "__quo_StringDescriptor",
      nullptr,  // insertBefore
      ::llvm::GlobalVariable::NotThreadLocal,
      0,  // address space
      true);  // isExternallyInitialized
}

void Builtins::SetupBuiltinFunctions() {
  fns.quo_alloc = module_->getOrInsertFunction(
      "__quo_alloc",
      ::llvm::FunctionType::get(
          ::llvm::Type::getInt8PtrTy(ctx_),
          {
              ::llvm::PointerType::getUnqual(types.class_desc_ty),
              ::llvm::Type::getInt32Ty(ctx_),
          },
          false));  // isVarArg
  fns.quo_inc_refs = module_->getOrInsertFunction(
      "__quo_inc_refs",
      ::llvm::FunctionType::get(
          ::llvm::Type::getVoidTy(ctx_),
          { ::llvm::Type::getInt8PtrTy(ctx_) },
          false));  // isVarArg
  fns.quo_dec_refs = module_->getOrInsertFunction(
      "__quo_dec_refs",
      ::llvm::FunctionType::get(
          ::llvm::Type::getVoidTy(ctx_),
          { ::llvm::Type::getInt8PtrTy(ctx_) },
          false));  // isVarArg
  fns.quo_assign = module_->getOrInsertFunction(
      "__quo_assign",
      ::llvm::FunctionType::get(
          ::llvm::Type::getVoidTy(ctx_),
          {
              ::llvm::PointerType::getUnqual(::llvm::Type::getInt8PtrTy(ctx_)),
              ::llvm::Type::getInt8PtrTy(ctx_),
              ::llvm::Type::getInt8Ty(ctx_),
          },
          false));  // isVarArg
  fns.quo_alloc_string = module_->getOrInsertFunction(
      "__quo_alloc_string",
      ::llvm::FunctionType::get(
          ::llvm::PointerType::getUnqual(types.string_type.ty),
          { ::llvm::Type::getInt8PtrTy(ctx_), ::llvm::Type::getInt32Ty(ctx_) },
          false));  // isVarArg
  fns.quo_string_concat = module_->getOrInsertFunction(
      "__quo_string_concat",
      ::llvm::FunctionType::get(
          ::llvm::PointerType::getUnqual(types.string_type.ty),
          {
              ::llvm::PointerType::getUnqual(types.string_type.ty),
              ::llvm::PointerType::getUnqual(types.string_type.ty),
          },
          false));  // isVarArg

  fns.quo_print_fn_def.set_name("Print");
  fns.quo_print_fn_def.mutable_return_type_spec()->CopyFrom(
      types.int32_type.type_spec);
  FnParam* param = fns.quo_print_fn_def.add_params();
  param->set_name("s");
  param->mutable_type_spec()->CopyFrom(types.string_type.type_spec);
  fns.quo_print = module_->getOrInsertFunction(
      "__quo_print",
      ::llvm::FunctionType::get(
          ::llvm::PointerType::getUnqual(types.int32_type.ty),
          { ::llvm::PointerType::getUnqual(types.string_type.ty) },
          false));  // isVarArg
}

}  // namespace quo

