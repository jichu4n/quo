#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast/ast.pb.h"
#include "runtime/descriptor.pb.h"

namespace quo {

// An item in the function table at compile time.
struct FnTableItem {
  const ClassDef* class_def;
  const FnDef* fn_def;

  ::std::string GetOutputName() const;
};

class FnTable {
 public:
  int Add(const FnTableItem& item);

  ::std::vector<FnTableItem>::const_iterator begin() const {
    return items_.begin();
  }
  ::std::vector<FnTableItem>::const_iterator end() const {
    return items_.end();
  }

 private:
  ::std::vector<FnTableItem> items_;
};

// Encapsulates the type system.
class Types {
 public:
  // Constructs a type system representation for a given AST module.
  static ::std::unique_ptr<Types> Create(const ModuleDef& module_def);

  // Returns the constructed module descriptor.
  const rt::ModuleDescriptor& GetModuleDescriptor() const {
    return *module_desc_.get();
  }
  // Returns the constructed function table.
  const FnTable& GetFnTable() const {
    return fn_table_;
  }

 private:
  Types();

  void ProcessModule(
      const ModuleDef& module_def, rt::ModuleDescriptor* module_desc);
  void ProcessClassDef(
      const ClassDef& class_def, rt::ClassDescriptor* class_desc);
  void ProcessProp(const VarDeclStmt& var_decl, rt::PropDescriptor* prop_desc);
  void ProcessMethod(const FnDef& fn_def, rt::FnDescriptor* fn_desc);
  void ProcessFn(const FnDef& fn_def, rt::FnDescriptor* fn_desc);

  // Sort classes topologically, so that super classes appear before their
  // derived classes.
  static ::std::vector<const ClassDef*> SortClassDefs(
      const ::std::unordered_map<::std::string, const ClassDef*>&
          class_defs_by_name);

  static ::std::string GetName(
      const rt::ClassMemberDescriptor& class_member_desc);
  static ::std::string GetName(
      const ClassDef& class_def, const ClassDef::Member& member);

  // Current AST module being processed.
  const ModuleDef* module_def_;
  // Processed descriptors.
  ::std::unique_ptr<rt::ModuleDescriptor> module_desc_;
  // Current AST class being processed.
  const ClassDef* class_def_;
  // Current class being processed.
  const rt::ClassDescriptor* class_desc_;
  // Class descriptors, indexed by name.
  ::std::unordered_map<::std::string, rt::ClassDescriptor*>
      class_descs_by_name_;
  // Class descriptors, indexed by ClassDef.
  ::std::unordered_map<const ClassDef*, rt::ClassDescriptor*>
      class_descs_by_class_def_;
  ::std::unordered_map<const FnDef*, rt::FnDescriptor*>
      fn_descs_by_fn_def_;
  // Function definitions.
  FnTable fn_table_;
};

}  // namespace quo

#endif  // TYPES_HPP_
