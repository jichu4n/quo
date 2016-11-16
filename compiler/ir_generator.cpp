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
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glog/logging.h>
#include <llvm/Support/raw_ostream.h>

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

  struct {
    ::llvm::Constant* quo_alloc;
    ::llvm::Constant* quo_free;
    ::llvm::Constant* quo_copy;
  } builtin_fns;

  struct {
    ::llvm::GlobalVariable* int32_desc;
    ::llvm::GlobalVariable* bool_desc;
  } builtin_globals;
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
  SetupBuiltinGlobals(&state);
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
    ::llvm::Type::getInt8PtrTy(ctx_),
  };
  builtin_types_.string_ty = ::llvm::StructType::create(
      ctx_, string_fields, string_type_spec.name());
  builtin_types_map_.insert(
      {string_type_spec.SerializeAsString(), builtin_types_.string_ty});
}

void IRGenerator::SetupBuiltinGlobals(State* state) {
  state->builtin_globals.int32_desc = new ::llvm::GlobalVariable(
      *state->module,
      ::llvm::Type::getInt8Ty(ctx_),
      true,  // isConstant
      ::llvm::GlobalVariable::ExternalLinkage,
      nullptr,  // initializer,
      "__quo_Int32Descriptor",
      nullptr,  // insertBefore
      ::llvm::GlobalVariable::NotThreadLocal,
      0,  // address space
      true);  // isExternallyInitialized
  state->builtin_globals.bool_desc = new ::llvm::GlobalVariable(
      *state->module,
      ::llvm::Type::getInt8Ty(ctx_),
      true,  // isConstant
      ::llvm::GlobalVariable::ExternalLinkage,
      nullptr,  // initializer,
      "__quo_BoolDescriptor",
      nullptr,  // insertBefore
      ::llvm::GlobalVariable::NotThreadLocal,
      0,  // address space
      true);  // isExternallyInitialized
}

