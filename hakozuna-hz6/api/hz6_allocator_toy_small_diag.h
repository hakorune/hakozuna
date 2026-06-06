#ifndef HZ6_ALLOCATOR_TOY_SMALL_DIAG_H
#define HZ6_ALLOCATOR_TOY_SMALL_DIAG_H

#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

static inline int hz6_toy_small_hotpath_diag_is_toy_small(uint16_t front_id,
                                                          uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  return front_id == HZ6_FRONT_TOY &&
         class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
#else
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_fast_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_fast_attempt;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_fast_hit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_fast_hit;
    ++allocator->stats.toy_small_activate_descriptor;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_front_dispatch(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_front_dispatch;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_route_lookup(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_route_lookup;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_owner_equal(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_owner_equal;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_fast_hit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_fast_hit;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_cache_push(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_cache_push;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

#endif
