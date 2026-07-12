#ifndef H8_SMALL_AVAILABLE_INDEX_H
#define H8_SMALL_AVAILABLE_INDEX_H

#include <stdint.h>

typedef struct H8ThreadCtx H8ThreadCtx;
typedef struct H8OwnerRecord H8OwnerRecord;
typedef struct H8Span H8Span;

typedef struct H8SmallAvailableIndexStats {
  uint64_t push;
  uint64_t duplicate;
  uint64_t pop_attempt;
  uint64_t pop_hit;
  uint64_t pop_reject;
  uint64_t reset;
} H8SmallAvailableIndexStats;

#if defined(H8_SMALL_AVAILABLE_INDEX_L1)
void h8_small_available_index_push(H8ThreadCtx* ctx, H8Span* span);
H8Span* h8_small_available_index_pop(H8ThreadCtx* ctx,
                                     H8OwnerRecord* owner,
                                     uint32_t class_id);
void h8_small_available_index_reset(H8ThreadCtx* ctx);
H8SmallAvailableIndexStats h8_small_available_index_stats(void);
void h8_small_available_index_dump(void);
#else
static inline void h8_small_available_index_push(H8ThreadCtx* ctx,
                                                 H8Span* span) {
  (void)ctx;
  (void)span;
}
static inline H8Span* h8_small_available_index_pop(H8ThreadCtx* ctx,
                                                    H8OwnerRecord* owner,
                                                    uint32_t class_id) {
  (void)ctx;
  (void)owner;
  (void)class_id;
  return (H8Span*)0;
}
static inline void h8_small_available_index_reset(H8ThreadCtx* ctx) {
  (void)ctx;
}
static inline H8SmallAvailableIndexStats h8_small_available_index_stats(void) {
  H8SmallAvailableIndexStats stats = {0};
  return stats;
}
static inline void h8_small_available_index_dump(void) {}
#endif

#endif /* H8_SMALL_AVAILABLE_INDEX_H */
