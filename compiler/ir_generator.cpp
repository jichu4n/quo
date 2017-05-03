/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2016-2017 Chuan Ji <ji@chu4n.com>                          *
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
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>
#include "glog/logging.h"
#include "llvm/Support/raw_ostream.h"
#include "compiler/builtins.hpp"
#include "compiler/symbols.hpp"

namespace quo {

using ::std::bind;
using ::std::function;
using ::std::list;
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
  list<Scope> scopes;
  list<Scope>::iterator current_fn_scope_it;
  ::std::unordered_map<::std::string, const FnDef*> fn_defs_by_name;
  unique_ptr<const Builtins> builtins;
};

IRGenerator::IRGenerator() {}

unique_ptr<::llvm::Module> IRGenerator::ProcessModule(
    const ModuleDef& module_def) {
  unique_ptr<::llvm::Module> module(
      new ::llvm::Module(module_def.name(), ctx_));
  State state;
  state.module = module.get();
  state.module_def = &module_def;
  state.builtins = Builtins::Create(state.module);
  for (const auto& member : module_def.members()) {
    // TODO(cji): Handle extern fns.
    if (member.type_case() != ModuleDef::Member::kFnDef) {
      continue;
    }
    const FnDef* fn_def = &member.fn_def();
    state.fn_defs_by_name.insert({ fn_def->name(), fn_def });
  }
  for (const auto& member : module_def.members()) {
    ProcessModuleMember(&state, member);
  }
  return module;
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
      [this, state](const FnParam& param) {
        return ::llvm::PointerType::getUnqual(
            state->builtins->LookupType(param.type_spec()));
      });
  ::llvm::Type* const raw_return_ty =
      state->builtins->LookupType(fn_def.return_type_spec());
  ::llvm::Type* const return_ty = raw_return_ty->isVoidTy() ?
      raw_return_ty :
      ::llvm::PointerType::getUnqual(raw_return_ty);
  ::llvm::FunctionType* fn_ty = ::llvm::FunctionType::get(
      return_ty,
      param_tys,
      false /* isVarArg */);
  state->fn_ty = fn_ty;
  ::llvm::Function* fn = ::llvm::Function::Create(
      fn_ty, ::llvm::Function::ExternalLinkage, fn_def.name(), state->module);
  int i = 0;
  for (::llvm::Argument& arg : fn->args()) {
    arg.setName(fn_def.params(i++).name());
  }
  state->fn = fn;

  ::llvm::BasicBlock* bb = ::llvm::BasicBlock::Create(ctx_, "", fn);
  ::llvm::IRBuilder<> builder(ctx_);
  builder.SetInsertPoint(bb);
  state->ir_builder = &builder;

  // Add args to scope.
  state->scopes.emplace_back();
  state->current_fn_scope_it = (++state->scopes.rbegin()).base();
  vector<::llvm::Argument*> args;
  i = 0;
  for (::llvm::Argument& arg : fn->args()) {
    const TypeSpec& type_spec = fn_def.params(i++).type_spec();
    ::llvm::Value* arg_var = builder.CreateAlloca(
        arg.getType(), nullptr, arg.getName());
    // Store args in scope and increment ref counts.
    builder.CreateStore(&arg, arg_var);
    state->ir_builder->CreateCall(
        state->builtins->fns.quo_inc_refs,
        {
            state->ir_builder->CreateBitCast(
                &arg, ::llvm::Type::getInt8PtrTy(ctx_)),
        });
    state->scopes.back().AddVar({ arg.getName(), type_spec, arg_var});
    args.push_back(&arg);
  }

  ProcessBlock(state, fn_def.block(), true);

  if (fn_ty->getReturnType()->isVoidTy()) {
    builder.CreateRetVoid();
  } else {
    builder.CreateUnreachable();
  }
}

