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
#include "compiler/expr_ir_generator.hpp"
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
  unique_ptr<const Builtins> builtins;
  unique_ptr<Symbols> symbols;
  unique_ptr<ExprIRGenerator> expr_ir_generator;
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
  state.symbols = Symbols::Create(module_def);
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
  state->expr_ir_generator.reset(
      new ExprIRGenerator(
          state->module,
          state->ir_builder,
          state->builtins.get(),
          state->symbols.get()));

  // Add args to scope.
  Scope* fn_scope = state->symbols->PushScope();
  fn_scope->is_fn_scope = true;
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
    state->symbols->GetScope()->AddVar({ arg.getName(), type_spec, arg_var});
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
    state->symbols->PushScope();
  }
  for (const Stmt& stmt : block.stmts()) {
    switch (stmt.type_case()) {
      case Stmt::kExpr:
        state->expr_ir_generator->ProcessExpr(stmt.expr().expr());
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
    state->symbols->GetScope()->temps.clear();
  }
  if (state->ir_builder->GetInsertBlock()->getTerminator() == nullptr) {
    DestroyFnScopes(state);
  }
  state->symbols->PopScope();
}

void IRGenerator::ProcessRetStmt(State* state, const RetStmt& stmt) {
  ExprResult expr_result = state->expr_ir_generator->ProcessExpr(stmt.expr());
  state->expr_ir_generator->EnsureAddress(&expr_result);
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
  ExprResult cond_expr_result =
      state->expr_ir_generator->ProcessExpr(stmt.cond_expr());
  ::llvm::BasicBlock* true_bb = ::llvm::BasicBlock::Create(
      ctx_, "if", state->fn);
  ::llvm::BasicBlock* false_bb = ::llvm::BasicBlock::Create(ctx_, "else");
  ::llvm::BasicBlock* merge_bb = ::llvm::BasicBlock::Create(ctx_, "endif");

  state->ir_builder->CreateCondBr(
      state->expr_ir_generator->ExtractBoolValue(
          cond_expr_result.value), true_bb, false_bb);

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
    init_expr_result = state->expr_ir_generator->ProcessExpr(stmt.init_expr());
    if (stmt.has_type_spec() &&
        stmt.type_spec().SerializeAsString() !=
        init_expr_result.type_spec.SerializeAsString()) {
      LOG(FATAL) << "Invalid init expr in var decl: " << stmt.DebugString();
    }
    state->expr_ir_generator->EnsureAddress(&init_expr_result);
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
    state->expr_ir_generator->AssignObject(
        type_spec, var, init_expr_result.address);
  }
  state->symbols->GetScope()->AddVar({ stmt.name(), type_spec, var });
}

void IRGenerator::DestroyTemps(State* state) {
  Scope* scope = state->symbols->GetScope();
  for (auto it = scope->temps.rbegin(); it != scope->temps.rend(); ++it) {
    state->ir_builder->CreateCall(
        state->builtins->fns.quo_dec_refs,
        {
            state->ir_builder->CreateBitCast(
                *it, ::llvm::Type::getInt8PtrTy(ctx_)),
        });
  }
}

void IRGenerator::DestroyFnScopes(State* state) {
  for (Scope* scope : state->symbols->GetScopesInFn()) {
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
}

}  // namespace quo

