#include "runtime/core_types.hpp"

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// ============================================================================
//   Null
// ============================================================================

QValue* __QNullValue_GetMember(QValue* self, const char* member_name) {
  return __QValue_GetMemberFromMap(
      &QNullTypeInfo, QFieldMap(), QMethodMap(), self, member_name);
}

QTypeInfo QNullTypeInfo = {
    .name = "Null",
    .get_member_fn = &__QNullValue_GetMember,
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

QValue* __QFunctionValue_GetMember(QValue* self, const char* member_name) {
  return nullptr;
}

QTypeInfo QFunctionTypeInfo = {
    .name = "Function",
    .get_member_fn = &__QFunctionValue_GetMember,
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

QValue* __QIntValue_GetMember(QValue* self, const char* member_name) {
  static const QMethodMap methods{
      {
          "toString",
          reinterpret_cast<void*>(+[](QIntValue* self) {
            return __QStringValue_Create(to_string(self->value).c_str());
          }),
      },
  };
  return __QValue_GetMemberFromMap(
      &QIntTypeInfo, QFieldMap(), methods, self, member_name);
}

QTypeInfo QIntTypeInfo = {
    .name = "Int",
    .get_member_fn = &__QIntValue_GetMember,
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

QValue* __QStringValue_GetMember(QValue* self, const char* member_name) {
  static const QMethodMap methods{
      {
          "length",
          reinterpret_cast<void*>(+[](QStringValue* self) {
            return __QIntValue_Create(self->value.length());
          }),
      },
  };
  return __QValue_GetMemberFromMap(
      &QStringTypeInfo, QFieldMap(), methods, self, member_name);
}

QTypeInfo QStringTypeInfo = {
    .name = "String",
    .get_member_fn = &__QStringValue_GetMember,
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

// ============================================================================
//   Array
// ============================================================================

class ArrayAccessException : public invalid_argument {
 public:
  ArrayAccessException(const ArrayAccessException& other) = default;
  ArrayAccessException(const string& message)
      : invalid_argument("ArrayAccessException: " + message){};
};

QValue* __QArrayValue_GetMember(QValue* self, const char* member_name) {
  static const QMethodMap methods{
      {
          "length",
          reinterpret_cast<void*>(+[](QArrayValue* self) {
            return __QIntValue_Create(self->elements.size());
          }),
      },
      {
          "get",
          reinterpret_cast<void*>(+[](QArrayValue* self, QIntValue* idx) {
            assert(idx != nullptr);
            assert(idx->type_info == &QIntTypeInfo);
            if (idx->value < 0 || idx->value >= self->elements.size()) {
              throw ArrayAccessException(
                  string("Array index ") + to_string(idx->value) +
                  " out of bounds for array with " +
                  to_string(self->elements.size()) + " elements");
            }
            return self->elements[idx->value];
          }),
      },
      {
          "append",
          reinterpret_cast<void*>(+[](QArrayValue* self, QValue* element) {
            assert(element != nullptr);
            self->elements.push_back(element);
            return &__QNull;
          }),
      },
  };
  return __QValue_GetMemberFromMap(
      &QArrayTypeInfo, QFieldMap(), methods, self, member_name);
}

QTypeInfo QArrayTypeInfo = {
    .name = "Array",
    .get_member_fn = &__QArrayValue_GetMember,
};

QArrayValue* __QArrayValue_Create() {
  return new QArrayValue({
      {
          .type_info = &QArrayTypeInfo,
          .refs = 1,
      },
  });
}
