#ifndef HZ12_SPAN_H
#define HZ12_SPAN_H

#include <stddef.h>
#include <stdint.h>
#include "hz12_size_class.h"

/* HZ12SpanBackedClassify-L1: a 4 GiB virtual arena carved into 64 KiB spans, one
 * class per span, with a flat direct-index classify table. This replaces L0's
 * system-malloc backing + token-hash classify. See
 * docs/no_go/HZ12_SPAN_BACKED_CLASSIFY_L1.md. */

#define HZ12_SPAN_SHIFT 16u
#define HZ12_SPAN_BYTES (1ul << HZ12_SPAN_SHIFT)    /* 64 KiB */
#define HZ12_ARENA_BYTES (1ull << 32)               /* 4 GiB virtual */
#define HZ12_SPAN_COUNT (HZ12_ARENA_BYTES >> HZ12_SPAN_SHIFT) /* 65536 */

#ifndef HZ12_ARENA_NOHUGEPAGE
#define HZ12_ARENA_NOHUGEPAGE 0
#endif

/* State defined in hz12_span.c. */
extern char* hz12_arena_base;
extern uint8_t hz12_span_class[HZ12_SPAN_COUNT];
uint64_t hz12_span_create_count_load(void);
uint64_t hz12_returned_push_count_load(void);
uint64_t hz12_returned_pop_hit_count_load(void);
uint64_t hz12_returned_pop_miss_count_load(void);

void hz12_span_init(void); /* mmap the arena + init the returned sink (once) */

/* Direct-index classify. Returns 1 and sets *class_id for a carved in-arena
 * pointer; 0 (miss) for uncarved / out-of-arena / foreign. ONE dependent load,
 * no hash. (Speed mode: an interior/stale/double-free in-arena pointer would
 * also classify -- valid-C assumption; checked mode catches that later.) */
static inline int hz12_span_classify(const void* ptr, uint8_t* class_id) {
  if (!hz12_arena_base) {
    return 0;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz12_arena_base;
  uintptr_t off = p - base;
  if (p < base || off >= (uintptr_t)HZ12_ARENA_BYTES) {
    return 0;
  }
  uint8_t c1 = hz12_span_class[(size_t)(off >> HZ12_SPAN_SHIFT)];
  if (c1 == 0u) {
    return 0;
  }
  *class_id = (uint8_t)(c1 - 1u);
  return 1;
}

static inline int hz12_arena_contains(const void* ptr) {
  if (!hz12_arena_base) {
    return 0;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz12_arena_base;
  uintptr_t off = p - base;
  return p >= base && off < (uintptr_t)HZ12_ARENA_BYTES;
}

/* Hand out a fresh 64 KiB span for a class (atomic arena bump), stamp its
 * span_class entry. Returns the span base, or NULL if the arena is full. */
void* hz12_span_carve_for_class(uint8_t class_id);

/* Per-class global returned-object sink (the overflow target for arena
 * pointers -- they are NOT system pointers and must never be sys_free'd).
 * Intrusive singly-linked via the object's first word. Mutex-guarded. */
void hz12_returned_push(uint8_t class_id, void* ptr);
void hz12_returned_push_range(uint8_t class_id, void** items, uint32_t count);
void* hz12_returned_pop(uint8_t class_id);
uint32_t hz12_returned_pop_range(uint8_t class_id, void** out, uint32_t max);

/* Diagnostic-only cold query used by the HZ12 reclaim gate. */
uint32_t hz12_returned_count_in_span(uint8_t class_id,
                                     const void* span_base);
typedef struct H12ReturnedSpanSnapshot {
  uint32_t objects;
  uint32_t unique_slots;
  uint32_t duplicate_slots;
  uint32_t invalid_slots;
  uint32_t slot_capacity;
  uint8_t complete;
} H12ReturnedSpanSnapshot;
int hz12_returned_snapshot_span(uint8_t class_id, const void* span_base,
                                H12ReturnedSpanSnapshot* out);
uint32_t hz12_returned_detach_span(uint8_t class_id, const void* span_base);
int hz12_span_route_detach(const void* span_base, uint8_t expected_class_id);
int hz12_span_route_attach(const void* span_base, uint8_t class_id);

#endif /* HZ12_SPAN_H */
