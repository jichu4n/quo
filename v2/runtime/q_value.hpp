#ifndef QVALUE_HPP
#define QVALUE_HPP

#include <cstdint>

struct QValue;

typedef QValue* (*GetPropertyFn)(QValue* self, const char* prop_name);

/** Runtime type information. */
struct QTypeInfo {
  const char* name;
  GetPropertyFn get_property_fn;
};

/** Base class for runtime values. */
struct QValue {
  QTypeInfo* type_info;
  int64_t refs;
};

/** Access object property by name. */
extern "C" QValue* __QValue_GetProperty(QValue* self, const char* prop_name);

#endif

