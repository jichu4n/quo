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
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glog/logging.h>

namespace quo {

using ::std::bind;
using ::std::function;
using ::std::mem_fn;
using ::std::string;
using ::std::transform;
using ::std::unordered_map;
using ::std::unique_ptr;
using ::std::vector;

struct IRGenerator::State {
  ::llvm::Module* module;
  const ModuleDef* module_def;
  ::llvm::FunctionType* fn_ty;
  ::llvm::Function* fn;
  const FnDef* fn_def;
  ::llvm::IRBuilder<>* ir_builder;
  unordered_map<string, ::llvm::Value*> locals;

  struct {
    ::llvm::Constant* quo_alloc;
    ::llvm::Constant* quo_free;
    ::llvm::Constant* quo_copy;
  } builtin_fns_;
};

IRGenerator::IRGenerator() {
  SetupBuiltinTypes();
}

unique_ptr<::llvm::Module> IRGenerator::ProcessModule(
    const ModuleDef& module_def) {
  unique_ptr<::llvm::Module> module(
      new ::llvm::Module(module_def.name(), ctx_));
  State state;
  state.module = module.get();
  state.module_def = &module_def;
  SetupBuiltinFunctions(&state);
  for (const auto& member : module_def.members()) {
    ProcessModuleMember(&state, member);
  }
  return module;
}

void IRGenerator::SetupBuiltinTypes() {
  TypeSpec object_type_spec;
  object_type_spec.set_name("Object");
  ::llvm::Type* const object_fields[] = {
    ::llvm::Type::getInt8PtrTy(ctx_),
  };
  builtin_types_.object_ty = ::llvm::StructType::create(
      ctx_, object_fields, object_type_spec.name());
  builtin_types_map_.insert(
      {object_type_spec.SerializeAsString(), builtin_types_.object_ty});

  TypeSpec int32_type_spec;
  int32_type_spec.set_name("Int32");
  ::llvm::Type* const int32_fields[] = {
    ::llvm::Type::getInt8PtrTy(ctx_),
    ::llvm::Type::getInt32Ty(ctx_),
  };
  builtin_types_.int32_ty = ::llvm::StructType::create(
      ctx_, int32_fields, int32_type_spec.name());
  builtin_types_map_.insert(
      {int32_type_spec.SerializeAsString(), builtin_types_.int32_ty});
  TypeSpec int_type_spec;
  int_type_spec.set_name("Int");
  builtin_types_map_.insert(
      {int_type_spec.SerializeAsString(), builtin_types_.int32_ty});

  TypeSpec bool_type_spec;
  bool_type_spec.set_name("Bool");
  ::llvm::Type* const bool_fields[] = {
    ::llvm::PointerType::getInt8PtrTy(ctx_),
    ::llvm::Type::getInt1Ty(ctx_),
  };
  builtin_types_.bool_ty = ::llvm::StructType::create(
      ctx_, bool_fields, bool_type_spec.name());
  builtin_types_map_.insert(
      {bool_type_spec.SerializeAsString(), builtin_types_.bool_ty});

  TypeSpec string_type_spec;
  int32_type_spec.set_name("String");
  ::llvm::Type* const string_fields[] = {
    ::llvm::Type::getInt8PtrTy(ctx_),
    ::llvm::Type::getInt32Ty(ctx_),
    ::llvm::Type::getInt8PtrTy(ctx_),
  };
  builtin_types_.string_ty = ::llvm::StructType::create(
      ctx_, string_fields, string_type_spec.name());
  builtin_types_map_.insert(
      {string_type_spec.SerializeAsString(), builtin_types_.string_ty});
}

void IRGenerator::SetupBuiltinFunctions(State* state) {
  state->builtin_fns_.quo_alloc = state->module->getOrInsertFunction(
      "__quo_alloc",
      ::llvm::FunctionType::get(
          ::llvm::Type::getInt8PtrTy(ctx_),
          { ::llvm::Type::getInt32Ty(ctx_) },
          false));  // isVarArg
  state->builtin_fns_.quo_free = state->module->getOrInsertFunction(
      "__quo_free",
      ::llvm::FunctionType::get(
          ::llvm::Type::getVoidTy(ctx_),
          { ::llvm::Type::getInt8PtrTy(ctx_) },
          false));  // isVarArg
  state->builtin_fns_.quo_copy = state->module->getOrInsertFunction(
      "__quo_copy",
      ::llvm::FunctionType::get(
          ::llvm::Type::getInt8PtrTy(ctx_),
          {
              ::llvm::Type::getInt8PtrTy(ctx_),
              ::llvm::Type::getInt8PtrTy(ctx_),
              ::llvm::Type::getInt32Ty(ctx_),
          },
          false));  // isVarArg
}

void IRGenerator::ProcessModuleMember(
    State* state, const ModuleDef::Member& member) {
  switch (member.type_case()) {
    case ModuleDef::Member::kFnDef:
      ProcessModuleFnDef(state, member.fn_def());
      break;
    default:
      LOG(FATAL) << "Unsupported module member type: " << member.type_case();
  }
}

void IRGenerator::ProcessModuleFnDef(
    State* state, const FnDef& fn_def) {
  state->fn_def = &fn_def;
  vector<::llvm::Type*> param_tys(fn_def.params_size());
  transform(
      fn_def.params().begin(),
      fn_def.params().end(),
      param_tys.begin(),
      [this](const FnParam& param) {
        return ::llvm::PointerType::getUnqual(LookupType(param.type_spec()));
      });
  ::llvm::FunctionType* fn_ty = ::llvm::FunctionType::get(
      ::llvm::PointerType::getUnqual(LookupType(fn_def.return_type_spec())),
      param_tys,
      false /* isVarArg */);
  state->fn_ty = fn_ty;
  ::llvm::Function* fn = ::llvm::Function::Create(
      fn_ty, ::llvm::Function::ExternalLinkage, fn_def.name(), state->module);
  int i = 0;
  for (::llvm::Argument& arg : fn->args()) {
    arg.setName(fn_def.params(i).name());
    ++i;
  }
  state->fn = fn;

  ::llvm::BasicBlock* bb = ::llvm::BasicBlock::Create(ctx_, "", fn);
  ::llvm::IRBuilder<> builder(ctx_);
  builder.SetInsertPoint(bb);
  state->ir_builder = &builder;

  // Add args to locals.
  state->locals.clear();
  for (::llvm::Argument& arg : fn->args()) {
    ::llvm::Value* arg_local = builder.CreateAlloca(
        arg.getType(), nullptr, arg.getName());
    state->locals.insert({ arg.getName(), arg_local });
    // TODO: Implement copy and move modes.
    builder.CreateStore(&arg, arg_local);
  }

  ProcessBlock(state, fn_def.block());

  if (fn_ty->getReturnType()->isVoidTy()) {
    builder.CreateRetVoid();
  }
}

void IRGenerator::ProcessBlock(State* state, const Block& block) {
  for (const Stmt& stmt : block.stmts()) {
    switch (stmt.type_case()) {
      case Stmt::kExpr:
        ProcessExpr(state, stmt.expr().expr());
        break;
      case Stmt::kRet:
        ProcessRetStmt(state, stmt.ret());
        break;
      default:
        LOG(FATAL) << "Unknown statement type:" << stmt.type_case();
    }
  }
}

void IRGenerator::ProcessRetStmt(State* state, const RetStmt& stmt) {
  ExprResult expr_result = ProcessExpr(state, stmt.expr());
  if (expr_result.address == nullptr) {
    ::llvm::Value* value_size = state->ir_builder->CreatePtrToInt(
        state->ir_builder->CreateGEP(
          ::llvm::ConstantPointerNull::get(
            ::llvm::PointerType::getUnqual(expr_result.value->getType())),
          ::llvm::ConstantInt::get(::llvm::Type::getInt32Ty(ctx_), 1)),
        ::llvm::Type::getInt32Ty(ctx_));
    expr_result.address = state->ir_builder->CreatePointerCast(
        state->ir_builder->CreateCall(
            state->builtin_fns_.quo_alloc, { value_size }),
        ::llvm::PointerType::getUnqual(expr_result.value->getType()));
    // TODO: Proper copying and memory management.
    state->ir_builder->CreateStore(expr_result.value, expr_result.address);
  }
  state->ir_builder->CreateRet(expr_result.address);
}

IRGenerator::ExprResult IRGenerator::ProcessExpr(
    State* state, const Expr& expr) {
  switch (expr.type_case()) {
    case Expr::kConstant:
      return ProcessConstantExpr(state, expr.constant());
    case Expr::kBinaryOp:
      return ProcessBinaryOpExpr(state, expr.binary_op());
    case Expr::kVar:
      return ProcessVarExpr(state, expr.var());
    default:
      LOG(FATAL) << "Unknown expression type:" << expr.type_case();
  }
}

IRGenerator::ExprResult IRGenerator::ProcessConstantExpr(
    State* state, const ConstantExpr& expr) {
  ExprResult result = { nullptr, nullptr };
  switch (expr.value_case()) {
    case ConstantExpr::kIntValue:
      result.value = CreateInt32Value(
          state,
          ::llvm::ConstantInt::getSigned(
              ::llvm::Type::getInt32Ty(ctx_), expr.intvalue()));
      break;
    case ConstantExpr::kBoolValue:
      result.value = CreateBoolValue(
          state,
          expr.boolvalue() ?
              ::llvm::ConstantInt::getTrue(ctx_) :
              ::llvm::ConstantInt::getFalse(ctx_));
      break;
    default:
      LOG(FATAL) << "Unknown constant type:" << expr.value_case();
  }
  return result;
}

IRGenerator::ExprResult IRGenerator::ProcessVarExpr(
    State* state, const VarExpr& expr) {
  ExprResult result = { nullptr, nullptr };

  auto it = state->locals.find(expr.name());
  if (it != state->locals.end()) {
    result.address = state->ir_builder->CreateLoad(
        it->second, it->first);
    result.value = state->ir_builder->CreateLoad(
        result.address);
    return result;
  }

  LOG(FATAL) << "Unknown variable: " << expr.name();
}

IRGenerator::ExprResult IRGenerator::ProcessBinaryOpExpr(
    State* state, const BinaryOpExpr& expr) {
  static const unordered_map<
      int,
      function<::llvm::Value*(::llvm::Value*, ::llvm::Value*)>> kIntOps = {
      {
          static_cast<int>(BinaryOpExpr::ADD),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateAdd(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::SUB),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateSub(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::MUL),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateMul(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::DIV),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateSDiv(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::MOD),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateSRem(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::EQ),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateICmpEQ(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::NE),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateICmpNE(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::GT),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateICmpSGT(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::GE),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateICmpSGE(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::LT),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateICmpSLT(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::LE),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateICmpSLE(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::AND),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateAnd(l, r);
          }
      },
      {
          static_cast<int>(BinaryOpExpr::OR),
          [state](::llvm::Value* l, ::llvm::Value* r) {
            return state->ir_builder->CreateOr(l, r);
          }
      },
  };

