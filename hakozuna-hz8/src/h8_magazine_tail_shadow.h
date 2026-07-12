#ifndef H8_MAGAZINE_TAIL_SHADOW_H
#define H8_MAGAZINE_TAIL_SHADOW_H

#include <stdint.h>

#define H8_MAGAZINE_TAIL_MAX_CLASSES 11u

typedef struct H8MagazineTailShadowSnapshot {
  uint64_t checkpoints;
  uint64_t inventory_spans;
  uint64_t inventory_bytes;
  uint64_t empty_spans;
  uint64_t empty_bytes;
  uint64_t tail_spans;
  uint64_t tail_empty_spans;
  uint64_t hypothetical_decommit_bytes;
  uint64_t max_inventory_bytes;
  uint64_t max_empty_bytes;
  uint64_t max_hypothetical_decommit_bytes;
  uint64_t blocked_live;
  uint64_t blocked_pending;
  uint64_t blocked_state;
  uint64_t class_inventory_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES];
  uint64_t class_empty_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES];
  uint64_t class_max_empty_bytes[H8_MAGAZINE_TAIL_MAX_CLASSES];
} H8MagazineTailShadowSnapshot;

typedef struct H8ThreadCtx H8ThreadCtx;

#if defined(H8_MAGAZINE_TAIL_RECLAIM_SHADOW_L0)
void h8_magazine_tail_shadow_checkpoint(H8ThreadCtx* ctx);
H8MagazineTailShadowSnapshot h8_magazine_tail_shadow_snapshot(void);
void h8_magazine_tail_shadow_dump(void);
#else
static inline void h8_magazine_tail_shadow_checkpoint(H8ThreadCtx* ctx) {
  (void)ctx;
}
static inline H8MagazineTailShadowSnapshot
h8_magazine_tail_shadow_snapshot(void) {
  H8MagazineTailShadowSnapshot snapshot = {0};
  return snapshot;
}
static inline void h8_magazine_tail_shadow_dump(void) {}
#endif

#endif /* H8_MAGAZINE_TAIL_SHADOW_H */
