#ifndef HZ6_CONTRACT_H
#define HZ6_CONTRACT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6RouteKind {
  HZ6_ROUTE_MISS = 0,
  HZ6_ROUTE_VALID = 1,
  HZ6_ROUTE_INVALID = 2
} Hz6RouteKind;

typedef enum Hz6ObjectState {
  HZ6_STATE_DEAD = 0,
  HZ6_STATE_ACTIVE = 1,
  HZ6_STATE_LOCAL_FREE = 2,
  HZ6_STATE_TRANSFER_FREE = 3,
  HZ6_STATE_REMOTE_PENDING = 4,
  HZ6_STATE_ORPHAN = 5
} Hz6ObjectState;

typedef enum Hz6FrontId {
  HZ6_FRONT_NONE = 0,
  HZ6_FRONT_LOCAL2P = 1,
  HZ6_FRONT_MIDPAGE = 2,
  HZ6_FRONT_LARGE = 3,
  HZ6_FRONT_TOY = 250
} Hz6FrontId;

typedef struct Hz6RouteResult {
  Hz6RouteKind kind;
  uint16_t front_id;
  uint16_t class_id;
  uint32_t generation;
  void* descriptor;
} Hz6RouteResult;

typedef struct Hz6OwnerToken {
  uint32_t slot;
  uint32_t generation;
} Hz6OwnerToken;

static inline Hz6RouteResult hz6_route_miss(void) {
  Hz6RouteResult result;
  result.kind = HZ6_ROUTE_MISS;
  result.front_id = HZ6_FRONT_NONE;
  result.class_id = 0;
  result.generation = 0;
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
  result.descriptor = descriptor;
  return result;
}

static inline int hz6_owner_equal(Hz6OwnerToken a, Hz6OwnerToken b) {
  return a.slot == b.slot && a.generation == b.generation;
}

#ifdef __cplusplus
}
#endif

#endif
