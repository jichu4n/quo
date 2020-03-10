#include "runtime/core_types.hpp"
#include <string>

// ============================================================================
//   Null
// ============================================================================

QTypeInfo QNullTypeInfo = {
  .name = "Null",
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

QTypeInfo QFunctionTypeInfo = {
  .name = "Function",
};

struct QFunctionValue : QValue {
  QValue* self;
  void* address;
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

QTypeInfo QIntTypeInfo = {
  .name = "Int",
};

struct QIntValue : QValue {
  int64_t value;
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

QTypeInfo QStringTypeInfo = {
  .name = "String",
};

struct QStringValue : QValue {
  ::std::string value;
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

