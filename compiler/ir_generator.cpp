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
#include "compiler/exceptions.hpp"

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
  symbols_ = Symbols::Create(module_, builtins_.get(), module_def);
  for (const auto& member : module_def.members()) {
    ProcessModuleMember(member);
  }
  return module;
}

void IRGenerator::ProcessModuleMember(const ModuleDef::Member& member) {
  try {
    switch (member.type_case()) {
      case ModuleDef::Member::kFnDef:
        ProcessModuleFnDef(member.fn_def());
        break;
      case ModuleDef::Member::kClassDef:
        break;
      default:
        throw Exception(
            "Unsupported module member type: %d",
            member.type_case());
    }
  } catch (const Exception& e) {
    throw e.withDefault(member.line());
  }
}

void IRGenerator::ProcessModuleFnDef(const FnDef& fn_def) {
  const FnType* fn_type = symbols_->LookupFnOrThrow(fn_def.name());
  fn_def_ = &fn_def;
  fn_ty_ = fn_type->fn_ty;
  fn_ = fn_type->fn;

  ::llvm::BasicBlock* bb = ::llvm::BasicBlock::Create(ctx_, "", fn_);
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
  int i = 0;
  for (::llvm::Argument& arg : fn_->args()) {
    const FnParam& param = fn_def.params(i++);
    ::llvm::Value* arg_var = builder.CreateAlloca(
        arg.getType(), nullptr, arg.getName());
    // Store args in scope and increment ref counts.
    builder.CreateStore(&arg, arg_var);
    if (param.ref_mode() == STRONG_REF) {
      ir_builder_->CreateCall(
          builtins_->fns.quo_inc_refs,
          {
              ir_builder_->CreateBitCast(
                  &arg, ::llvm::Type::getInt8PtrTy(ctx_)),
          });
    }
    symbols_->GetScope()->AddVar({
        arg.getName(),
        param.type_spec(),
        arg_var,
        param.ref_mode(),
    });
    args.push_back(&arg);
  }

  ProcessBlock(fn_def.block(), true);

  if (fn_ty_->getReturnType()->isVoidTy()) {
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
    try {
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
          throw Exception("Unknown statement type: %d", stmt.type_case());
      }
    } catch (const Exception& e) {
      throw e.withDefault(stmt.line());
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
  Var var;
  var.name = stmt.name();
  var.ref_mode = stmt.ref_mode();
  if (stmt.has_init_expr()) {
    AssignExpr assign_expr;
    assign_expr.mutable_value_expr()->CopyFrom(stmt.init_expr());
    ExprResult init_expr_result =
        expr_ir_generator_->ProcessAssignExpr(assign_expr, &var);
    if (stmt.has_type_spec() &&
        stmt.type_spec().SerializeAsString() !=
        init_expr_result.type_spec.SerializeAsString()) {
      throw Exception(
          "Invalid init expr type in var decl: %s",
          stmt.DebugString().c_str());
    }
    var.type_spec = init_expr_result.type_spec;
    var.ref_address = init_expr_result.ref_address;
  } else {
    if (!stmt.has_type_spec()) {
      throw Exception(
          "Missing type spec in var decl: %s",
          stmt.DebugString().c_str());
    }
    var.type_spec = stmt.type_spec();
    var.ref_address = expr_ir_generator_->CreateLocalVar(
        symbols_->LookupTypeOrDie(var.type_spec)->ty, stmt.name());
  }
  symbols_->GetScope()->AddVar(var);
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
      if (it->ref_mode == STRONG_REF) {
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
}

}  // namespace quo

