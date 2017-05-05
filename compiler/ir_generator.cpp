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

namespace quo {

using ::std::bind;
using ::std::function;
using ::std::list;
using ::std::string;
using ::std::transform;
using ::std::unordered_map;
using ::std::unique_ptr;
using ::std::vector;

IRGenerator::IRGenerator() {}

unique_ptr<::llvm::Module> IRGenerator::ProcessModule(
    const ModuleDef& module_def) {
  unique_ptr<::llvm::Module> module(
      new ::llvm::Module(module_def.name(), ctx_));
  module_ = module.get();
  module_def_ = &module_def;
  builtins_ = Builtins::Create(module_);
  symbols_ = Symbols::Create(module_def);
  for (const auto& member : module_def.members()) {
    ProcessModuleMember(member);
  }
  return module;
}

void IRGenerator::ProcessModuleMember(const ModuleDef::Member& member) {
  switch (member.type_case()) {
    case ModuleDef::Member::kFnDef:
      ProcessModuleFnDef(member.fn_def());
      break;
    default:
      LOG(FATAL) << "Unsupported module member type: " << member.type_case();
  }
}

void IRGenerator::ProcessModuleFnDef(const FnDef& fn_def) {
  fn_def_ = &fn_def;
  vector<::llvm::Type*> param_tys(fn_def.params_size());
  transform(
      fn_def.params().begin(),
      fn_def.params().end(),
      param_tys.begin(),
      [this](const FnParam& param) {
        return ::llvm::PointerType::getUnqual(
            builtins_->LookupType(param.type_spec()));
      });
  ::llvm::Type* const raw_return_ty =
      builtins_->LookupType(fn_def.return_type_spec());
  ::llvm::Type* const return_ty = raw_return_ty->isVoidTy() ?
      raw_return_ty :
      ::llvm::PointerType::getUnqual(raw_return_ty);
  ::llvm::FunctionType* fn_ty = ::llvm::FunctionType::get(
      return_ty,
      param_tys,
      false /* isVarArg */);
  fn_ty_ = fn_ty;
  ::llvm::Function* fn = ::llvm::Function::Create(
      fn_ty, ::llvm::Function::ExternalLinkage, fn_def.name(), module_);
  int i = 0;
  for (::llvm::Argument& arg : fn->args()) {
    arg.setName(fn_def.params(i++).name());
  }
  fn_ = fn;

  ::llvm::BasicBlock* bb = ::llvm::BasicBlock::Create(ctx_, "", fn);
  ::llvm::IRBuilder<> builder(ctx_);
  builder.SetInsertPoint(bb);
  ir_builder_ = &builder;
  expr_ir_generator_.reset(
      new ExprIRGenerator(
          module_,
          ir_builder_,
          builtins_.get(),
          symbols_.get()));

  // Add args to scope.
  Scope* fn_scope = symbols_->PushScope();
  fn_scope->is_fn_scope = true;
  vector<::llvm::Argument*> args;
  i = 0;
  for (::llvm::Argument& arg : fn->args()) {
    const TypeSpec& type_spec = fn_def.params(i++).type_spec();
    ::llvm::Value* arg_var = builder.CreateAlloca(
        arg.getType(), nullptr, arg.getName());
    // Store args in scope and increment ref counts.
    builder.CreateStore(&arg, arg_var);
    ir_builder_->CreateCall(
        builtins_->fns.quo_inc_refs,
        {
            ir_builder_->CreateBitCast(
                &arg, ::llvm::Type::getInt8PtrTy(ctx_)),
        });
    symbols_->GetScope()->AddVar({ arg.getName(), type_spec, arg_var});
    args.push_back(&arg);
  }

  ProcessBlock(fn_def.block(), true);

  if (fn_ty->getReturnType()->isVoidTy()) {
    builder.CreateRetVoid();
  } else {
    builder.CreateUnreachable();
  }
}

void IRGenerator::ProcessBlock(
    const Block& block, bool manage_parent_scope) {
  if (!manage_parent_scope) {
    symbols_->PushScope();
  }
  for (const Stmt& stmt : block.stmts()) {
    switch (stmt.type_case()) {
      case Stmt::kExpr:
        expr_ir_generator_->ProcessExpr(stmt.expr().expr());
        break;
      case Stmt::kRet:
        ProcessRetStmt(stmt.ret());
        break;
      case Stmt::kCond:
        ProcessCondStmt(stmt.cond());
        break;
      case Stmt::kVarDecl:
        ProcessVarDeclStmt(stmt.var_decl());
        break;
      default:
        LOG(FATAL) << "Unknown statement type:" << stmt.type_case();
    }
    if (ir_builder_->GetInsertBlock()->getTerminator() == nullptr) {
      DestroyTemps();
    }
    symbols_->GetScope()->temps.clear();
  }
  if (ir_builder_->GetInsertBlock()->getTerminator() == nullptr) {
    DestroyFnScopes();
  }
  symbols_->PopScope();
}

void IRGenerator::ProcessRetStmt(const RetStmt& stmt) {
  ExprResult expr_result = expr_ir_generator_->ProcessExpr(stmt.expr());
  expr_ir_generator_->EnsureAddress(&expr_result);
  ::llvm::Value* ret_address = expr_result.address;
  ir_builder_->CreateCall(
      builtins_->fns.quo_inc_refs,
      {
          ir_builder_->CreateBitCast(
              ret_address, ::llvm::Type::getInt8PtrTy(ctx_)),
      });
  DestroyTemps();
  DestroyFnScopes();
  ir_builder_->CreateRet(ret_address);
}

void IRGenerator::ProcessCondStmt(const CondStmt& stmt) {
  ExprResult cond_expr_result =
      expr_ir_generator_->ProcessExpr(stmt.cond_expr());
  ::llvm::BasicBlock* true_bb = ::llvm::BasicBlock::Create(
      ctx_, "if", fn_);
  ::llvm::BasicBlock* false_bb = ::llvm::BasicBlock::Create(ctx_, "else");
  ::llvm::BasicBlock* merge_bb = ::llvm::BasicBlock::Create(ctx_, "endif");

  ir_builder_->CreateCondBr(
      expr_ir_generator_->ExtractBoolValue(
          cond_expr_result.value), true_bb, false_bb);

  ir_builder_->SetInsertPoint(true_bb);
  ProcessBlock(stmt.true_block());
  if (ir_builder_->GetInsertBlock()->getTerminator() == nullptr) {
    ir_builder_->CreateBr(merge_bb);
  }

  false_bb->insertInto(fn_);
  ir_builder_->SetInsertPoint(false_bb);
  ProcessBlock(stmt.false_block());
  if (ir_builder_->GetInsertBlock()->getTerminator() == nullptr) {
    ir_builder_->CreateBr(merge_bb);
  }

  merge_bb->insertInto(fn_);
  ir_builder_->SetInsertPoint(merge_bb);
}

void IRGenerator::ProcessVarDeclStmt(const VarDeclStmt& stmt) {
  ::llvm::Value* var;
  TypeSpec type_spec;
  ::llvm::Type* ty;
  ExprResult init_expr_result;
  if (stmt.has_init_expr()) {
    init_expr_result = expr_ir_generator_->ProcessExpr(stmt.init_expr());
    if (stmt.has_type_spec() &&
        stmt.type_spec().SerializeAsString() !=
        init_expr_result.type_spec.SerializeAsString()) {
      LOG(FATAL) << "Invalid init expr in var decl: " << stmt.DebugString();
    }
    expr_ir_generator_->EnsureAddress(&init_expr_result);
    type_spec = init_expr_result.type_spec;
    ty = init_expr_result.value->getType();
  } else {
    if (!stmt.has_type_spec()) {
      LOG(FATAL) << "Missing type spec in var decl: " << stmt.DebugString();
    }
    type_spec = stmt.type_spec();
    ty = builtins_->LookupType(type_spec);
  }
  var = ir_builder_->CreateAlloca(
      ::llvm::PointerType::getUnqual(ty), nullptr, stmt.name());
  ir_builder_->CreateStore(
      ::llvm::ConstantPointerNull::get(::llvm::PointerType::getUnqual(ty)),
      var);
  if (stmt.has_init_expr()) {
    expr_ir_generator_->AssignObject(
        type_spec, var, init_expr_result.address);
  }
  symbols_->GetScope()->AddVar({ stmt.name(), type_spec, var });
}

void IRGenerator::DestroyTemps() {
  Scope* scope = symbols_->GetScope();
  for (auto it = scope->temps.rbegin(); it != scope->temps.rend(); ++it) {
    ir_builder_->CreateCall(
        builtins_->fns.quo_dec_refs,
        {
            ir_builder_->CreateBitCast(
                *it, ::llvm::Type::getInt8PtrTy(ctx_)),
        });
  }
}

void IRGenerator::DestroyFnScopes() {
  for (Scope* scope : symbols_->GetScopesInFn()) {
    for (auto it = scope->vars.rbegin(); it != scope->vars.rend(); ++it) {
      ir_builder_->CreateCall(
          builtins_->fns.quo_dec_refs,
          {
              ir_builder_->CreateBitCast(
                  ir_builder_->CreateLoad(
                      it->ref_address,
                      it->name + "_addr"),
                  ::llvm::Type::getInt8PtrTy(ctx_)),
          });
    }
  }
}

}  // namespace quo

