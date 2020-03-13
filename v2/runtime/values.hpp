#ifndef VALUES_HPP
#define VALUES_HPP

#include <cstdint>

struct QValue;

typedef QValue* (*GetMemberFn)(QValue* self, const char* prop_name);

/** Runtime type information. */
struct QTypeInfo {
  const char* name;
  GetMemberFn get_member_fn;
};

/** Base class for runtime values. */
struct QValue {
  QTypeInfo* type_info;
  int64_t refs;
};

/** Access object property by name. */
extern "C" QValue* __QValue_GetMember(QValue* self, const char* prop_name);

#endif

