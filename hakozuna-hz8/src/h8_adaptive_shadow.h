#ifndef H8_ADAPTIVE_SHADOW_H
#define H8_ADAPTIVE_SHADOW_H

#include "h8_class_map.h"
#include "h8_medium.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum H8AdaptiveShadowMode {
  H8_ADAPTIVE_SHADOW_BALANCED = 0,
  H8_ADAPTIVE_SHADOW_TRANSFER_PRESSURE = 1,
  H8_ADAPTIVE_SHADOW_RSS_PRESSURE = 2,
  H8_ADAPTIVE_SHADOW_MODE_COUNT = 3
} H8AdaptiveShadowMode;

typedef struct H8AdaptiveShadowSnapshot {
  uint64_t recommendation[H8_ADAPTIVE_SHADOW_MODE_COUNT];
  uint64_t small_refill[H8_CLASS_COUNT];
  uint64_t small_refill_pending[H8_CLASS_COUNT];
  uint64_t medium_source[H8_MEDIUM_CLASS_COUNT];
  uint64_t medium_rss_pressure[H8_MEDIUM_CLASS_COUNT];
  uint64_t remote_collect;
  uint64_t remote_pending_before;
  uint64_t remote_pending_after;
} H8AdaptiveShadowSnapshot;

#if defined(H8_ADAPTIVE_TRANSFER_SHADOW_L0)
void h8_adaptive_shadow_note_small_refill(uint32_t class_id,
                                          size_t pending);
void h8_adaptive_shadow_note_medium_source(uint32_t class_id,
                                           size_t resident_bytes,
                                           size_t resident_budget);
void h8_adaptive_shadow_note_remote_collect(size_t pending_before,
                                            size_t pending_after);
void h8_adaptive_shadow_note_medium_residency(uint32_t class_id,
                                              size_t resident_bytes,
                                              size_t resident_budget,
                                              bool rejected);
H8AdaptiveShadowSnapshot h8_adaptive_shadow_snapshot(void);
void h8_adaptive_shadow_dump(void);
#else
static inline void h8_adaptive_shadow_note_small_refill(uint32_t class_id,
                                                        size_t pending) {
  (void)class_id;
  (void)pending;
}
static inline void h8_adaptive_shadow_note_medium_source(
    uint32_t class_id, size_t resident_bytes, size_t resident_budget) {
  (void)class_id;
  (void)resident_bytes;
  (void)resident_budget;
}
static inline void h8_adaptive_shadow_note_remote_collect(
    size_t pending_before, size_t pending_after) {
  (void)pending_before;
  (void)pending_after;
}
static inline void h8_adaptive_shadow_note_medium_residency(
    uint32_t class_id, size_t resident_bytes, size_t resident_budget,
    bool rejected) {
  (void)class_id;
  (void)resident_bytes;
  (void)resident_budget;
  (void)rejected;
}
static inline H8AdaptiveShadowSnapshot h8_adaptive_shadow_snapshot(void) {
  H8AdaptiveShadowSnapshot snapshot = {0};
  return snapshot;
}
static inline void h8_adaptive_shadow_dump(void) {}
#endif

#endif /* H8_ADAPTIVE_SHADOW_H */
