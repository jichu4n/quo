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

#ifndef BUILTINS_HPP_
#define BUILTINS_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "ast/ast.pb.h"

namespace quo {

// A built-in data type.
struct BuiltinType {
  // The AST type representation.
  TypeSpec type_spec;
  // The LLVM IR type representation.
  ::llvm::StructType* ty;
};

// Encapsulates global built-in entities.
class Builtins {
 public:
  // Constructs a Builtins instance for the given LLVM IR module.
  static ::std::unique_ptr<const Builtins> Create(::llvm::Module* module);

  // Looks up the LLVM IR type representation for an AST type.
  ::llvm::Type* LookupType(const TypeSpec& type_spec) const;
  // Looks up the class descriptor for an AST type.
  ::llvm::GlobalVariable* LookupDescriptor(const TypeSpec& type_spec) const;

  // Built-in data types.
  struct {
    BuiltinType object_type;
    BuiltinType int32_type;
    BuiltinType int_type;
    BuiltinType bool_type;
    BuiltinType string_type;
    ::llvm::StructType* class_desc_ty;
    ::llvm::StructType* class_view_ty;
    ::llvm::StructType* field_desc_ty;
  } types;

  // Built-in runtime library functions.
  struct {
    ::llvm::Constant* quo_alloc;
    ::llvm::Constant* quo_inc_refs;
    ::llvm::Constant* quo_dec_refs;
    ::llvm::Constant* quo_assign;
    ::llvm::Constant* quo_alloc_string;
    ::llvm::Constant* quo_string_concat;
    FnDef quo_print_fn_def;
    ::llvm::Constant* quo_print;
  } fns;

  // Built-in runtime library global variables.
  struct {
    // Data type class descriptors.
    ::llvm::GlobalVariable* int32_desc;
    ::llvm::GlobalVariable* bool_desc;
    ::llvm::GlobalVariable* string_desc;
  } globals;

 private:
  explicit Builtins(::llvm::Module* module);
  void SetupBuiltinTypes();
  void SetupBuiltinGlobals();
  void SetupBuiltinFunctions();

  // LLVM context.
  ::llvm::LLVMContext& ctx_;
  // LLVM IR module where the built-in entities are declared.
  ::llvm::Module* const module_;

  // Serialized TypeSpec -> LLVM IR type.
  ::std::unordered_map<::std::string, ::llvm::Type*> types_map_;
  // Function name -> runtime library function.
  ::std::unordered_map<::std::string, const FnDef*> fn_defs_by_name;
  // Serialized TypeSpec -> runtime library global variable.
  ::std::unordered_map<::std::string, ::llvm::GlobalVariable*> descs_map_;
};

}  // namespace quo

#endif  // BUILTINS_HPP_
