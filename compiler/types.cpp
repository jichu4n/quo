#include "compiler/types.hpp"
#include <functional>
#include <unordered_set>
#include "compiler/exceptions.hpp"
#include "compiler/utils.hpp"
#include "glog/logging.h"

namespace quo {

using ::std::function;
using ::std::string;
using ::std::unique_ptr;
using ::std::unordered_map;
using ::std::unordered_set;
using ::std::vector;

int FnTable::Add(const FnTableItem& item) {
  items_.push_back(item);
  return items_.size() - 1;
}

unique_ptr<Types> Types::Create(const ModuleDef& module_def) {
  unique_ptr<Types> types(new Types());
  types->ProcessModule(module_def, types->module_desc_.get());
  return types;
}

Types::Types()
    : module_def_(nullptr),
      module_desc_(new rt::ModuleDescriptor()),
      class_def_(nullptr),
      class_desc_(nullptr) {}

void Types::ProcessModule(
    const ModuleDef& module_def, rt::ModuleDescriptor* module_desc) {
  module_def_ = &module_def;
  unordered_map<string, const ClassDef*> class_defs_by_name;
  for (const auto& member : module_def.members()) {
    switch (member.type_case()) {
      case ModuleDef::Member::kClassDef: {
        const ClassDef& class_def = member.class_def();
        if (class_defs_by_name.count(class_def.name())) {
          throw Exception(
              class_def.line(), "Duplicate class definition '%s'",
              class_def.name().c_str());
        }
        class_defs_by_name[class_def.name()] = &class_def;
        break;
      }
      case ModuleDef::Member::kFnDef:
        ProcessFn(
            member.fn_def(),
            module_desc->add_member_descs()->mutable_fn_desc());
        break;
    }
  }
  vector<const ClassDef*> sorted_class_defs = SortClassDefs(class_defs_by_name);
  for (const ClassDef* class_def : sorted_class_defs) {
    ProcessClassDef(
        *class_def, module_desc->add_member_descs()->mutable_class_desc());
  }
}

void Types::ProcessClassDef(
    const ClassDef& class_def, rt::ClassDescriptor* class_desc) {
  if (class_descs_by_name_.count(class_def.name())) {
    throw Exception(
        class_def.line(), "Duplicate class definition '%s'",
        class_def.name().c_str());
  }
  class_def_ = &class_def;
  class_desc->set_name(class_def.name());
  class_descs_by_name_[class_desc->name()] = class_desc;
  class_desc_ = class_desc;
  unordered_map<string, rt::ClassMemberDescriptor*> member_descs_by_name;
  int prop_idx = 0;
  for (const auto& type_spec : class_def.super_classes()) {
    rt::ClassDescriptor* super_class_desc =
        class_descs_by_name_.at(type_spec.name());
    for (const auto& super_class_member_desc :
         super_class_desc->member_descs()) {
      const string name = GetName(super_class_member_desc);
      const auto& existing_member_desc_it = member_descs_by_name.find(name);
      if (existing_member_desc_it == member_descs_by_name.end()) {
        rt::ClassMemberDescriptor* member_desc = class_desc->add_member_descs();
        member_descs_by_name[name] = member_desc;
        member_desc->CopyFrom(super_class_member_desc);
        if (member_desc->type_case() == rt::ClassMemberDescriptor::kPropDesc) {
          member_desc->mutable_prop_desc()->set_index(prop_idx++);
        }
      } else {
        if (existing_member_desc_it->second->decl_class_name() ==
            super_class_member_desc.decl_class_name()) {
          // Diamond inheritance, no-op.
        } else if (
            existing_member_desc_it->second->type_case() ==
                rt::ClassMemberDescriptor::kFnDesc &&
            super_class_member_desc.type_case() ==
                rt::ClassMemberDescriptor::kFnDesc &&
            super_class_member_desc.fn_desc().is_override()) {
          // Override.
          // TODO: Check method signature compatibility.
          existing_member_desc_it->second->CopyFrom(super_class_member_desc);
        } else {
          throw Exception(
              class_def.line(),
              "Conflicting class member name in super classes '%s' and '%s': "
              "'%s'",
              existing_member_desc_it->second->decl_class_name().c_str(),
              super_class_member_desc.decl_class_name().c_str(), name.c_str());
        }
      }
    }
  }
  for (const auto& member : class_def.members()) {
    string name = GetName(class_def, member);
    rt::ClassMemberDescriptor* member_desc;
    auto existing_member_desc_it = member_descs_by_name.find(name);
    if (existing_member_desc_it != member_descs_by_name.end()) {
      if (existing_member_desc_it->second->type_case() ==
              rt::ClassMemberDescriptor::kFnDesc &&
          member.type_case() == ClassDef::Member::kFnDef &&
          member.fn_def().is_override()) {
        // TODO: Check method signature compatibility.
        member_desc = existing_member_desc_it->second;
        member_desc->Clear();
      } else {
        throw Exception(
            member.line(),
            "Conflicting class member name '%s' in class '%s' (previously "
            "declared in '%s')",
            name.c_str(), class_desc->name().c_str(),
            existing_member_desc_it->second->decl_class_name().c_str());
      }
    } else {
      member_desc = class_desc->add_member_descs();
      member_descs_by_name[name] = member_desc;
    }
    member_desc->set_decl_class_name(class_desc->name());
    switch (member.type_case()) {
      case ClassDef::Member::kVarDecl:
        ProcessProp(member.var_decl(), member_desc->mutable_prop_desc());
        member_desc->mutable_prop_desc()->set_index(prop_idx++);
        break;
      case ClassDef::Member::kFnDef:
        ProcessMethod(member.fn_def(), member_desc->mutable_fn_desc());
        break;
    }
  }
  class_desc_ = nullptr;
  class_def_ = nullptr;
}

void Types::ProcessProp(
    const VarDeclStmt& var_decl, rt::PropDescriptor* prop_desc) {
  prop_desc->set_name(var_decl.name());
  prop_desc->set_type_name(var_decl.type_spec().name());
  prop_desc->set_ref_mode(static_cast<rt::RefMode>(var_decl.ref_mode()));
}

void Types::ProcessMethod(const FnDef& fn_def, rt::FnDescriptor* fn_desc) {
  rt::FnParamDescriptor* fn_param_desc = fn_desc->add_param_descs();
  fn_param_desc->set_name("this");
  fn_param_desc->set_ref_mode(rt::STRONG_REF);
  fn_param_desc->set_type_name(class_desc_->name());
  ProcessFn(fn_def, fn_desc);
}

void Types::ProcessFn(const FnDef& fn_def, rt::FnDescriptor* fn_desc) {
  fn_desc->set_name(fn_def.name());
  fn_desc->set_fn_table_index(
      fn_table_.Add(
          {
              class_def_, class_desc_, &fn_def,
          }));
  for (const FnParam& param : fn_def.params()) {
    rt::FnParamDescriptor* fn_param_desc = fn_desc->add_param_descs();
    fn_param_desc->set_name(param.name());
    fn_param_desc->set_ref_mode(static_cast<rt::RefMode>(param.ref_mode()));
    fn_param_desc->set_type_name(param.type_spec().name());
  }
  fn_desc->set_return_type_name(fn_def.return_type_spec().name());
  fn_desc->set_is_override(fn_def.is_override());
}

vector<const ClassDef*> Types::SortClassDefs(
    const unordered_map<string, const ClassDef*>& class_defs_by_name) {
  // Topological sort via DFS.
  vector<const ClassDef*> sorted_class_defs;
  sorted_class_defs.reserve(class_defs_by_name.size());
  unordered_set<const ClassDef*> processed_class_defs;
  unordered_set<const ClassDef*> staged_class_defs;
  function<void(const ClassDef*)> visit =
      [&class_defs_by_name, &sorted_class_defs, &processed_class_defs,
       &staged_class_defs, &visit](const ClassDef* class_def) {
        if (processed_class_defs.count(class_def)) {
          return;
        }
        if (staged_class_defs.count(class_def)) {
          throw Exception(
              class_def->line(), "Cycle detected in class hierarchy: '%s'",
              class_def->name().c_str());
        }
        staged_class_defs.insert(class_def);
        for (const auto& type_spec : class_def->super_classes()) {
          const auto it = class_defs_by_name.find(type_spec.name());
          if (it == class_defs_by_name.end()) {
            throw Exception(
                class_def->line(), "Super class '%s' not found",
                type_spec.name().c_str());
          }
          visit(it->second);
        }
        sorted_class_defs.push_back(class_def);
        processed_class_defs.insert(class_def);
      };
  for (const auto& it : class_defs_by_name) {
    visit(it.second);
  }
  return sorted_class_defs;
}

string Types::GetName(const rt::ClassMemberDescriptor& class_member_desc) {
  switch (class_member_desc.type_case()) {
    case rt::ClassMemberDescriptor::kPropDesc:
      return class_member_desc.prop_desc().name();
    case rt::ClassMemberDescriptor::kFnDesc:
      return class_member_desc.fn_desc().name();
    default:
      throw Exception(
          "Unexpected class member type: %s",
          class_member_desc.GetTypeName().c_str());
  }
}

string Types::GetName(
    const ClassDef& class_def, const ClassDef::Member& member) {
  string name;
  switch (member.type_case()) {
    case ClassDef::Member::kVarDecl:
      name = member.var_decl().name();
      break;
    case ClassDef::Member::kFnDef:
      name = member.fn_def().name();
      break;
    default:
      throw Exception(
          "Unexpected member type: %s", member.GetTypeName().c_str());
  }
  CHECK(!name.empty());
  if (name[0] == '_') {
    name = StringPrintf("_%s%s", class_def.name().c_str(), name.c_str());
  }
  return name;
}

}  // namespace quo
