#ifndef HZ11_TRANSFER_CACHE_H
#define HZ11_TRANSFER_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include "hz11_size_class.h"

/* HZ11TransferCacheCentralSpan-L1: batch middle-end for the span lane.
 * Replaces the per-object returned-list sink with:
 *   thread cache <-> transfer cache <-> central object stack <-> span batch
 *
 * Only active when HZ11_TRANSFER_CENTRAL_SPAN=1. Compiles to empty stubs when off. */

#ifndef HZ11_TRANSFER_CENTRAL_SPAN
#define HZ11_TRANSFER_CENTRAL_SPAN 0
#endif

#define HZ11_TRANSFER_BATCH 16u
#define HZ11_TRANSFER_CAP   1024u
#define HZ11_CENTRAL_CAP   4096u  /* max slot_count for class-0 (16B) from one 64KiB span */

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

#endif

#endif /* HZ11_TRANSFER_CACHE_H */
