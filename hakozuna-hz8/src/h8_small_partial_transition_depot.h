#ifndef H8_SMALL_PARTIAL_TRANSITION_DEPOT_H
#define H8_SMALL_PARTIAL_TRANSITION_DEPOT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct H8ThreadCtx H8ThreadCtx;
typedef struct H8OwnerRecord H8OwnerRecord;
typedef struct H8Span H8Span;

#if defined(H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1)
void h8_small_partial_depot_push(H8ThreadCtx* ctx, H8Span* span);
H8Span* h8_small_partial_depot_pop(H8ThreadCtx* ctx,
                                   H8OwnerRecord* owner,
                                   uint32_t class_id);
void h8_small_partial_depot_reset(H8ThreadCtx* ctx);
void h8_small_partial_depot_discard(H8ThreadCtx* ctx,
                                     H8Span* span,
                                     uint32_t class_id);
#else
static inline void h8_small_partial_depot_push(H8ThreadCtx* ctx,
                                                H8Span* span) {
  (void)ctx;
  (void)span;
}
static inline H8Span* h8_small_partial_depot_pop(H8ThreadCtx* ctx,
                                                  H8OwnerRecord* owner,
                                                  uint32_t class_id) {
  (void)ctx;
  (void)owner;
  (void)class_id;
  return (H8Span*)0;
}
static inline void h8_small_partial_depot_reset(H8ThreadCtx* ctx) {
  (void)ctx;
}
static inline void h8_small_partial_depot_discard(H8ThreadCtx* ctx,
                                                   H8Span* span,
                                                   uint32_t class_id) {
  (void)ctx;
  (void)span;
  (void)class_id;
}
#endif

#if defined(H8_SMALL_PARTIAL_TRANSITION_DEPOT_DIAG)
void h8_small_partial_depot_note_transition(uint32_t class_id);
void h8_small_partial_depot_note_mag_probe(uint32_t class_id,
                                           uint32_t steps,
                                           bool hit);
void h8_small_partial_depot_commit_checkpoint(H8ThreadCtx* ctx,
                                               uint32_t class_id);
void h8_small_partial_depot_dump(void);
#else
static inline void h8_small_partial_depot_note_transition(uint32_t class_id) {
  (void)class_id;
}
static inline void h8_small_partial_depot_note_mag_probe(uint32_t class_id,
                                                          uint32_t steps,
                                                          bool hit) {
  (void)class_id;
  (void)steps;
  (void)hit;
}
static inline void h8_small_partial_depot_commit_checkpoint(
    H8ThreadCtx* ctx, uint32_t class_id) {
  (void)ctx;
  (void)class_id;
}
static inline void h8_small_partial_depot_dump(void) {}
#endif

#endif /* H8_SMALL_PARTIAL_TRANSITION_DEPOT_H */
