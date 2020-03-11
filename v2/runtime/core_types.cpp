#include "runtime/core_types.hpp"
#include <string>
#include <unordered_map>

using namespace std;

// ============================================================================
//   Null
// ============================================================================

QValue* __QNullValue_GetProperty(QValue* self, const char* prop_name) {
  return nullptr;
}

QTypeInfo QNullTypeInfo = {
  .name = "Null",
  .get_property_fn = &__QNullValue_GetProperty,
};

const QNullValue __QNull = {
  {
    .type_info = &QNullTypeInfo,
    .refs = 1,
  },
};

// ============================================================================
//   Function
// ============================================================================

struct QFunctionValue : QValue {
  QValue* self;
  void* address;
};

QValue* __QFunctionValue_GetProperty(QValue* self, const char* prop_name) {
  return nullptr;
}

QTypeInfo QFunctionTypeInfo = {
  .name = "Function",
  .get_property_fn = &__QFunctionValue_GetProperty,
};

QFunctionValue* __QFunctionValue_Create(QValue* self, void* address) {
  return new QFunctionValue({
    {
      .type_info = &QFunctionTypeInfo,
      .refs = 1,
    },
    .self = self,
    .address = address,
  });
}

// ============================================================================
//   Int
// ============================================================================

struct QIntValue : QValue {
  int64_t value;
};

QValue* __QIntValue_GetProperty(QValue* self, const char* prop_name) {
  static const unordered_map<string, void*> methods {
    {
      "toString",
      reinterpret_cast<void*>(+[](QIntValue* self) {
	return __QStringValue_Create(to_string(self->value).c_str());
      }),
    },
  };
  if (methods.count(prop_name)) {
    return __QFunctionValue_Create(self, methods.at(prop_name));
  }
  return nullptr;
}

QTypeInfo QIntTypeInfo = {
  .name = "Int",
  .get_property_fn = &__QIntValue_GetProperty,
};

QIntValue* __QIntValue_Create(int64_t value) {
  return new QIntValue({
    {
      .type_info = &QIntTypeInfo,
      .refs = 1,
    },
    .value = value,
  });
}

// ============================================================================
//   String
// ============================================================================

struct QStringValue : QValue {
  string value;
};

QValue* __QStringValue_GetProperty(QValue* self, const char* prop_name) {
  static const unordered_map<string, void*> methods {
    {
      "length",
      reinterpret_cast<void*>(+[](QStringValue* self) {
	return __QIntValue_Create(self->value.length());
      }),
    },
  };
  if (methods.count(prop_name)) {
    return __QFunctionValue_Create(self, methods.at(prop_name));
  }
  return nullptr;
}

QTypeInfo QStringTypeInfo = {
  .name = "String",
  .get_property_fn = &__QStringValue_GetProperty,
};

QStringValue* __QStringValue_Create(const char* value) {
  return new QStringValue({
    {
      .type_info = &QStringTypeInfo,
      .refs = 1,
    },
    .value = value,
  });
}

