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

#include <memory>
#include "glog/logging.h"
#include "llvm/Support/raw_ostream.h"
#include "compiler/ir_generator.hpp"

using ::std::unique_ptr;
using namespace ::quo;

int main(int argc, char* argv[]) {
  ModuleDef module_def;
  FuncDef* fn_def = module_def.add_members()->mutable_func_def();
  fn_def->set_name("foo");
  FuncParam* param = fn_def->add_params();
  param->set_name("x");
  param->mutable_type_spec()->set_name("Int");
  RetStmt* ret = fn_def->mutable_block()->add_stmts()->mutable_ret();
  BinaryOpExpr* expr = ret->mutable_expr()->mutable_binary_op();
  expr->set_op(BinaryOpExpr::ADD);
  expr->mutable_left_expr()->mutable_constant()->set_intvalue(42);
  expr->mutable_right_expr()->mutable_constant()->set_intvalue(9);
  // LOG(INFO) << module_def.DebugString();

  IRGenerator ir_generator;
  unique_ptr<::llvm::Module> module = ir_generator.ProcessModule(module_def);

  module->print(
      ::llvm::outs(),
      nullptr /* AssemblyAnnotationWriter */,
      false /* ShouldPreserveUseListOrder */,
      true /* IsForDebug */);

  return 0;
}
