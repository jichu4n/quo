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

#include "compiler/expr_ir_generator.hpp"
#include <functional>
#include <vector>
#include "glog/logging.h"
#include "compiler/builtins.hpp"
#include "compiler/exceptions.hpp"
#include "compiler/symbols.hpp"
#include "compiler/utils.hpp"

namespace quo {

using ::std::function;
using ::std::string;
using ::std::unordered_map;
using ::std::vector;

ExprResult::ExprResult()
    : value(nullptr),
      address(nullptr),
      ref_address(nullptr),
      fn_def(nullptr),
      class_type(nullptr) {}

ExprIRGenerator::ExprIRGenerator(
    ::llvm::Module* module,
    ::llvm::IRBuilder<>* ir_builder,
    const Builtins* builtins,
    Symbols* symbols)
    : ctx_(module->getContext()),
      module_(module),
      ir_builder_(ir_builder),
      builtins_(builtins),
      symbols_(symbols) {}


ExprResult ExprIRGenerator::ProcessExpr(const Expr& expr) {
  try {
    switch (expr.type_case()) {
      case Expr::kConstant:
        return ProcessConstantExpr(expr.constant());
      case Expr::kVar:
        return ProcessVarExpr(expr.var());
      case Expr::kMember:
        return ProcessMemberExpr(expr.member());
      case Expr::kCall:
        return ProcessCallExpr(expr.call());
      case Expr::kBinaryOp:
        return ProcessBinaryOpExpr(expr.binary_op());
      case Expr::kAssign:
        return ProcessAssignExpr(expr.assign(), nullptr);
      default:
        throw Exception("Unknown expression type: %d", expr.type_case());
    }
  } catch (const Exception& e) {
    throw e.withDefault(expr.line());
  }
}

ExprResult ExprIRGenerator::ProcessConstantExpr(const ConstantExpr& expr) {
  ExprResult result;
  switch (expr.value_case()) {
    case ConstantExpr::kIntValue:
      result.type_spec = builtins_->types.int32_type.type_spec;
      result.value = CreateInt32Value(
          ::llvm::ConstantInt::getSigned(
              ::llvm::Type::getInt32Ty(ctx_), expr.int_value()));
      break;
    case ConstantExpr::kBoolValue:
      result.type_spec = builtins_->types.bool_type.type_spec;
      result.value = CreateBoolValue(
          expr.bool_value() ?
              ::llvm::ConstantInt::getTrue(ctx_) :
              ::llvm::ConstantInt::getFalse(ctx_));
      break;
    case ConstantExpr::kStrValue: {
      result.type_spec = builtins_->types.string_type.type_spec;
      ::llvm::Constant* array = ::llvm::ConstantDataArray::getString(
          ctx_,
          expr.str_value(),
          false);  // addNull
      ::llvm::GlobalVariable* array_var = new ::llvm::GlobalVariable(
          *module_,
          array->getType(),
          true,  // isConstant
          ::llvm::GlobalValue::PrivateLinkage,
          array);
      result.address = ir_builder_->CreateCall(
          builtins_->fns.quo_alloc_string,
          {
              ir_builder_->CreateBitCast(
                  array_var,
                  ::llvm::Type::getInt8PtrTy(ctx_)),
              ::llvm::ConstantInt::getSigned(
                  ::llvm::Type::getInt32Ty(ctx_), expr.str_value().size()),
          });
      symbols_->GetScope()->AddTemp(result.address);
      result.value = ir_builder_->CreateLoad(result.address);
      break;
    }
    default:
      throw Exception("Unknown constant type: %d", expr.value_case());
  }
  return result;
}

ExprResult ExprIRGenerator::ProcessVarExpr(
    const VarExpr& expr) {
  ExprResult result;

  const Var* var = symbols_->LookupVar(expr.name());
  if (var != nullptr) {
    result.type_spec = var->type_spec;
    result.ref_address = var->ref_address;
    result.address = ir_builder_->CreateLoad(
        var->ref_address, expr.name() + "_addr");
    result.ref_mode = var->ref_mode;
    result.value = ir_builder_->CreateLoad(
        result.address, expr.name() + "_val");
    return result;
  }

  const FnType* fn_type = symbols_->LookupFn(expr.name());
  if (fn_type != nullptr) {
    result.fn_def = fn_type->fn_def;
    result.value = fn_type->fn;
    return result;
  }

  TypeSpec type_spec;
  type_spec.set_name(expr.name());
  ClassType* class_type = symbols_->LookupType(type_spec);
  if (class_type != nullptr) {
    result.type_spec = class_type->type_spec;
    result.value = class_type->desc;
    result.class_type = class_type;
    return result;
  }

  if (expr.name() == builtins_->fns.quo_print_fn_def.name()) {
    result.fn_def = &builtins_->fns.quo_print_fn_def;
    result.value = builtins_->fns.quo_print;
    return result;
  }

  throw Exception("Unknown variable: %s", expr.name().c_str());
}

ExprResult ExprIRGenerator::ProcessMemberExpr(const MemberExpr& expr) {
  ExprResult parent_result = ProcessExpr(expr.parent_expr());
  EnsureAddress(&parent_result);
  ClassType* parent_class_type = symbols_->LookupTypeOrDie(
      parent_result.type_spec);
  ClassType::MemberType member_type = parent_class_type->LookupMemberOrThrow(
      expr.member_name());
  ExprResult result;
  if (member_type.type_case == ClassDef::Member::kVarDecl) {
    FieldType* field_type = member_type.field_type;
    const string& value_name = StringPrintf(
        "%s_%s",
        parent_result.type_spec.name().c_str(),
        field_type->name.c_str());
    result.type_spec = field_type->type_spec;
    result.ref_address = GetFieldRefAddress(
        parent_class_type, field_type, parent_result.address);
    result.address = ir_builder_->CreateLoad(
        result.ref_address, StringPrintf("%s_addr", value_name.c_str()));
    result.value = ir_builder_->CreateLoad(result.address, value_name);
    result.ref_mode = field_type->ref_mode;
  } else if (member_type.type_case == ClassDef::Member::kFnDef) {
    FnType* fn_type = member_type.method_type;
    result.fn_def = fn_type->fn_def;
    result.value = fn_type->fn;
    result.address = parent_result.address;
    return result;
  } else {
    throw Exception("Unknown class member type: %d", member_type.type_case);
  }
  return result;
}

ExprResult ExprIRGenerator::ProcessBinaryOpExpr(
    const BinaryOpExpr& expr) {
  static const unordered_map<
      int,
      function<::llvm::Value*(::llvm::Value*, ::llvm::Value*)>> kIntOps = {
      {
          static_cast<int>(BinaryOpExpr::ADD),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateAdd(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::SUB),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            ir_builder_->CreateSub(l, r);
            return ir_builder_->CreateSub(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::MUL),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateMul(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::DIV),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateSDiv(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::MOD),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateSRem(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::EQ),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateICmpEQ(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::NE),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateICmpNE(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::GT),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateICmpSGT(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::GE),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateICmpSGE(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::LT),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateICmpSLT(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::LE),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateICmpSLE(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::AND),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateAnd(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::OR),
          [this](::llvm::Value* l, ::llvm::Value* r) {
            return ir_builder_->CreateOr(l, r);
          }
      },
  };

  ExprResult result;
  ExprResult left_result = ProcessExpr(expr.left_expr());
  ExprResult right_result = ProcessExpr(expr.right_expr());
  switch (expr.op()) {
    case BinaryOpExpr::ADD:
      if (left_result.value->getType() ==
          builtins_->types.string_type.ty &&
          right_result.value->getType() ==
          builtins_->types.string_type.ty) {
        result.type_spec = builtins_->types.string_type.type_spec;
        result.address = ir_builder_->CreateCall(
            builtins_->fns.quo_string_concat,
            { left_result.address, right_result.address });
        symbols_->GetScope()->AddTemp(result.address);
        result.value = ir_builder_->CreateLoad(result.address);
        break;
      }
      // fall through
    case BinaryOpExpr::SUB:
    case BinaryOpExpr::MUL:
    case BinaryOpExpr::DIV:
    case BinaryOpExpr::MOD:
      if (left_result.value->getType() ==
          builtins_->types.int32_type.ty &&
          right_result.value->getType() ==
          builtins_->types.int32_type.ty) {
        result.type_spec = builtins_->types.int32_type.type_spec;
        result.value = CreateInt32Value(
            kIntOps.at(expr.op())(
                ExtractInt32Value(left_result.value),
                ExtractInt32Value(right_result.value)));
      } else {
        throw Exception(
                "Incompatible types for binary operation %s: %s",
                BinaryOpExpr::Op_Name(expr.op()).c_str(),
                expr.ShortDebugString().c_str());
      }
      break;
    case BinaryOpExpr::GT:
    case BinaryOpExpr::GE:
    case BinaryOpExpr::LT:
    case BinaryOpExpr::LE:
      if (left_result.value->getType() ==
          builtins_->types.int32_type.ty &&
          right_result.value->getType() ==
          builtins_->types.int32_type.ty) {
        result.type_spec = builtins_->types.bool_type.type_spec;
        result.value = CreateBoolValue(
            kIntOps.at(expr.op())(
                ExtractInt32Value(left_result.value),
                ExtractInt32Value(right_result.value)));
      } else {
        throw Exception(
                "Incompatible types for binary operation %s: %s",
                BinaryOpExpr::Op_Name(expr.op()).c_str(),
                expr.ShortDebugString().c_str());
      }
      break;
    case BinaryOpExpr::EQ:
    case BinaryOpExpr::NE:
      if (left_result.value->getType() ==
          builtins_->types.int32_type.ty &&
          right_result.value->getType() ==
          builtins_->types.int32_type.ty) {
        result.type_spec = builtins_->types.bool_type.type_spec;
        result.value = CreateBoolValue(
            kIntOps.at(expr.op())(
                ExtractInt32Value(left_result.value),
                ExtractInt32Value(right_result.value)));
      } else if (left_result.value->getType() ==
          builtins_->types.bool_type.ty &&
          right_result.value->getType() ==
          builtins_->types.bool_type.ty) {
        result.type_spec = builtins_->types.bool_type.type_spec;
        result.value = CreateBoolValue(
            kIntOps.at(expr.op())(
                ExtractBoolValue(left_result.value),
                ExtractBoolValue(right_result.value)));
      } else {
        throw Exception(
                "Incompatible types for binary operation %s: %s",
                BinaryOpExpr::Op_Name(expr.op()).c_str(),
                expr.ShortDebugString().c_str());
      }
      break;
    case BinaryOpExpr::AND:
    case BinaryOpExpr::OR:
      if (left_result.value->getType() ==
          builtins_->types.bool_type.ty &&
          right_result.value->getType() ==
          builtins_->types.bool_type.ty) {
        result.type_spec = builtins_->types.bool_type.type_spec;
        result.value = CreateBoolValue(
            kIntOps.at(expr.op())(
                ExtractBoolValue(left_result.value),
                ExtractBoolValue(right_result.value)));
      } else {
        throw Exception(
                "Incompatible types for binary operation %s: %s",
                BinaryOpExpr::Op_Name(expr.op()).c_str(),
                expr.ShortDebugString().c_str());
      }
      break;
    default:
        throw Exception(
                "Unknown binary operator %s: %s",
                BinaryOpExpr::Op_Name(expr.op()).c_str(),
                expr.ShortDebugString().c_str());
  }
  return result;
}

ExprResult ExprIRGenerator::ProcessCallExpr(
    const CallExpr& expr) {
  ExprResult result;
  ExprResult fn_result = ProcessExpr(expr.fn_expr());
  vector<::llvm::Value*> arg_results(expr.arg_exprs_size());
  transform(
      expr.arg_exprs().begin(),
      expr.arg_exprs().end(),
      arg_results.begin(),
      [this](Expr e) {
        ExprResult arg_result = ProcessExpr(e);
        EnsureAddress(&arg_result);
        return arg_result.address;
      });
  if (fn_result.fn_def != nullptr) {
    if (fn_result.address != nullptr) {
      // Insert "this" argument.
      arg_results.insert(arg_results.begin(), fn_result.address);
    }
    result.type_spec = fn_result.fn_def->return_type_spec();
    result.address = ir_builder_->CreateCall(fn_result.value, arg_results);
    result.value = ir_builder_->CreateLoad(result.address);
  } else if (fn_result.class_type != nullptr) {
    result.type_spec = fn_result.type_spec;
    result.address = CreateObject(fn_result.class_type);
    result.value = ir_builder_->CreateLoad(result.address);
  }
  symbols_->GetScope()->AddTemp(result.address);
  return result;
}

ExprResult ExprIRGenerator::ProcessAssignExpr(
    const AssignExpr& expr, const Var* var) {
  ExprResult value_result = ProcessExpr(expr.value_expr());
  ExprResult dest_result;
  if (var == nullptr) {
    dest_result = ProcessExpr(expr.dest_expr());
    if (dest_result.ref_address == nullptr) {
      throw Exception(
          "Cannot assign to value expression: %s",
          expr.dest_expr().ShortDebugString().c_str());
    }
  } else {
    dest_result.type_spec = value_result.type_spec;
    dest_result.ref_address = CreateLocalVar(
        value_result.value->getType(), var->name);
    dest_result.ref_mode = var->ref_mode;
  }
  if (dest_result.ref_mode == WEAK_REF && value_result.ref_address == nullptr) {
    throw Exception(
        "Cannot assign temp value to weak reference: %s",
        expr.ShortDebugString().c_str());
  }
  EnsureAddress(&value_result);
  ::llvm::Value* dest_address = AssignObject(
      dest_result.ref_address,
      value_result.address,
      dest_result.ref_mode);
  ExprResult result;
  result.type_spec = dest_result.type_spec;
  result.value = ir_builder_->CreateLoad(dest_address);
  result.address = dest_address;
  result.ref_address = dest_result.ref_address;
  result.ref_mode = dest_result.ref_mode;
  return result;
}

::llvm::Value* ExprIRGenerator::CreateInt32Value(
    ::llvm::Value* raw_int32_value) {
  ::llvm::Value* init_value = ::llvm::ConstantStruct::get(
      builtins_->types.int32_type.ty,
      ::llvm::ConstantStruct::get(
          builtins_->types.object_type.ty,
          builtins_->types.int32_type.desc,
          ::llvm::ConstantInt::getSigned(::llvm::Type::getInt32Ty(ctx_), 1),
          nullptr),
      ::llvm::ConstantInt::getSigned(::llvm::Type::getInt32Ty(ctx_), 0),
      nullptr);
  return ir_builder_->CreateInsertValue(
      init_value, raw_int32_value, {1});
}

::llvm::Value* ExprIRGenerator::ExtractInt32Value(
    ::llvm::Value* wrapped_int32_value) {
  return ir_builder_->CreateExtractValue(wrapped_int32_value, {1});
}

::llvm::Value* ExprIRGenerator::CreateBoolValue(
    ::llvm::Value* raw_bool_value) {
  ::llvm::Value* init_value = ::llvm::ConstantStruct::get(
      builtins_->types.bool_type.ty,
      ::llvm::ConstantStruct::get(
          builtins_->types.object_type.ty,
          builtins_->types.bool_type.desc,
          ::llvm::ConstantInt::getSigned(::llvm::Type::getInt32Ty(ctx_), 1),
          nullptr),
      ::llvm::ConstantInt::getSigned(::llvm::Type::getInt1Ty(ctx_), 0),
      nullptr);
  return ir_builder_->CreateInsertValue(
      init_value, raw_bool_value, {1});
}

::llvm::Value* ExprIRGenerator::ExtractBoolValue(
    ::llvm::Value* wrapped_bool_value) {
  return ir_builder_->CreateExtractValue(wrapped_bool_value, {1});
}

void ExprIRGenerator::EnsureAddress(ExprResult* result) {
  if (result->address != nullptr) {
    return;
  }
  result->address = CreateObject(symbols_->LookupTypeOrDie(result->type_spec));
  ir_builder_->CreateStore(result->value, result->address);
  result->address->setName("temp");
  symbols_->GetScope()->AddTemp(result->address);
}

::llvm::Value* ExprIRGenerator::CreateObject(ClassType* class_type) {
  CHECK_NOTNULL(class_type);
  ::llvm::Value* value_size = ir_builder_->CreatePtrToInt(
      ir_builder_->CreateGEP(
        ::llvm::ConstantPointerNull::get(
          ::llvm::PointerType::getUnqual(class_type->ty)),
        ::llvm::ConstantInt::get(::llvm::Type::getInt32Ty(ctx_), 1)),
      ::llvm::Type::getInt32Ty(ctx_));
  ::llvm::Value* p = ir_builder_->CreatePointerCast(
      ir_builder_->CreateCall(
          builtins_->fns.quo_alloc,
          {
              class_type->desc,
              value_size,
          }),
      ::llvm::PointerType::getUnqual(class_type->ty));
  return p;
}

::llvm::Value* ExprIRGenerator::AssignObject(
    ::llvm::Value* dest_ref_address,
    ::llvm::Value* src_address,
    RefMode ref_mode) {
  ir_builder_->CreateCall(
      builtins_->fns.quo_assign,
      {
          ir_builder_->CreateBitCast(
              dest_ref_address,
              ::llvm::PointerType::getUnqual(
                ::llvm::Type::getInt8PtrTy(ctx_))),
          ir_builder_->CreateBitCast(
              src_address, ::llvm::Type::getInt8PtrTy(ctx_)),
          ::llvm::ConstantInt::get(
              ::llvm::Type::getInt8Ty(ctx_), static_cast<int8_t>(ref_mode)),
      });
  return src_address;
}

::llvm::Value* ExprIRGenerator::CreateLocalVar(
    ::llvm::Type* ty, const string& name) {
  ::llvm::Value* ref_address = ir_builder_->CreateAlloca(
      ::llvm::PointerType::getUnqual(ty), nullptr, name);
  ir_builder_->CreateStore(
      ::llvm::ConstantPointerNull::get(::llvm::PointerType::getUnqual(ty)),
      ref_address);
  return ref_address;
}

::llvm::Value* ExprIRGenerator::GetFieldRefAddress(
    const ClassType* parent_class_type,
    const FieldType* field_type,
    ::llvm::Value* object_address) {
  return ir_builder_->CreateBitCast(
      ir_builder_->CreateCall(
          builtins_->fns.quo_get_field,
          {
              ir_builder_->CreateBitCast(
                  object_address,
                  ::llvm::Type::getInt8PtrTy(ctx_)),
              parent_class_type->desc,
              ::llvm::ConstantInt::getSigned(
                  ::llvm::Type::getInt32Ty(ctx_), field_type->index),
          }),
        ::llvm::PointerType::getUnqual(
            ::llvm::PointerType::getUnqual(
                symbols_->LookupTypeOrDie(field_type->type_spec)->ty)),
        StringPrintf(
            "%s_%s_ref",
            parent_class_type->class_def->name().c_str(),
            field_type->name.c_str()));
}

}  // namespace quo

