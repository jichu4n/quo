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
        ProcessFnDef(member.fn_def(), nullptr);
        break;
      case ModuleDef::Member::kClassDef:
        ProcessClassDef(member.class_def());
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

void IRGenerator::ProcessClassDef(const ClassDef& class_def) {
  for (const ClassDef::Member& member : class_def.members()) {
    if (member.type_case() == ClassDef::Member::kFnDef) {
      ProcessFnDef(member.fn_def(), &class_def);
    }
  }
}

void IRGenerator::ProcessFnDef(
    const FnDef& fn_def, const ClassDef* parent_class_def) {
  ClassType* parent_class_type;
  const FnType* fn_type;
  if (parent_class_def == nullptr) {
    parent_class_type = nullptr;
    fn_type = symbols_->LookupFnOrThrow(fn_def.name());
  } else {
    TypeSpec type_spec;
    type_spec.set_name(parent_class_def->name());
    parent_class_type = symbols_->LookupTypeOrDie(type_spec);
    fn_type = parent_class_type->LookupMethodOrThrow(fn_def.name());
  }
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

  // Object scope.
  Scope* object_scope;
  Scope* fn_scope;
  if (parent_class_def == nullptr) {
    object_scope = nullptr;
    fn_scope = symbols_->PushScope();
    fn_scope->is_fn_scope = true;
  } else {
    object_scope = symbols_->PushScope();
    object_scope->is_fn_scope = true;
    fn_scope = symbols_->PushScope();
  }

  // Add args to scope.
  vector<::llvm::Argument*> args;
  vector<FnParam> params;
  if (parent_class_def != nullptr) {
    FnParam this_param;
    this_param.set_name("this");
    this_param.set_ref_mode(STRONG_REF);
    this_param.mutable_type_spec()->set_name(parent_class_def->name());
    params.push_back(this_param);
  }
  params.insert(params.end(), fn_def.params().begin(), fn_def.params().end());
  int i = 0;
  for (::llvm::Argument& arg : fn_->args()) {
    const FnParam& param = params[i++];
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
    fn_scope->AddVar({
        arg.getName(),
        param.type_spec(),
        arg_var,
        param.ref_mode(),
    });
    args.push_back(&arg);
  }

  if (parent_class_def != nullptr) {
    for (const FieldType& field_type : parent_class_type->fields) {
      object_scope->AddVar({
          field_type.name,
          field_type.type_spec,
          expr_ir_generator_->GetFieldRefAddress(
              parent_class_type,
              &field_type,
              ir_builder_->CreateLoad(
                  fn_scope->vars.front().ref_address)),
          WEAK_REF,
      });
    }
  }

  ProcessBlock(fn_def.block(), true);
  if (parent_class_def != nullptr) {
    symbols_->PopScope();
  }

  if (fn_ty_->getReturnType()->isVoidTy()) {
    builder.CreateRetVoid();
  } else {
    builder.CreateUnreachable();
  }
}

void IRGenerator::ProcessBlock(
    const Block& block, bool is_fn_body) {
  if (!is_fn_body) {
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
        case Stmt::kBrk:
          ProcessBrkStmt(stmt.brk());
          break;
        case Stmt::kCond:
          ProcessCondStmt(stmt.cond());
          break;
        case Stmt::kCondLoop:
          ProcessCondLoopStmt(stmt.cond_loop());
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
  if (is_fn_body) {
    if (ir_builder_->GetInsertBlock()->getTerminator() == nullptr) {
      DestroyFnScopes();
    }
  } else {
    DestroyScope();
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

void IRGenerator::ProcessBrkStmt(const BrkStmt& stmt) {
  if (loop_end_bb_stack_.empty()) {
    throw Exception("Break statement with no corresponding loop");
  }
  DestroyTemps();
  DestroyScope();
  ir_builder_->CreateBr(loop_end_bb_stack_.top());
}

void IRGenerator::ProcessCondStmt(const CondStmt& stmt) {
  ExprResult cond_expr_result =
      expr_ir_generator_->ProcessExpr(stmt.cond_expr());
  ::llvm::BasicBlock* true_bb = ::llvm::BasicBlock::Create(
      ctx_, "if", fn_);
  ::llvm::BasicBlock* false_bb = ::llvm::BasicBlock::Create(ctx_, "else");
  ::llvm::BasicBlock* merge_bb = ::llvm::BasicBlock::Create(ctx_, "endif");

  ir_builder_->CreateCondBr(
      expr_ir_generator_->ExtractBoolValue(cond_expr_result.value),
      true_bb,
      false_bb);

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

void IRGenerator::ProcessCondLoopStmt(const CondLoopStmt& stmt) {
  ::llvm::BasicBlock* loop_cond_bb = ::llvm::BasicBlock::Create(
      ctx_, "while", fn_);
  ::llvm::BasicBlock* loop_body_bb = ::llvm::BasicBlock::Create(ctx_, "do");
  ::llvm::BasicBlock* loop_end_bb =
      ::llvm::BasicBlock::Create(ctx_, "endwhile");

  ir_builder_->CreateBr(loop_cond_bb);
  ir_builder_->SetInsertPoint(loop_cond_bb);
  ExprResult cond_expr_result =
      expr_ir_generator_->ProcessExpr(stmt.cond_expr());
  ir_builder_->CreateCondBr(
      expr_ir_generator_->ExtractBoolValue(cond_expr_result.value),
      loop_body_bb,
      loop_end_bb);

  loop_body_bb->insertInto(fn_);
  ir_builder_->SetInsertPoint(loop_body_bb);
  loop_end_bb_stack_.push(loop_end_bb);
  ProcessBlock(stmt.block());
  loop_end_bb_stack_.pop();
  if (ir_builder_->GetInsertBlock()->getTerminator() == nullptr) {
    ir_builder_->CreateBr(loop_cond_bb);
  }

  loop_end_bb->insertInto(fn_);
  ir_builder_->SetInsertPoint(loop_end_bb);
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

void IRGenerator::DestroyScope(Scope* scope) {
  if (scope == nullptr) {
    scope = symbols_->GetScope();
  }
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

void IRGenerator::DestroyFnScopes() {
  for (Scope* scope : symbols_->GetScopesInFn()) {
    DestroyScope(scope);
  }
}

}  // namespace quo

