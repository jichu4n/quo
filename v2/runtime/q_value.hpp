#ifndef QVALUE_HPP
#define QVALUE_HPP

#include <cstdint>

struct QTypeInfo {
  const char* name;
};

/** Base class for runtime values. */
struct QValue {
  QTypeInfo* type_info;
  int64_t refs;
};

#endif

