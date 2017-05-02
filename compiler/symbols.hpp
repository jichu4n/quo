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
#include <unordered_map>
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

  // Adds a variable into the scope.
  void AddVar(const Var& var);
  // Adds a temp into the scope.
  void AddTemp(::llvm::Value* address);

  // Look up a variable's address by name. Returns nullptr if not found.
  const Var* Lookup(const ::std::string& name);
  // Look up a variable's name by address. Returns nullptr if not found.
  const Var* Lookup(::llvm::Value* address);
};

}  // namespace quo

#endif  // SYMBOLS_HPP_
