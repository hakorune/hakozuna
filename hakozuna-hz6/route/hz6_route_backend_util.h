#ifndef HZ6_ROUTE_BACKEND_UTIL_H
#define HZ6_ROUTE_BACKEND_UTIL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int hz6_route_backend_valid_granularity(size_t granularity) {
  return granularity != 0 && (granularity & (granularity - (size_t)1)) == 0;
}

static inline uintptr_t hz6_route_backend_align_down(uintptr_t value,
                                                     size_t granularity) {
  return value & ~((uintptr_t)granularity - (uintptr_t)1);
}

static inline uintptr_t hz6_route_backend_align_up(uintptr_t value,
                                                   size_t granularity) {
  return (value + (uintptr_t)granularity - (uintptr_t)1) &
         ~((uintptr_t)granularity - (uintptr_t)1);
}

#ifdef __cplusplus
}
#endif

#endif
