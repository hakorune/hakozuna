#ifndef HZ11_TRANSFER_CACHE_H
#define HZ11_TRANSFER_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "hz11_span.h"
#include "hz11_size_class.h"

/* HZ11TransferCacheCentralSpan-L1: batch middle-end for the span lane.
 * Replaces the per-object returned-list sink with:
 *   thread cache <-> transfer cache <-> central object stack <-> span batch
 *
 * Only active when HZ11_TRANSFER_CENTRAL_SPAN=1. Compiles to empty stubs when off. */

#ifndef HZ11_TRANSFER_CENTRAL_SPAN
#define HZ11_TRANSFER_CENTRAL_SPAN 0
#endif

#ifndef HZ11_TRANSFER_BATCH
#define HZ11_TRANSFER_BATCH 16u
#endif

#ifndef HZ11_TRANSFER_CAP
#define HZ11_TRANSFER_CAP   1024u
#endif

#ifndef HZ11_CENTRAL_CAP
#define HZ11_CENTRAL_CAP   4096u  /* max slot_count for class-0 (16B) from one 64KiB span */
#endif

#ifndef HZ11_CENTRAL_CLASS_DIAG
#define HZ11_CENTRAL_CLASS_DIAG 0
#endif

#ifndef HZ11_CENTRAL_SPAN_RETURN
#define HZ11_CENTRAL_SPAN_RETURN 0
#endif

#ifndef HZ11_REUSABLE_SPAN_CAP
#define HZ11_REUSABLE_SPAN_CAP 4096u
#endif

#ifndef HZ11_SPAN_SOURCE_DIAG
#define HZ11_SPAN_SOURCE_DIAG 0
#endif

#if HZ11_TRANSFER_CENTRAL_SPAN

/* Batch remove: pop up to max_n objects from the transfer cache.
 * Returns the number actually popped (0 = empty). */
uint32_t hz11_transfer_remove_range(uint8_t class_id, void** out, uint32_t max_n);

/* Batch insert: push up to n objects into the transfer cache.
 * Returns the number actually inserted. Caller must send excess to
 * hz11_central_stack_insert_range. Arena pointers are never sys_free'd. */
uint32_t hz11_transfer_insert_range(uint8_t class_id, void** items, uint32_t n);

/* Central object stack: spill/reuse target when transfer cache is full.
 * Refill must check this BEFORE carving a fresh span. */
uint32_t hz11_central_stack_remove_range(uint8_t class_id, void** out, uint32_t max_n);
void hz11_central_stack_insert_range(uint8_t class_id, void** items, uint32_t n);

uint64_t hz11_transfer_remove_hit_count_load(void);
uint64_t hz11_transfer_remove_miss_count_load(void);
uint64_t hz11_transfer_insert_count_load(void);
uint64_t hz11_transfer_insert_spill_count_load(void);
uint64_t hz11_central_remove_hit_count_load(void);
uint64_t hz11_central_remove_miss_count_load(void);
uint64_t hz11_central_insert_count_load(void);
void hz11_central_stack_dump_class_stats(void);

void* hz11_span_return_pop_reusable_span(uint8_t class_id);
void hz11_span_return_register_active_span(uint8_t class_id, void* base,
                                           uint32_t slot_count);
uint64_t hz11_span_return_count_load(void);
uint64_t hz11_span_reuse_count_load(void);
uint64_t hz11_central_full_span_count_load(void);
uint64_t hz11_central_partial_span_count_load(void);
uint64_t hz11_central_object_count_load(void);
uint64_t hz11_span_return_by_class_load(uint8_t class_id);
uint64_t hz11_central_high_water_by_class_load(uint8_t class_id);

void hz11_span_source_diag_transfer_refill(uint8_t class_id, uint32_t hit);
void hz11_span_source_diag_central_refill(uint8_t class_id, uint32_t hit);
void hz11_span_source_diag_current_exhaust(uint8_t class_id);
void hz11_span_source_diag_current_replace(uint8_t class_id);
void hz11_span_source_diag_arena_carve(uint8_t class_id);
void hz11_span_source_diag_span_reuse(uint8_t class_id);
void hz11_span_source_diag_sweep(uint8_t class_id, uint32_t scanned);
void hz11_span_source_diag_meta_lock(uint8_t class_id);
void hz11_span_source_diag_dump(void);

#else

/* Stubs: compile to nothing when the flag is off. */
static inline uint32_t hz11_transfer_remove_range(uint8_t c, void** o, uint32_t m) {
  (void)c; (void)o; (void)m; return 0u;
}
static inline uint32_t hz11_transfer_insert_range(uint8_t c, void** i, uint32_t n) {
  (void)c; (void)i; (void)n; return 0u;
}
static inline uint32_t hz11_central_stack_remove_range(uint8_t c, void** o, uint32_t m) {
  (void)c; (void)o; (void)m; return 0u;
}
static inline void hz11_central_stack_insert_range(uint8_t c, void** i, uint32_t n) {
  (void)c; (void)i; (void)n;
}
static inline uint64_t hz11_transfer_remove_hit_count_load(void) { return 0u; }
static inline uint64_t hz11_transfer_remove_miss_count_load(void) { return 0u; }
static inline uint64_t hz11_transfer_insert_count_load(void) { return 0u; }
static inline uint64_t hz11_transfer_insert_spill_count_load(void) { return 0u; }
static inline uint64_t hz11_central_remove_hit_count_load(void) { return 0u; }
static inline uint64_t hz11_central_remove_miss_count_load(void) { return 0u; }
static inline uint64_t hz11_central_insert_count_load(void) { return 0u; }
static inline void hz11_central_stack_dump_class_stats(void) {}
static inline void* hz11_span_return_pop_reusable_span(uint8_t c) {
  (void)c; return NULL;
}
static inline void hz11_span_return_register_active_span(uint8_t c, void* b,
                                                         uint32_t s) {
  (void)c; (void)b; (void)s;
}
static inline uint64_t hz11_span_return_count_load(void) { return 0u; }
static inline uint64_t hz11_span_reuse_count_load(void) { return 0u; }
static inline uint64_t hz11_central_full_span_count_load(void) { return 0u; }
static inline uint64_t hz11_central_partial_span_count_load(void) { return 0u; }
static inline uint64_t hz11_central_object_count_load(void) { return 0u; }
static inline uint64_t hz11_span_return_by_class_load(uint8_t c) {
  (void)c; return 0u;
}
static inline uint64_t hz11_central_high_water_by_class_load(uint8_t c) {
  (void)c; return 0u;
}
static inline void hz11_span_source_diag_transfer_refill(uint8_t c, uint32_t h) {
  (void)c; (void)h;
}
static inline void hz11_span_source_diag_central_refill(uint8_t c, uint32_t h) {
  (void)c; (void)h;
}
static inline void hz11_span_source_diag_current_exhaust(uint8_t c) { (void)c; }
static inline void hz11_span_source_diag_current_replace(uint8_t c) { (void)c; }
static inline void hz11_span_source_diag_arena_carve(uint8_t c) { (void)c; }
static inline void hz11_span_source_diag_span_reuse(uint8_t c) { (void)c; }
static inline void hz11_span_source_diag_sweep(uint8_t c, uint32_t s) {
  (void)c; (void)s;
}
static inline void hz11_span_source_diag_meta_lock(uint8_t c) { (void)c; }
static inline void hz11_span_source_diag_dump(void) {}

#endif

#endif /* HZ11_TRANSFER_CACHE_H */
