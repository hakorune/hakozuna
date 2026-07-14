#ifndef H8_SMALL_TRANSITION_INVENTORY_H
#define H8_SMALL_TRANSITION_INVENTORY_H

#include <stdint.h>

typedef struct H8ThreadCtx H8ThreadCtx;
typedef struct H8OwnerRecord H8OwnerRecord;
typedef struct H8Span H8Span;

typedef struct H8SmallTransitionInventoryStats {
  uint64_t candidate;
  uint64_t push;
  uint64_t duplicate;
  uint64_t pop_attempt;
  uint64_t pop_hit;
  uint64_t pop_reject;
  uint64_t remote_push;
  uint64_t reset;
  uint64_t depth_max;
} H8SmallTransitionInventoryStats;

#if defined(H8_SMALL_TRANSITION_INVENTORY_L1)
void h8_small_transition_inventory_note_local_free(H8ThreadCtx* ctx,
                                                   H8Span* span,
                                                   uint32_t old_free_head);
void h8_small_transition_inventory_note_remote_collect(H8ThreadCtx* ctx,
                                                       H8Span* span,
                                                       uint32_t old_free_head);
H8Span* h8_small_transition_inventory_pop(H8ThreadCtx* ctx,
                                          H8OwnerRecord* owner,
                                          uint32_t class_id);
void h8_small_transition_inventory_reset(H8ThreadCtx* ctx);
H8SmallTransitionInventoryStats h8_small_transition_inventory_stats(void);
void h8_small_transition_inventory_dump(void);
#else
static inline void h8_small_transition_inventory_note_local_free(
    H8ThreadCtx* ctx, H8Span* span, uint32_t old_free_head) {
  (void)ctx;
  (void)span;
  (void)old_free_head;
}
static inline void h8_small_transition_inventory_note_remote_collect(
    H8ThreadCtx* ctx, H8Span* span, uint32_t old_free_head) {
  (void)ctx;
  (void)span;
  (void)old_free_head;
}
static inline H8Span* h8_small_transition_inventory_pop(
    H8ThreadCtx* ctx, H8OwnerRecord* owner, uint32_t class_id) {
  (void)ctx;
  (void)owner;
  (void)class_id;
  return (H8Span*)0;
}
static inline void h8_small_transition_inventory_reset(H8ThreadCtx* ctx) {
  (void)ctx;
}
static inline H8SmallTransitionInventoryStats
h8_small_transition_inventory_stats(void) {
  H8SmallTransitionInventoryStats stats = {0};
  return stats;
}
static inline void h8_small_transition_inventory_dump(void) {}
#endif

#endif /* H8_SMALL_TRANSITION_INVENTORY_H */
