#include "runtime/values.hpp"

#include <stdexcept>
#include <string>

#include "absl/strings/str_cat.h"

using namespace std;
using namespace absl;

class MemberAccessException : public invalid_argument {
 public:
  MemberAccessException(const MemberAccessException& other) = default;
  MemberAccessException(const string& message)
      : invalid_argument("MemberAccessException: " + message){};
};

QValue* __QValue_GetMember(QValue* self, const char* member_name) {
  if (self == nullptr) {
    throw MemberAccessException(
        StrCat("Attempting to access member ", member_name, " on null value"));
  }
  const auto type_info = self->type_info;
  const auto value = (*(type_info->get_member_fn))(self, member_name);
  if (value == nullptr) {
    throw MemberAccessException(StrCat(
        "Member \"", member_name, "\" does not exist on type ",
        type_info->name));
  }
  return value;
}