  ExprResult result = { nullptr, nullptr };
  ExprResult left_result = ProcessExpr(state, expr.left_expr());
  ExprResult right_result = ProcessExpr(state, expr.right_expr());
  switch (expr.op()) {
    case BinaryOpExpr::ADD:
    case BinaryOpExpr::SUB:
    case BinaryOpExpr::MUL:
    case BinaryOpExpr::DIV:
    case BinaryOpExpr::MOD:
      if (left_result.value->getType() == builtin_types_.int32_ty &&
          right_result.value->getType() == builtin_types_.int32_ty) {
        result.value = CreateInt32Value(
            state,
            kIntOps.at(expr.op())(
                ExtractInt32Value(state, left_result.value),
                ExtractInt32Value(state, right_result.value)));
      } else {
        LOG(FATAL) << "Incompatible types for binary operation " << expr.op();
      }
      break;
    case BinaryOpExpr::GT:
    case BinaryOpExpr::GE:
    case BinaryOpExpr::LT:
    case BinaryOpExpr::LE:
      if (left_result.value->getType() == builtin_types_.int32_ty &&
          right_result.value->getType() == builtin_types_.int32_ty) {
        result.value = CreateBoolValue(
            state,
            kIntOps.at(expr.op())(
                ExtractInt32Value(state, left_result.value),
                ExtractInt32Value(state, right_result.value)));
      } else {
        LOG(FATAL) << "Incompatible types for binary operation " << expr.op();
      }
      break;
    case BinaryOpExpr::EQ:
    case BinaryOpExpr::NE:
      if (left_result.value->getType() == builtin_types_.int32_ty &&
          right_result.value->getType() == builtin_types_.int32_ty) {
        result.value = CreateBoolValue(
            state,
            kIntOps.at(expr.op())(
                ExtractInt32Value(state, left_result.value),
                ExtractInt32Value(state, right_result.value)));
      } else if (left_result.value->getType() == builtin_types_.bool_ty &&
          right_result.value->getType() == builtin_types_.bool_ty) {
        result.value = CreateBoolValue(
            state,
            kIntOps.at(expr.op())(
                ExtractBoolValue(state, left_result.value),
                ExtractBoolValue(state, right_result.value)));
      } else {
        LOG(FATAL) << "Incompatible types for binary operation " << expr.op();
      }
      break;
    case BinaryOpExpr::AND:
    case BinaryOpExpr::OR:
      if (left_result.value->getType() == builtin_types_.bool_ty &&
          right_result.value->getType() == builtin_types_.bool_ty) {
        result.value = CreateBoolValue(
            state,
            kIntOps.at(expr.op())(
                ExtractBoolValue(state, left_result.value),
                ExtractBoolValue(state, right_result.value)));
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

::llvm::Value* IRGenerator::CreateInt32Value(
    State* state, ::llvm::Value* raw_int32_value) {
  ::llvm::Value* init_value = ::llvm::ConstantStruct::get(
      builtin_types_.int32_ty,
      ::llvm::ConstantPointerNull::get(
          ::llvm::Type::getInt8PtrTy(ctx_)),
      ::llvm::ConstantInt::getSigned(
          ::llvm::Type::getInt32Ty(ctx_),
          0),
      nullptr);
  return state->ir_builder->CreateInsertValue(
      init_value, raw_int32_value, {1});
}

::llvm::Value* IRGenerator::ExtractInt32Value(
    State* state, ::llvm::Value* wrapped_int32_value) {
  return state->ir_builder->CreateExtractValue(wrapped_int32_value, {1});
}

::llvm::Value* IRGenerator::CreateBoolValue(
    State* state, ::llvm::Value* raw_bool_value) {
  ::llvm::Value* init_value = ::llvm::ConstantStruct::get(
      builtin_types_.bool_ty,
      ::llvm::ConstantPointerNull::get(
          ::llvm::Type::getInt8PtrTy(ctx_)),
      ::llvm::ConstantInt::getSigned(
          ::llvm::Type::getInt1Ty(ctx_),
          0),
      nullptr);
  return state->ir_builder->CreateInsertValue(
      init_value, raw_bool_value, {1});
}

::llvm::Value* IRGenerator::ExtractBoolValue(
    State* state, ::llvm::Value* wrapped_bool_value) {
  return state->ir_builder->CreateExtractValue(wrapped_bool_value, {1});
}

}  // namespace quo

