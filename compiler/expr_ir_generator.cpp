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
#include "compiler/symbols.hpp"

namespace quo {

using ::std::function;
using ::std::string;
using ::std::unordered_map;
using ::std::vector;

ExprResult::ExprResult()
    : value(nullptr),
      address(nullptr),
      ref_address(nullptr),
      fn_def(nullptr) {}

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
  switch (expr.type_case()) {
    case Expr::kConstant:
      return ProcessConstantExpr(expr.constant());
    case Expr::kVar:
      return ProcessVarExpr(expr.var());
    case Expr::kCall:
      return ProcessCallExpr(expr.call());
    case Expr::kBinaryOp:
      return ProcessBinaryOpExpr(expr.binary_op());
    case Expr::kAssign:
      return ProcessAssignExpr(expr.assign(), nullptr);
    default:
      LOG(FATAL) << "Unknown expression type:" << expr.type_case();
  }
}

ExprResult ExprIRGenerator::ProcessConstantExpr(const ConstantExpr& expr) {
  ExprResult result;
  switch (expr.value_case()) {
    case ConstantExpr::kIntValue:
      result.type_spec = builtins_->types.int32_type.type_spec;
      result.value = CreateInt32Value(
          ::llvm::ConstantInt::getSigned(
              ::llvm::Type::getInt32Ty(ctx_), expr.intvalue()));
      break;
    case ConstantExpr::kBoolValue:
      result.type_spec = builtins_->types.bool_type.type_spec;
      result.value = CreateBoolValue(
          expr.boolvalue() ?
              ::llvm::ConstantInt::getTrue(ctx_) :
              ::llvm::ConstantInt::getFalse(ctx_));
      break;
    case ConstantExpr::kStrValue: {
      result.type_spec = builtins_->types.string_type.type_spec;
      ::llvm::Constant* array = ::llvm::ConstantDataArray::getString(
          ctx_,
          expr.strvalue(),
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
                  ::llvm::Type::getInt32Ty(ctx_), expr.strvalue().size()),
          });
      symbols_->GetScope()->AddTemp(result.address);
      result.value = ir_builder_->CreateLoad(result.address);
      break;
    }
    default:
      LOG(FATAL) << "Unknown constant type:" << expr.value_case();
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

  ::llvm::Function* fn = module_->getFunction(expr.name());
  const FnDef* fn_def = symbols_->LookupFnDef(expr.name());
  if (fn != nullptr && fn_def != nullptr) {
    result.fn_def = fn_def;
    result.value = fn;
    return result;
  }

  if (expr.name() == builtins_->fns.quo_print_fn_def.name()) {
    result.fn_def = &builtins_->fns.quo_print_fn_def;
    result.value = builtins_->fns.quo_print;
    return result;
  }

  LOG(FATAL) << "Unknown variable: " << expr.name();
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
        LOG(FATAL) << "Incompatible types for binary operation "
          << BinaryOpExpr::Op_Name(expr.op());
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
        LOG(FATAL) << "Incompatible types for binary operation " << expr.op();
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
        LOG(FATAL) << "Incompatible types for binary operation " << expr.op();
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
        LOG(FATAL) << "Incompatible types for binary operation " << expr.op();
      }
      break;
    default:
      LOG(FATAL) << "Unknown binary operator :"
          << BinaryOpExpr::Op_Name(expr.op());
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
  result.type_spec = CHECK_NOTNULL(fn_result.fn_def)->return_type_spec();
  result.address = ir_builder_->CreateCall(fn_result.value, arg_results);
  result.value = ir_builder_->CreateLoad(result.address);
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
      LOG(FATAL) << "Cannot assign to expression: "
                 << expr.dest_expr().ShortDebugString();
    }
  } else {
    dest_result.type_spec = value_result.type_spec;
    dest_result.ref_address = CreateLocalVar(
        value_result.value->getType(), var->name);
    dest_result.ref_mode = var->ref_mode;
  }
  if (dest_result.ref_mode == WEAK_REF && value_result.ref_address == nullptr) {
    LOG(FATAL) << "Cannot assign temp value to weak reference: "
               << expr.ShortDebugString();
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
        builtins_->globals.int32_desc,
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
        builtins_->globals.bool_desc,
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
  result->address = CreateObject(result->type_spec);
  ir_builder_->CreateStore(result->value, result->address);
  result->address->setName("temp");
  symbols_->GetScope()->AddTemp(result->address);
}

::llvm::Value* ExprIRGenerator::CreateObject(const TypeSpec& type_spec) {
  ::llvm::Type* const ty = builtins_->LookupType(type_spec);
  ::llvm::Value* const desc =
      builtins_->LookupDescriptor(type_spec);
  ::llvm::Value* value_size = ir_builder_->CreatePtrToInt(
      ir_builder_->CreateGEP(
        ::llvm::ConstantPointerNull::get(
          ::llvm::PointerType::getUnqual(ty)),
        ::llvm::ConstantInt::get(::llvm::Type::getInt32Ty(ctx_), 1)),
      ::llvm::Type::getInt32Ty(ctx_));
  ::llvm::Value* p = ir_builder_->CreatePointerCast(
      ir_builder_->CreateCall(
          builtins_->fns.quo_alloc, { desc, value_size }),
      ::llvm::PointerType::getUnqual(ty));
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

}  // namespace quo

