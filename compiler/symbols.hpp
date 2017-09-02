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

#ifndef SYMBOLS_HPP_
#define SYMBOLS_HPP_

#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "ast/ast.pb.h"

namespace quo {

class Builtins;

// Represents a local variable or function argument during IR generation.
struct Var {
  // The name of the variable / argument.
  ::std::string name;
  // The type of the variable.
  TypeSpec type_spec;
  // The address of the variable, as returned by the alloca instruction.
  // Type: QObject**
  ::llvm::Value* ref_address;
  // The reference mode of the variable.
  RefMode ref_mode;
};

// Represents a single layer of variable scope.
struct Scope {
  // Addresses of variable in this scope in insertion order.
  ::std::list<Var> vars;
  // Map from name to variables.
  ::std::unordered_map<::std::string, Var*> vars_by_name;
  // Map from address to names.
  ::std::unordered_map<::llvm::Value*, Var*> vars_by_ref_address;
  // Addresses of temps in this scope in insertion order.
  ::std::list<::llvm::Value*> temps;
  // Whether this scope corresponds to a function.
  bool is_fn_scope;

  // Adds a variable into the scope.
  void AddVar(const Var& var);
  // Adds a temp into the scope.
  void AddTemp(::llvm::Value* address);

  // Look up a variable's address by name. Returns nullptr if not found.
  const Var* Lookup(const ::std::string& name) const;
  // Look up a variable's name by address. Returns nullptr if not found.
  const Var* Lookup(::llvm::Value* address) const;
};

// Represents a member field (instance variable).
struct FieldType {
  // The name of the field.
  ::std::string name;
  // The type of the field.
  TypeSpec type_spec;
  // The reference mode of the field.
  RefMode ref_mode;
  // The index of the field within the class.
  int index;
};

// Represents a function or method.
struct FnType {
  // The name of the method.
  ::std::string name;
  // The function's definition.
  const FnDef* fn_def;
  // The function's type signature.
  ::llvm::FunctionType* fn_ty;
  // The function's address.
  ::llvm::Function* fn;
};

// Represents a class during IR generation.
struct ClassType {
  // Class type spec.
  TypeSpec type_spec;
  // Class definition in the AST.
  const ClassDef* class_def;
  // The LLVM IR type representation.
  ::llvm::StructType* ty;
  // Class descriptor object.
  ::llvm::GlobalVariable* desc;
  // Member fields (instance variables).
  ::std::list<FieldType> fields;
  // Member fields, indexed by name.
  ::std::unordered_map<::std::string, FieldType*> fields_by_name;
  // Methods.
  ::std::list<FnType> methods;
  // Methods, indexed by name.
  ::std::unordered_map<::std::string, FnType*> methods_by_name;

  // Looks up a field by name, and throws an exception if not found.
  FieldType* LookupFieldOrThrow(const ::std::string& name);
  // Looks up a method by name, and throws an exception if not found.
  FnType* LookupMethodOrThrow(const ::std::string& name);

  struct MemberType {
    ClassDef::Member::TypeCase type_case;
    FieldType* field_type;
    FnType* method_type;
  };
  MemberType LookupMemberOrThrow(const ::std::string& name);
};

// Represents all symbols visible from a particular scope.
class Symbols {
 public:
  // Creates a new symbol table for an AST module.
  static ::std::unique_ptr<Symbols> Create(
      ::llvm::Module* module,
      Builtins* builtins,
      const ModuleDef& module_def);
  // Pushes a new scope onto the stack.
  Scope* PushScope();
  // Pops off the top-most scope.
  void PopScope();
  // Returns a reference to the top-most scope.
  Scope* GetScope();
  // Lists all scopes from the top up to and including the closest function
  // scope, sorted from top to bottom.
  const ::std::vector<Scope*> GetScopesInFn();

  // Looks up a variable in the scope stack by name.
  const Var* LookupVar(const ::std::string& name) const;
  // Looks up a function definition by name.
  const FnType* LookupFn(const ::std::string& name) const;
  // Same as LookupFn, but throws an exception if the function cannot be found.
  const FnType* LookupFnOrThrow(const ::std::string& name) const;

  // Looks up the LLVM IR type representation for an AST type. Returns nullptr
  // if the type could not be found.
  ClassType* LookupType(const TypeSpec& type_spec) const;
  // Same as LookupType, but throws an exception if the type cannot be found.
  ClassType* LookupTypeOrDie(const TypeSpec& type_spec) const;

 private:
  Symbols(::llvm::Module* module, Builtins* builtins);

  void SetupBuiltinClassTypes();
  void SetupFnDefs(const ModuleDef& module_def);
  void SetupFnType(FnType* fn_type, const FnDef& fn_def, ClassType* class_type);
  void SetupClassTypes(const ModuleDef& module_def);
  void SetupClassFields(ClassType* class_type, const ClassDef& class_def);
  void SetupClassMethods(ClassType* class_type, const ClassDef& class_def);

  ::llvm::LLVMContext& ctx_;
  ::llvm::Module* const module_;
  Builtins* builtins_;

  // Stack of scopes, from outermost to innermost.
  ::std::list<Scope> scopes_;
  // Global functions.
  ::std::list<FnType> fn_types_;
  // Global functions, by name.
  ::std::unordered_map<::std::string, FnType*> fn_types_by_name_;
  // User-defined classes.
  ::std::list<ClassType> class_types_;
  // Classes by serialized type spec.
  ::std::unordered_map<::std::string, ClassType*> class_types_map_;
};

}  // namespace quo

#endif  // SYMBOLS_HPP_
