#include "hz11_thread_cache.h"

#include <string.h>

/* ---------- state ---------- */

_Thread_local H11ThreadCache* hz11_tls = NULL;

/* ---------- TLS cache init + slow paths ---------- */

H11ThreadCache* hz11_thread_cache_init_slow(void) {
  void* raw = hz11_sys_malloc(sizeof(H11ThreadCache));
  if (!raw) {
    return NULL;
  }
  H11ThreadCache* tc = (H11ThreadCache*)raw;
  memset(tc, 0, sizeof(*tc));
#if HZ11_CLASSIFY_SPAN
  hz11_span_init(); /* ensure the arena is mapped before any span carve */
#endif
  hz11_tls = tc;
#if HZ11_CACHE_TOPPTR
  for (uint32_t c = 0u; c < HZ11_CLASS_COUNT; ++c) {
    tc->class_cache[c].top = tc->class_cache[c].items;
  }
#endif
  return tc;
}

static void hz11_thread_cache_flush_class(H11ThreadCache* tc, uint8_t class_id) {
#if HZ11_CACHE_SOA
  uint32_t n = tc->class_counts[class_id];
  HZ11_COUNT_ADD(tc->flush_items, n);
  for (uint32_t i = 0u; i < n; ++i) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(tc->class_items[class_id][i])) {
      hz11_returned_push(class_id, tc->class_items[class_id][i]);
    } else {
      hz11_sys_free(tc->class_items[class_id][i]);
    }
#else
    hz11_sys_free(tc->class_items[class_id][i]);
#endif
    tc->class_items[class_id][i] = NULL;
  }
  tc->class_counts[class_id] = 0u;
#else
  H11ClassCache* cc = &tc->class_cache[class_id];
  size_t slot = hz11_class_slot_size(class_id);
#if HZ11_CACHE_TOPPTR
  void** p = cc->items;
  uint32_t n = (uint32_t)(cc->top - cc->items);
  HZ11_COUNT_ADD(tc->flush_items, n);
  while (p < cc->top) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(*p)) {
      hz11_returned_push(class_id, *p);
    } else {
      hz11_sys_free(*p);
    }
#else
    hz11_sys_free(*p);
#endif
    ++p;
  }
  cc->top = cc->items;
  tc->cached_bytes -= slot * n;
#else
  HZ11_COUNT_ADD(tc->flush_items, cc->count);
  for (uint32_t i = 0; i < cc->count; ++i) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(cc->items[i])) {
      hz11_returned_push(class_id, cc->items[i]);
    } else {
      hz11_sys_free(cc->items[i]);
    }
#else
    hz11_sys_free(cc->items[i]);
#endif
    cc->items[i] = NULL;
  }
  tc->cached_bytes -= slot * cc->count;
  cc->count = 0;
#endif
#endif /* HZ11_CACHE_SOA */
  HZ11_COUNT_INC(tc->flush_count);
}

void hz11_thread_cache_push_overflow_slow(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  HZ11_COUNT_INC(tc->overflow_count);
  hz11_thread_cache_flush_class(tc, class_id);
  if (class_id < HZ11_CLASS_COUNT) {
#if HZ11_CACHE_SOA
    tc->class_items[class_id][0] = ptr;
    tc->class_counts[class_id] = 1u;
#else
    H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
    *cc->top++ = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
#else
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
#endif
#endif /* HZ11_CACHE_SOA */
  } else {
    hz11_sys_free(ptr);
  }
}

void* hz11_thread_cache_refill(H11ThreadCache* tc, uint8_t class_id) {
#if !HZ11_CLASSIFY_SPAN && !HZ11_ENABLE_HOT_COUNTERS
  (void)tc; /* token lane: tc only used for the now-compiled-out refill_count */
#endif
  HZ11_COUNT_INC(tc->refill_count);
#if HZ11_CLASSIFY_SPAN
  /* 1. per-class returned-object sink first (reuse before carving a fresh span) */
  void* reused = hz11_returned_pop(class_id);
  if (reused) {
    return reused;
  }
  /* 2. bump from the per-thread current span */
  size_t slot = hz11_class_slot_size(class_id);
  H11SpanCurrent* cs = &tc->current[class_id];
  if (cs->base && cs->bump_index < cs->slot_count) {
    return cs->base + (size_t)cs->bump_index++ * slot;
  }
  /* 3. current span exhausted (or none yet): carve a fresh 64 KiB span */
  char* base = (char*)hz11_span_carve_for_class(class_id);
  if (!base) {
    return hz11_sys_malloc(slot); /* arena full -- fall back (bench never hits) */
  }
  cs->base = base;
  cs->bump_index = 0;
  cs->slot_count = (uint32_t)(HZ11_SPAN_BYTES / slot);
  return cs->base + (size_t)cs->bump_index++ * slot;
#else
  return hz11_sys_malloc(hz11_class_slot_size(class_id));
#endif
}
