#ifndef HAKOZUNA_HZ7_H
#define HAKOZUNA_HZ7_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct H7Stats {
  size_t active_bytes;
  size_t reserved_bytes;
  size_t span_count;
  size_t direct_count;
  size_t route_count;
  size_t route_register_fail;
} H7Stats;

typedef enum H7RouteKind {
  H7_ROUTE_MISS = 0,
  H7_ROUTE_VALID = 1,
  H7_ROUTE_INVALID = 2
} H7RouteKind;

void* h7_malloc(size_t size);
void* h7_calloc(size_t count, size_t size);
void h7_free(void* ptr);
H7RouteKind h7_route(void* ptr);
H7Stats h7_stats(void);

#ifdef __cplusplus
}
#endif

#endif