void IRGenerator::ProcessBlock(
    State* state, const Block& block, bool manage_parent_scope) {
  if (!manage_parent_scope) {
    state->scopes.emplace_back();
  }
  for (const Stmt& stmt : block.stmts()) {
    switch (stmt.type_case()) {
      case Stmt::kExpr:
        ProcessExpr(state, stmt.expr().expr());
        break;
      case Stmt::kRet:
        ProcessRetStmt(state, stmt.ret());
        break;
      case Stmt::kCond:
        ProcessCondStmt(state, stmt.cond());
        break;
      case Stmt::kVarDecl:
        ProcessVarDeclStmt(state, stmt.var_decl());
        break;
      default:
        LOG(FATAL) << "Unknown statement type:" << stmt.type_case();
    }
    if (state->ir_builder->GetInsertBlock()->getTerminator() == nullptr) {
      DestroyTemps(state);
    }
    state->scopes.back().temps.clear();
  }
  if (state->ir_builder->GetInsertBlock()->getTerminator() == nullptr) {
    DestroyFnScopes(state);
  }
  state->scopes.pop_back();
}

void IRGenerator::ProcessRetStmt(State* state, const RetStmt& stmt) {
  ExprResult expr_result = ProcessExpr(state, stmt.expr());
  EnsureAddress(state, &expr_result);
  ::llvm::Value* ret_address = expr_result.address;
  state->ir_builder->CreateCall(
      state->builtins->fns.quo_inc_refs,
      {
          state->ir_builder->CreateBitCast(
              ret_address, ::llvm::Type::getInt8PtrTy(ctx_)),
      });
  DestroyTemps(state);
  DestroyFnScopes(state);
  state->ir_builder->CreateRet(ret_address);
}

void IRGenerator::ProcessCondStmt(State* state, const CondStmt& stmt) {
  ExprResult cond_expr_result = ProcessExpr(state, stmt.cond_expr());
  ::llvm::BasicBlock* true_bb = ::llvm::BasicBlock::Create(
      ctx_, "if", state->fn);
  ::llvm::BasicBlock* false_bb = ::llvm::BasicBlock::Create(ctx_, "else");
  ::llvm::BasicBlock* merge_bb = ::llvm::BasicBlock::Create(ctx_, "endif");

  state->ir_builder->CreateCondBr(
      ExtractBoolValue(state, cond_expr_result.value), true_bb, false_bb);

  state->ir_builder->SetInsertPoint(true_bb);
  ProcessBlock(state, stmt.true_block());
  if (state->ir_builder->GetInsertBlock()->getTerminator() == nullptr) {
    state->ir_builder->CreateBr(merge_bb);
  }

  false_bb->insertInto(state->fn);
  state->ir_builder->SetInsertPoint(false_bb);
  ProcessBlock(state, stmt.false_block());
  if (state->ir_builder->GetInsertBlock()->getTerminator() == nullptr) {
    state->ir_builder->CreateBr(merge_bb);
  }

  merge_bb->insertInto(state->fn);
  state->ir_builder->SetInsertPoint(merge_bb);
}

void IRGenerator::ProcessVarDeclStmt(State* state, const VarDeclStmt& stmt) {
  ::llvm::Value* var;
  TypeSpec type_spec;
  ::llvm::Type* ty;
  ExprResult init_expr_result;
  if (stmt.has_init_expr()) {
    init_expr_result = ProcessExpr(state, stmt.init_expr());
    if (stmt.has_type_spec() &&
        stmt.type_spec().SerializeAsString() !=
        init_expr_result.type_spec.SerializeAsString()) {
      LOG(FATAL) << "Invalid init expr in var decl: " << stmt.DebugString();
    }
    EnsureAddress(state, &init_expr_result);
    type_spec = init_expr_result.type_spec;
    ty = init_expr_result.value->getType();
  } else {
    if (!stmt.has_type_spec()) {
      LOG(FATAL) << "Missing type spec in var decl: " << stmt.DebugString();
    }
    type_spec = stmt.type_spec();
    ty = state->builtins->LookupType(type_spec);
  }
  var = state->ir_builder->CreateAlloca(
      ::llvm::PointerType::getUnqual(ty), nullptr, stmt.name());
  state->ir_builder->CreateStore(
      ::llvm::ConstantPointerNull::get(::llvm::PointerType::getUnqual(ty)),
      var);
  if (stmt.has_init_expr()) {
    AssignObject(state, type_spec, var, init_expr_result.address);
  }
  state->scopes.back().AddVar({ stmt.name(), type_spec, var });
}

