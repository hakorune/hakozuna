#ifndef H8_SMALL_REUSE_VISIBILITY_SHADOW_H
#define H8_SMALL_REUSE_VISIBILITY_SHADOW_H

#include <stdint.h>

typedef struct H8SmallReuseVisibilitySnapshot {
  uint64_t checkpoints;
  uint64_t owned_scan_spans;
  uint64_t usable_spans;
  uint64_t hidden_usable_spans;
  uint64_t hidden_usable_bytes;
  uint64_t max_hidden_usable_spans;
  uint64_t max_hidden_usable_bytes;
  uint64_t full_spans;
  uint64_t blocked_state;
} H8SmallReuseVisibilitySnapshot;

typedef struct H8ThreadCtx H8ThreadCtx;
typedef struct H8OwnerRecord H8OwnerRecord;

#if defined(H8_SMALL_REUSE_VISIBILITY_SHADOW_L0)
void h8_small_reuse_visibility_checkpoint(H8ThreadCtx* ctx,
                                          H8OwnerRecord* owner,
                                          uint32_t class_id);
H8SmallReuseVisibilitySnapshot h8_small_reuse_visibility_snapshot(void);
void h8_small_reuse_visibility_dump(void);
#else
static inline void h8_small_reuse_visibility_checkpoint(H8ThreadCtx* ctx,
                                                        H8OwnerRecord* owner,
                                                        uint32_t class_id) {
  (void)ctx;
  (void)owner;
  (void)class_id;
}
static inline H8SmallReuseVisibilitySnapshot
h8_small_reuse_visibility_snapshot(void) {
  H8SmallReuseVisibilitySnapshot snapshot = {0};
  return snapshot;
}
static inline void h8_small_reuse_visibility_dump(void) {}
#endif

#endif /* H8_SMALL_REUSE_VISIBILITY_SHADOW_H */
