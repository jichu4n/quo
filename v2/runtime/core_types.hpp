#ifndef CORE_TYPES_HPP
#define CORE_TYPES_HPP

#include <cstdint>

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

#endif

