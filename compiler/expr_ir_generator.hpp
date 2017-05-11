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

#ifndef EXPR_IR_GENERATOR_HPP_
#define EXPR_IR_GENERATOR_HPP_

#include <string>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "ast/ast.pb.h"

namespace quo {

class Builtins;
class ClassType;
class Symbols;
class Var;

// The result of IR generation for an expression. Only a subset of the fields
// will be set for any given expression.
struct ExprResult {
  // The underlying type of the expression, e.g. Int32
  TypeSpec type_spec;
  // The value of the expression (QObject).
  ::llvm::Value* value;
  // Address on the heap storing the value (QObject*). Only set if the value
  // is actually stored in memory (i.e. not an intermediate value).
  ::llvm::Value* address;
  // Address on the stack or heap of a reference pointing to this object
  // (QObject**). Only set if the expression resolves to a reference
  // (variable, instance member, or array element).
  ::llvm::Value* ref_address;
  // The reference mode of ref_address.
  RefMode ref_mode;
  // A function reference. Only set if the expression resolves to a function.
  const FnDef* fn_def;
  // A class reference. Only set if the expression resolves to a class.
  ClassType* class_type;

  ExprResult();
};

// Implements AST -> IR generation for expressions.
class ExprIRGenerator {
 public:
  ExprIRGenerator(
      ::llvm::Module* module,
      ::llvm::IRBuilder<>* ir_builder,
      const Builtins* builtins,
      Symbols* symbols);

  // Runs AST -> IR generation for an expression.
  ExprResult ProcessExpr(const Expr& expr);

  // Generates IR for an assign expression.
  //
  // If var is nullptr, will assign result to expr.dest_expr. Otherwise,
  // will generate code to allocate var and assign to it.
  ExprResult ProcessAssignExpr(const AssignExpr& expr, const Var* var);

  // If "result' does not have a memory address, create a temporary variable and
  // copy the value there. Otherwise, do nothing.
  void EnsureAddress(ExprResult* result);
  // Extracts a bool (int8) value from a QBool value.
  ::llvm::Value* ExtractBoolValue(::llvm::Value* wrapped_bool_value);
  // Creates a null-initialized local variable of the given type. The type
  // should be a non-reference type (e.g. QObject). Returns the reference
  // address of the local variable.
  //
  // Return type: QObject**.
  ::llvm::Value* CreateLocalVar(::llvm::Type* ty, const ::std::string& name);

 private:
  ExprResult ProcessConstantExpr(const ConstantExpr& expr);
  ExprResult ProcessVarExpr(const VarExpr& expr);
  ExprResult ProcessCallExpr(const CallExpr& expr);
  ExprResult ProcessBinaryOpExpr(const BinaryOpExpr& expr);

  // Wraps an int32 value into a QInt32 value.
  ::llvm::Value* CreateInt32Value(::llvm::Value* raw_int32_value);
  // Extracts an int32 value from a QInt32 value.
  ::llvm::Value* ExtractInt32Value(::llvm::Value* wrapped_int32_value);
  // Wraps a bool (int8) value into a QBool value.
  ::llvm::Value* CreateBoolValue(::llvm::Value* raw_bool_value);
  // Returns an object in memory (QObject*) for the given type.
  ::llvm::Value* CreateObject(ClassType* class_type);
  // Assigns the object at src_address to the variable referenced by
  // dest_ref_address.
  //
  // This will decrement the reference count of the object currently referenced
  // by dest_ref_address, and increment the reference count of the object at
  // src_address.
  //
  // Type of dest_ref_address: QObject**
  // Type of src_address: QObject*
  ::llvm::Value* AssignObject(
      ::llvm::Value* dest_ref_address,
      ::llvm::Value* src_address,
      RefMode ref_mode);

  ::llvm::LLVMContext& ctx_;
  ::llvm::Module* const module_;
  ::llvm::IRBuilder<>* const ir_builder_;
  const Builtins* const builtins_;
  Symbols* const symbols_;
};

}  // namespace quo

#endif  // EXPR_IR_GENERATOR_HPP_
