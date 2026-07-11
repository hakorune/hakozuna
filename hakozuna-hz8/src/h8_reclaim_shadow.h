#ifndef H8_RECLAIM_SHADOW_H
#define H8_RECLAIM_SHADOW_H

#include <stdint.h>

typedef struct H8ThreadCtx H8ThreadCtx;
typedef struct H8Span H8Span;

typedef struct H8ReclaimShadowSnapshot {
  uint64_t owner_exit_spans;
  uint64_t owner_spans;
  uint64_t owner_span_bytes;
  uint64_t reclaimable_spans;
  uint64_t reclaimable_bytes;
  uint64_t blocked_active;
  uint64_t blocked_live;
  uint64_t blocked_pending;
  uint64_t blocked_state;
} H8ReclaimShadowSnapshot;

#if defined(H8_RECLAIM_ADAPTER_SHADOW_L0)
void h8_reclaim_shadow_note_owner_exit_span(H8ThreadCtx* ctx, H8Span* span,
                                            uint64_t used);
H8ReclaimShadowSnapshot h8_reclaim_shadow_snapshot(void);
void h8_reclaim_shadow_dump(void);
#else
static inline void h8_reclaim_shadow_note_owner_exit_span(H8ThreadCtx* ctx,
                                                          H8Span* span,
                                                          uint64_t used) {
  (void)ctx;
  (void)span;
  (void)used;
}
static inline H8ReclaimShadowSnapshot h8_reclaim_shadow_snapshot(void) {
  H8ReclaimShadowSnapshot snapshot = {0};
  return snapshot;
}
static inline void h8_reclaim_shadow_dump(void) {}
#endif

#endif /* H8_RECLAIM_SHADOW_H */
