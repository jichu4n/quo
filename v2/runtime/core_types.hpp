#ifndef CORE_TYPES_HPP
#define CORE_TYPES_HPP

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "runtime/values.hpp"

// ============================================================================
//   Function
// ============================================================================

/** Function type. */
extern QTypeInfo QFunctionTypeInfo;

/** Function value representation. */
struct QFunctionValue : QValue {
  QValue* self;
  void* address;
};

/** Function value constructor. */
extern "C" QFunctionValue* __QFunctionValue_Create(QValue* self, void* address);

// ============================================================================
//   Int
// ============================================================================

/** Int type. */
extern QTypeInfo QIntTypeInfo;

/** Int value representation. */
struct QIntValue : QValue {
  int64_t value;
};

/** Int value constructor. */
extern "C" QIntValue* __QIntValue_Create(int64_t value);

/** User-defined literal operator to construct a QIntValue. */
inline QIntValue* operator"" _Q(unsigned long long value) {
  return __QIntValue_Create(value);
}

// ============================================================================
//   String
// ============================================================================

/** String type. */
extern QTypeInfo QStringTypeInfo;

/** String value representation. */
struct QStringValue : QValue {
  ::std::string value;
};

/** String value constructor. */
extern "C" QStringValue* __QStringValue_Create(const char* value);
/** String value constructor. */
inline QStringValue* __QStringValue_Create(const ::std::string& value) {
  return __QStringValue_Create(value.c_str());
}

/** User-defined literal operator to construct a QStringValue. */
inline QStringValue* operator"" _Q(const char* value, size_t size) {
  return __QStringValue_Create(value);
}

// ============================================================================
//   Array
// ============================================================================

/** Array type. */
extern QTypeInfo QArrayTypeInfo;

/** Array value representation. */
struct QArrayValue : QValue {
  ::std::vector<QValue*> elements;
};

/** Array value constructor. */
extern "C" QArrayValue* __QArrayValue_Create();
/** Array value constructor. */
template <typename ContainerT>
QArrayValue* __QArrayValue_CreateFromContainer(const ContainerT& elements) {
  QArrayValue* value = __QArrayValue_Create();
  value->elements.insert(
      value->elements.begin(), elements.begin(), elements.end());
  return value;
}

// ============================================================================
//   Misc
// ============================================================================

/** Look up a Quo object member based on static type maps.
 *
 * @param fields QFieldMap or similar STL container.
 * @param methods QMethodMap or similar STL container.
 */
template <typename FieldMapT, typename MethodMapT>
QValue* __QValue_GetMemberFromMap(
    QTypeInfo* type, const FieldMapT& fields, const MethodMapT& methods,
    QValue* self, const char* member_name) {
  assert(self != nullptr);
  assert(self->type_info == type);
  const auto field_it = fields.find(member_name);
  if (field_it != fields.end()) {
    return static_cast<QObjectValue*>(self)->fields[field_it->second];
  }
  const auto method_it = methods.find(member_name);
  if (method_it != methods.end()) {
    return __QFunctionValue_Create(self, method_it->second);
  }
  return nullptr;
}

/** Mapping from the name of a field to its index in a QObjectValue. */
typedef ::std::unordered_map<::std::string, int> QFieldMap;

/** Mapping from the name of a method to a function pointer. */
typedef ::std::unordered_map<::std::string, void*> QMethodMap;

#endif

