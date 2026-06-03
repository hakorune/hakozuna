#ifndef HZ6_SOURCE_UTIL_H
#define HZ6_SOURCE_UTIL_H

#include <stddef.h>

static inline size_t hz6_source_round_up(size_t value, size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  size_t mask = alignment - (size_t)1;
  return (value + mask) & ~mask;
}

#endif
