#include "runtime/values.hpp"
#include <stdexcept>
#include <string>

using namespace std;

class PropertyAccessException : public invalid_argument {
  public:
    PropertyAccessException(const PropertyAccessException& other) = default;
    PropertyAccessException(const string& message)
      : invalid_argument("PropertyAccessException: " + message) {
    };
};

QValue* __QValue_GetProperty(QValue* self, const char* prop_name) {
  if (self == nullptr) {
    throw PropertyAccessException(
	string("Attempting to access field ") + prop_name + " on null value");
  }
  const auto type_info = self->type_info;
  const auto value = (*(type_info->get_property_fn))(self, prop_name);
  if (value == nullptr) {
    throw PropertyAccessException(
	string("Field \"") + prop_name + "\" does not exist on type " + type_info->name);
  }
  return value;
}

