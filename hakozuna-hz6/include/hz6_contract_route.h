#ifndef HZ6_CONTRACT_ROUTE_H
#define HZ6_CONTRACT_ROUTE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6Allocator Hz6Allocator;

typedef enum Hz6RouteKind {
  HZ6_ROUTE_MISS = 0,
  HZ6_ROUTE_VALID = 1,
  HZ6_ROUTE_INVALID = 2
} Hz6RouteKind;

typedef enum Hz6AllocPath {
  HZ6_ALLOC_PATH_LOCAL_REUSE = 0,
  HZ6_ALLOC_PATH_TRANSFER_REUSE = 1,
  HZ6_ALLOC_PATH_PREFILL_REUSE = 2,
  HZ6_ALLOC_PATH_SOURCE_PREFILL = 3,
  HZ6_ALLOC_PATH_DIRECT_SOURCE = 4,
  HZ6_ALLOC_PATH_RELEASED_REUSE = 5,
  HZ6_ALLOC_PATH_OOM = 6,
  HZ6_ALLOC_PATH_UNSUPPORTED = 7,
  HZ6_ALLOC_PATH_COUNT = 8
} Hz6AllocPath;

typedef enum Hz6FrontId {
  HZ6_FRONT_NONE = 0,
  HZ6_FRONT_LOCAL2P = 1,
  HZ6_FRONT_MIDPAGE = 2,
  HZ6_FRONT_LARGE = 3,
  HZ6_FRONT_TOY = 250
} Hz6FrontId;

typedef enum Hz6FrontAttrIndex {
  HZ6_FRONT_ATTR_LOCAL2P = 0,
  HZ6_FRONT_ATTR_MIDPAGE = 1,
  HZ6_FRONT_ATTR_LARGE = 2,
  HZ6_FRONT_ATTR_TOY = 3,
  HZ6_FRONT_ATTR_COUNT = 4
} Hz6FrontAttrIndex;

static inline int hz6_front_attr_index_from_id(uint16_t front_id,
                                               size_t* index) {
  if (!index) {
    return 0;
  }
  switch (front_id) {
    case HZ6_FRONT_LOCAL2P:
      *index = HZ6_FRONT_ATTR_LOCAL2P;
      return 1;
    case HZ6_FRONT_MIDPAGE:
      *index = HZ6_FRONT_ATTR_MIDPAGE;
      return 1;
    case HZ6_FRONT_LARGE:
      *index = HZ6_FRONT_ATTR_LARGE;
      return 1;
    case HZ6_FRONT_TOY:
      *index = HZ6_FRONT_ATTR_TOY;
      return 1;
    default:
      return 0;
  }
}

typedef struct Hz6RouteResult {
  Hz6RouteKind kind;
  uint16_t front_id;
  uint16_t class_id;
  uint32_t generation;
  Hz6Allocator* route_allocator;
  void* descriptor;
} Hz6RouteResult;

static inline Hz6RouteResult hz6_route_miss(void) {
  Hz6RouteResult result;
  result.kind = HZ6_ROUTE_MISS;
  result.front_id = HZ6_FRONT_NONE;
  result.class_id = 0;
  result.generation = 0;
  result.route_allocator = NULL;
  result.descriptor = NULL;
  return result;
}

static inline Hz6RouteResult hz6_route_invalid(uint16_t front_id,
                                               uint16_t class_id) {
  Hz6RouteResult result;
  result.kind = HZ6_ROUTE_INVALID;
  result.front_id = front_id;
  result.class_id = class_id;
  result.generation = 0;
  result.route_allocator = NULL;
  result.descriptor = NULL;
  return result;
}

static inline Hz6RouteResult hz6_route_valid(uint16_t front_id,
                                             uint16_t class_id,
                                             uint32_t generation,
                                             void* descriptor) {
  Hz6RouteResult result;
  result.kind = HZ6_ROUTE_VALID;
  result.front_id = front_id;
  result.class_id = class_id;
  result.generation = generation;
  result.route_allocator = NULL;
  result.descriptor = descriptor;
  return result;
}

#ifdef __cplusplus
}
#endif

#endif