void IRGenerator::SetupBuiltinFunctions(State* state) {
  state->builtin_fns.quo_alloc = state->module->getOrInsertFunction(
      "__quo_alloc",
      ::llvm::FunctionType::get(
          ::llvm::Type::getInt8PtrTy(ctx_),
          { ::llvm::Type::getInt32Ty(ctx_) },
          false));  // isVarArg
  state->builtin_fns.quo_free = state->module->getOrInsertFunction(
      "__quo_free",
      ::llvm::FunctionType::get(
          ::llvm::Type::getVoidTy(ctx_),
          { ::llvm::Type::getInt8PtrTy(ctx_) },
          false));  // isVarArg
  state->builtin_fns.quo_copy = state->module->getOrInsertFunction(
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
  ::llvm::Type* const raw_return_ty = LookupType(fn_def.return_type_spec());
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
    arg.setName(fn_def.params(i).name());
    ++i;
  }
  state->fn = fn;

  ::llvm::BasicBlock* bb = ::llvm::BasicBlock::Create(ctx_, "", fn);
  ::llvm::IRBuilder<> builder(ctx_);
  builder.SetInsertPoint(bb);
  state->ir_builder = &builder;

  // Add args to scope.
  state->scopes.emplace_back();
  state->current_fn_scope_it = (++state->scopes.rbegin()).base();
  for (::llvm::Argument& arg : fn->args()) {
    ::llvm::Value* arg_value = builder.CreateLoad(&arg, arg.getName() + "_val");
    ::llvm::Value* arg_var = builder.CreateAlloca(
        arg.getType(), nullptr, arg.getName());
    builder.CreateStore(CreateObject(state, arg_value), arg_var);
    state->scopes.back().AddVar(arg.getName(), arg_var);
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
  ::llvm::Value* ret_address = CreateObject(state, expr_result.value);
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
  if (stmt.has_init_expr()) {
    ExprResult init_expr_result = ProcessExpr(state, stmt.init_expr());
    var = state->ir_builder->CreateAlloca(
        ::llvm::PointerType::getUnqual(
            init_expr_result.value->getType()), nullptr, stmt.name());
    state->ir_builder->CreateStore(
        CreateObject(state, init_expr_result.value), var);
  } else {
    ::llvm::Type* ty = LookupType(stmt.type_spec());
    var = state->ir_builder->CreateAlloca(
        ::llvm::PointerType::getUnqual(ty), nullptr, stmt.name());
    state->ir_builder->CreateStore(CreateObject(state, ty), var);
  }
  state->scopes.back().AddVar(stmt.name(), var);
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

  for (auto scope_it = state->scopes.rbegin();
      scope_it != state->scopes.rend();
      ++scope_it) {
    ::llvm::Value* var = scope_it->Lookup(expr.name());
    if (var != nullptr) {
      result.address = state->ir_builder->CreateLoad(
          var, expr.name() + "_addr");
      result.value = state->ir_builder->CreateLoad(
          result.address, expr.name() + "_val");
      return result;
    }
  }

  ::llvm::Function* fn = state->module->getFunction(expr.name());
  if (fn != nullptr) {
    result.value = fn;
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

IRGenerator::ExprResult IRGenerator::ProcessCallExpr(
    State* state, const CallExpr& expr) {
  ExprResult result = { nullptr, nullptr };
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
  result.address = state->ir_builder->CreateCall(
      fn_result.value,
      arg_results);
  result.value = state->ir_builder->CreateLoad(result.address);
  state->scopes.back().AddTemp(result.address);
  return result;
}

IRGenerator::ExprResult IRGenerator::ProcessAssignExpr(
    State* state, const AssignExpr& expr) {
  ExprResult dest_result = ProcessExpr(state, expr.dest_expr());
  if (dest_result.address == nullptr) {
    LOG(FATAL) << "Cannot assign to expression: "
               << expr.dest_expr().ShortDebugString();
  }
  ExprResult value_result = ProcessExpr(state, expr.value_expr());
  state->ir_builder->CreateStore(value_result.value, dest_result.address);
  return { value_result.value, dest_result.address };
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
      state->builtin_globals.int32_desc,
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
      state->builtin_globals.bool_desc,
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

void IRGenerator::EnsureAddress(
    IRGenerator::State* state, IRGenerator::ExprResult* result) {
  if (result->address != nullptr) {
    return;
  }
  result->address = CreateObject(state, result->value);
  result->address->setName("temp");
  state->scopes.back().AddTemp(result->address);
}

::llvm::Value* IRGenerator::CreateObject(State* state, ::llvm::Type* ty) {
  ::llvm::Value* value_size = state->ir_builder->CreatePtrToInt(
      state->ir_builder->CreateGEP(
        ::llvm::ConstantPointerNull::get(
          ::llvm::PointerType::getUnqual(ty)),
        ::llvm::ConstantInt::get(::llvm::Type::getInt32Ty(ctx_), 1)),
      ::llvm::Type::getInt32Ty(ctx_));
  ::llvm::Value* p = state->ir_builder->CreatePointerCast(
      state->ir_builder->CreateCall(
          state->builtin_fns.quo_alloc, { value_size }),
      ::llvm::PointerType::getUnqual(ty));
  return p;
}

::llvm::Value* IRGenerator::CreateObject(
    IRGenerator::State* state, ::llvm::Value* init_value) {
  ::llvm::Value* p = CreateObject(state, init_value->getType());
  state->ir_builder->CreateStore(init_value, p);
  return p;
}

void IRGenerator::DestroyTemps(State* state) {
  Scope& scope = state->scopes.back();
  for (auto it = scope.temps.rbegin(); it != scope.temps.rend(); ++it) {
    state->ir_builder->CreateCall(
        state->builtin_fns.quo_free,
        {
            state->ir_builder->CreateBitCast(
                *it, ::llvm::Type::getInt8PtrTy(ctx_)),
        });
  }
}

void IRGenerator::DestroyScope(State* state, Scope* scope) {
  for (auto it = scope->vars.rbegin(); it != scope->vars.rend(); ++it) {
    const string& var_name = *CHECK_NOTNULL(scope->Lookup(*it));
    state->ir_builder->CreateCall(
        state->builtin_fns.quo_free,
        {
            state->ir_builder->CreateBitCast(
                state->ir_builder->CreateLoad(
                    *it,
                    var_name + "_addr"),
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


void IRGenerator::Scope::AddVar(
    const ::std::string& name, ::llvm::Value* address) {
  vars.push_back(address);
  vars_by_name.insert({ name, address});
  vars_by_address.insert({ address, name });
}

void IRGenerator::Scope::AddTemp(::llvm::Value* address) {
  temps.push_back(address);
}

::llvm::Value* IRGenerator::Scope::Lookup(const ::std::string& name) {
  auto it = vars_by_name.find(name);
  return it == vars_by_name.end() ? nullptr : it->second;
}

const ::std::string* IRGenerator::Scope::Lookup(::llvm::Value* address) {
  auto it = vars_by_address.find(address);
  return it == vars_by_address.end() ? nullptr : &(it->second);
}

}  // namespace quo

