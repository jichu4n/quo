#ifndef VALUES_HPP
#define VALUES_HPP

#include <cstdint>

struct QValue;

typedef QValue* (*GetMemberFn)(QValue* self, const char* prop_name);

/** Runtime type information. */
struct QTypeInfo {
  /** Name of this Quo type. */
  const char* name;
  /** Look up a member by name. */
  GetMemberFn get_member_fn;
};

/** Base class for runtime values. */
struct QValue {
  /** Pointer to this object's runtime type information. */
  QTypeInfo* type_info;
  /** Reference count. */
  int64_t refs;
};

struct QObjectValue : QValue {
  /** Member fields. */
  QValue* fields[];
};

/** Access object property by name. */
extern "C" QValue* __QValue_GetMember(QValue* self, const char* prop_name);

#endif

