#ifndef HZ11_SPAN_H
#define HZ11_SPAN_H

#include <stddef.h>
#include <stdint.h>
#include "hz11_size_class.h"

/* HZ11SpanBackedClassify-L1: a 4 GiB virtual arena carved into 64 KiB spans, one
 * class per span, with a flat direct-index classify table. This replaces L0's
 * system-malloc backing + token-hash classify. See
 * docs/HZ11_SPAN_BACKED_CLASSIFY_L1.md. */

#define HZ11_SPAN_SHIFT 16u
#define HZ11_SPAN_BYTES (1ul << HZ11_SPAN_SHIFT)    /* 64 KiB */
#define HZ11_ARENA_BYTES (1ull << 32)               /* 4 GiB virtual */
#define HZ11_SPAN_COUNT (HZ11_ARENA_BYTES >> HZ11_SPAN_SHIFT) /* 65536 */

/* State defined in hz11_span.c. */
extern char* hz11_arena_base;
extern uint8_t hz11_span_class[HZ11_SPAN_COUNT];
uint64_t hz11_span_create_count_load(void);

void hz11_span_init(void); /* mmap the arena + init the returned sink (once) */

/* Direct-index classify. Returns 1 and sets *class_id for a carved in-arena
 * pointer; 0 (miss) for uncarved / out-of-arena / foreign. ONE dependent load,
 * no hash. (Speed mode: an interior/stale/double-free in-arena pointer would
 * also classify -- valid-C assumption; checked mode catches that later.) */
static inline int hz11_span_classify(const void* ptr, uint8_t* class_id) {
  if (!hz11_arena_base) {
    return 0;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz11_arena_base;
  uintptr_t off = p - base;
  if (p < base || off >= (uintptr_t)HZ11_ARENA_BYTES) {
    return 0;
  }
  uint8_t c1 = hz11_span_class[(size_t)(off >> HZ11_SPAN_SHIFT)];
  if (c1 == 0u) {
    return 0;
  }
  *class_id = (uint8_t)(c1 - 1u);
  return 1;
}

static inline int hz11_arena_contains(const void* ptr) {
  if (!hz11_arena_base) {
    return 0;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz11_arena_base;
  uintptr_t off = p - base;
  return p >= base && off < (uintptr_t)HZ11_ARENA_BYTES;
}

/* Hand out a fresh 64 KiB span for a class (atomic arena bump), stamp its
 * span_class entry. Returns the span base, or NULL if the arena is full. */
void* hz11_span_carve_for_class(uint8_t class_id);

/* Per-class global returned-object sink (the overflow target for arena
 * pointers -- they are NOT system pointers and must never be sys_free'd).
 * Intrusive singly-linked via the object's first word. Mutex-guarded. */
void hz11_returned_push(uint8_t class_id, void* ptr);
void* hz11_returned_pop(uint8_t class_id);

#endif /* HZ11_SPAN_H */