IRGenerator::ExprResult IRGenerator::ProcessExpr(
    State* state, const Expr& expr) {
  switch (expr.type_case()) {
    case Expr::kConstant:
      return ProcessConstantExpr(state, expr.constant());
    case Expr::kVar:
      return ProcessVarExpr(state, expr.var());
    case Expr::kCall:
      return ProcessCallExpr(state, expr.call());
    case Expr::kBinaryOp:
      return ProcessBinaryOpExpr(state, expr.binary_op());
    case Expr::kAssign:
      return ProcessAssignExpr(state, expr.assign());
    default:
      LOG(FATAL) << "Unknown expression type:" << expr.type_case();
  }
}

IRGenerator::ExprResult IRGenerator::ProcessConstantExpr(
    State* state, const ConstantExpr& expr) {
  ExprResult result = { {}, nullptr, nullptr, nullptr, nullptr };
  switch (expr.value_case()) {
    case ConstantExpr::kIntValue:
      result.type_spec = state->builtins->types.int32_type.type_spec;
      result.value = CreateInt32Value(
          state,
          ::llvm::ConstantInt::getSigned(
              ::llvm::Type::getInt32Ty(ctx_), expr.intvalue()));
      break;
    case ConstantExpr::kBoolValue:
      result.type_spec = state->builtins->types.bool_type.type_spec;
      result.value = CreateBoolValue(
          state,
          expr.boolvalue() ?
              ::llvm::ConstantInt::getTrue(ctx_) :
              ::llvm::ConstantInt::getFalse(ctx_));
      break;
    case ConstantExpr::kStrValue: {
      result.type_spec = state->builtins->types.string_type.type_spec;
      ::llvm::Constant* array = ::llvm::ConstantDataArray::getString(
          ctx_,
          expr.strvalue(),
          false);  // addNull
      ::llvm::GlobalVariable* array_var = new ::llvm::GlobalVariable(
          *state->module,
          array->getType(),
          true,  // isConstant
          ::llvm::GlobalValue::PrivateLinkage,
          array);
      result.address = state->ir_builder->CreateCall(
          state->builtins->fns.quo_alloc_string,
          {
              state->ir_builder->CreateBitCast(
                  array_var,
                  ::llvm::Type::getInt8PtrTy(ctx_)),
              ::llvm::ConstantInt::getSigned(
                  ::llvm::Type::getInt32Ty(ctx_), expr.strvalue().size()),
          });
      state->scopes.back().AddTemp(result.address);
      result.value = state->ir_builder->CreateLoad(result.address);
      break;
    }
    default:
      LOG(FATAL) << "Unknown constant type:" << expr.value_case();
  }
  return result;
}

