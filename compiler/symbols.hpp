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

// Represents a local variable or function argument during IR generation.
struct Var {
  // The name of the variable / argument.
  ::std::string name;
  // Tye type of the variable.
  TypeSpec type_spec;
  // The address of the variable, as returned by the alloca instruction.
  // Type: QObject**
  ::llvm::Value* ref_address;
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

// Represents all symbols visible from a particular scope.
class Symbols {
 public:
  // Creates a new symbol table for an AST module.
  static ::std::unique_ptr<Symbols> Create(const ModuleDef& module_def);
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
  const FnDef* LookupFnDef(const ::std::string& name) const;

 private:
  Symbols();
  void SetupFnDefs(const ModuleDef& module_def);

  // Stack of scopes, from outermost to innermost.
  ::std::list<Scope> scopes_;
  // Global functions in the AST, by name.
  ::std::unordered_map<::std::string, const FnDef*> fn_defs_by_name_;
};

}  // namespace quo

#endif  // SYMBOLS_HPP_
