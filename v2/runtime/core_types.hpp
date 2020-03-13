#ifndef CORE_TYPES_HPP
#define CORE_TYPES_HPP

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "runtime/values.hpp"

// ============================================================================
//   Null
// ============================================================================

/** Null type. */
extern QTypeInfo QNullTypeInfo;

/** Null value representation. */
struct QNullValue : QValue {};

/** Singleton instance of null. */
extern const QNullValue __QNull;

// ============================================================================
//   Function
// ============================================================================

/** Function type. */
extern QTypeInfo QFunctionTypeInfo;

/** Function value representation. */
struct QFunctionValue;

/** Function value constructor. */
extern "C" QFunctionValue* __QFunctionValue_Create(QValue* self, void* address);

// ============================================================================
//   Int
// ============================================================================

/** Int type. */
extern QTypeInfo QIntTypeInfo;

/** Int value representation. */
struct QIntValue;

/** Int value constructor. */
extern "C" QIntValue* __QIntValue_Create(int64_t value);

// ============================================================================
//   String
// ============================================================================

/** String type. */
extern QTypeInfo QStringTypeInfo;

/** String value representation. */
struct QStringValue;

/** String value constructor. */
extern "C" QStringValue* __QStringValue_Create(const char* value);

// ============================================================================
//   Array
// ============================================================================

/** Array type. */
extern QTypeInfo QArrayTypeInfo;

/** Array value representation. */
struct QArrayValue;

/** Array value constructor. */
extern "C" QArrayValue* __QArrayValue_Create();

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