IRGenerator::ExprResult IRGenerator::ProcessVarExpr(
    State* state, const VarExpr& expr) {
  ExprResult result = { {}, nullptr, nullptr, nullptr, nullptr };

  for (auto scope_it = state->scopes.rbegin();
      scope_it != state->scopes.rend();
      ++scope_it) {
    const Var* var = scope_it->Lookup(expr.name());
    if (var != nullptr) {
      result.type_spec = var->type_spec;
      result.ref_address = var->ref_address;
      result.address = state->ir_builder->CreateLoad(
          var->ref_address, expr.name() + "_addr");
      result.value = state->ir_builder->CreateLoad(
          result.address, expr.name() + "_val");
      return result;
    }
  }

  ::llvm::Function* fn = state->module->getFunction(expr.name());
  const auto fn_def_it = state->fn_defs_by_name.find(expr.name());
  if (fn != nullptr && fn_def_it != state->fn_defs_by_name.end()) {
    result.fn_def = fn_def_it->second;
    result.value = fn;
    return result;
  }

  if (expr.name() == state->builtins->fns.quo_print_fn_def.name()) {
    result.fn_def = &state->builtins->fns.quo_print_fn_def;
    result.value = state->builtins->fns.quo_print;
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

  ExprResult result = { {}, nullptr, nullptr, nullptr, nullptr };
  ExprResult left_result = ProcessExpr(state, expr.left_expr());
  ExprResult right_result = ProcessExpr(state, expr.right_expr());
  switch (expr.op()) {
    case BinaryOpExpr::ADD:
      if (left_result.value->getType() ==
          state->builtins->types.string_type.ty &&
          right_result.value->getType() ==
          state->builtins->types.string_type.ty) {
        result.type_spec = state->builtins->types.string_type.type_spec;
        result.address = state->ir_builder->CreateCall(
            state->builtins->fns.quo_string_concat,
            { left_result.address, right_result.address });
        state->scopes.back().AddTemp(result.address);
        result.value = state->ir_builder->CreateLoad(result.address);
        break;
      }
      // fall through
    case BinaryOpExpr::SUB:
    case BinaryOpExpr::MUL:
    case BinaryOpExpr::DIV:
    case BinaryOpExpr::MOD:
      if (left_result.value->getType() ==
          state->builtins->types.int32_type.ty &&
          right_result.value->getType() ==
          state->builtins->types.int32_type.ty) {
        result.type_spec = state->builtins->types.int32_type.type_spec;
        result.value = CreateInt32Value(
            state,
            kIntOps.at(expr.op())(
                ExtractInt32Value(state, left_result.value),
                ExtractInt32Value(state, right_result.value)));
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
          state->builtins->types.int32_type.ty &&
          right_result.value->getType() ==
          state->builtins->types.int32_type.ty) {
        result.type_spec = state->builtins->types.bool_type.type_spec;
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
      if (left_result.value->getType() ==
          state->builtins->types.int32_type.ty &&
          right_result.value->getType() ==
          state->builtins->types.int32_type.ty) {
        result.type_spec = state->builtins->types.bool_type.type_spec;
        result.value = CreateBoolValue(
            state,
            kIntOps.at(expr.op())(
                ExtractInt32Value(state, left_result.value),
                ExtractInt32Value(state, right_result.value)));
      } else if (left_result.value->getType() ==
          state->builtins->types.bool_type.ty &&
          right_result.value->getType() ==
          state->builtins->types.bool_type.ty) {
        result.type_spec = state->builtins->types.bool_type.type_spec;
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
      if (left_result.value->getType() ==
          state->builtins->types.bool_type.ty &&
          right_result.value->getType() ==
          state->builtins->types.bool_type.ty) {
        result.type_spec = state->builtins->types.bool_type.type_spec;
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

IRGenerator::ExprResult IRGenerator::ProcessCallExpr(
    State* state, const CallExpr& expr) {
  ExprResult result = { {}, nullptr, nullptr, nullptr, nullptr };
  ExprResult fn_result = ProcessExpr(state, expr.fn_expr());
  vector<::llvm::Value*> arg_results(expr.arg_exprs_size());
  transform(
      expr.arg_exprs().begin(),
      expr.arg_exprs().end(),
      arg_results.begin(),
      [this, state](Expr e) {
        ExprResult arg_result = ProcessExpr(state, e);
        EnsureAddress(state, &arg_result);
        return arg_result.address;
      });
  result.type_spec = CHECK_NOTNULL(fn_result.fn_def)->return_type_spec();
  result.address = state->ir_builder->CreateCall(fn_result.value, arg_results);
  result.value = state->ir_builder->CreateLoad(result.address);
  state->scopes.back().AddTemp(result.address);
  return result;
}

IRGenerator::ExprResult IRGenerator::ProcessAssignExpr(
    State* state, const AssignExpr& expr) {
  ExprResult dest_result = ProcessExpr(state, expr.dest_expr());
  if (dest_result.address == nullptr || dest_result.ref_address == nullptr) {
    LOG(FATAL) << "Cannot assign to expression: "
               << expr.dest_expr().ShortDebugString();
  }
  ExprResult value_result = ProcessExpr(state, expr.value_expr());
  EnsureAddress(state, &value_result);
  ::llvm::Value* dest_address = AssignObject(
      state,
      dest_result.type_spec,
      dest_result.ref_address,
      value_result.address);
  return {
      dest_result.type_spec,
      state->ir_builder->CreateLoad(dest_address),
      dest_address,
      dest_result.ref_address,
      nullptr,
  };
}

::llvm::Value* IRGenerator::CreateInt32Value(
    State* state, ::llvm::Value* raw_int32_value) {
  ::llvm::Value* init_value = ::llvm::ConstantStruct::get(
      state->builtins->types.int32_type.ty,
      ::llvm::ConstantStruct::get(
        state->builtins->types.object_type.ty,
        state->builtins->globals.int32_desc,
        ::llvm::ConstantInt::getSigned(::llvm::Type::getInt32Ty(ctx_), 1),
        nullptr),
      ::llvm::ConstantInt::getSigned(::llvm::Type::getInt32Ty(ctx_), 0),
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
      state->builtins->types.bool_type.ty,
      ::llvm::ConstantStruct::get(
        state->builtins->types.object_type.ty,
        state->builtins->globals.bool_desc,
        ::llvm::ConstantInt::getSigned(::llvm::Type::getInt32Ty(ctx_), 1),
        nullptr),
      ::llvm::ConstantInt::getSigned(::llvm::Type::getInt1Ty(ctx_), 0),
      nullptr);
  return state->ir_builder->CreateInsertValue(
      init_value, raw_bool_value, {1});
}

::llvm::Value* IRGenerator::ExtractBoolValue(
    State* state, ::llvm::Value* wrapped_bool_value) {
  return state->ir_builder->CreateExtractValue(wrapped_bool_value, {1});
}

void IRGenerator::EnsureAddress(
    IRGenerator::State* state, IRGenerator::ExprResult* result) {
  if (result->address != nullptr) {
    return;
  }
  result->address = CreateObject(state, result->type_spec, result->value);
  result->address->setName("temp");
  state->scopes.back().AddTemp(result->address);
}

::llvm::Value* IRGenerator::CreateObject(
    State* state, const TypeSpec& type_spec) {
  ::llvm::Type* const ty = state->builtins->LookupType(type_spec);
  ::llvm::Value* const desc =
      state->builtins->LookupDescriptor(type_spec);
  ::llvm::Value* value_size = state->ir_builder->CreatePtrToInt(
      state->ir_builder->CreateGEP(
        ::llvm::ConstantPointerNull::get(
          ::llvm::PointerType::getUnqual(ty)),
        ::llvm::ConstantInt::get(::llvm::Type::getInt32Ty(ctx_), 1)),
      ::llvm::Type::getInt32Ty(ctx_));
  ::llvm::Value* p = state->ir_builder->CreatePointerCast(
      state->ir_builder->CreateCall(
          state->builtins->fns.quo_alloc, { desc, value_size }),
      ::llvm::PointerType::getUnqual(ty));
  return p;
}

::llvm::Value* IRGenerator::CreateObject(
    IRGenerator::State* state,
    const TypeSpec& type_spec,
    ::llvm::Value* init_value) {
  ::llvm::Value* p = CreateObject(state, type_spec);
  state->ir_builder->CreateStore(init_value, p);
  return p;
}

::llvm::Value* IRGenerator::AssignObject(
    IRGenerator::State* state,
    const TypeSpec& type_spec,
    ::llvm::Value* dest_ref_address,
    ::llvm::Value* src_address) {
  state->ir_builder->CreateCall(
      state->builtins->fns.quo_assign,
      {
          state->ir_builder->CreateBitCast(
              dest_ref_address,
              ::llvm::PointerType::getUnqual(
                ::llvm::Type::getInt8PtrTy(ctx_))),
          state->ir_builder->CreateBitCast(
              src_address, ::llvm::Type::getInt8PtrTy(ctx_)),
      });
  return src_address;
}

::llvm::Value* IRGenerator::CloneObject(
    IRGenerator::State* state,
    const TypeSpec& type_spec,
    ::llvm::Value* src_address) {
  return AssignObject(
      state, type_spec, CreateObject(state, type_spec), src_address);
}

void IRGenerator::DestroyTemps(State* state) {
  Scope& scope = state->scopes.back();
  for (auto it = scope.temps.rbegin(); it != scope.temps.rend(); ++it) {
    state->ir_builder->CreateCall(
        state->builtins->fns.quo_dec_refs,
        {
            state->ir_builder->CreateBitCast(
                *it, ::llvm::Type::getInt8PtrTy(ctx_)),
        });
  }
}

void IRGenerator::DestroyScope(State* state, Scope* scope) {
  for (auto it = scope->vars.rbegin(); it != scope->vars.rend(); ++it) {
    state->ir_builder->CreateCall(
        state->builtins->fns.quo_dec_refs,
        {
            state->ir_builder->CreateBitCast(
                state->ir_builder->CreateLoad(
                    it->ref_address,
                    it->name + "_addr"),
                ::llvm::Type::getInt8PtrTy(ctx_)),
        });
  }
}

void IRGenerator::DestroyFnScopes(State* state) {
  auto scope_it = state->scopes.rbegin();
  do {
    DestroyScope(state, &(*scope_it));
  } while ((++scope_it).base() != state->current_fn_scope_it);
}

}  // namespace quo

