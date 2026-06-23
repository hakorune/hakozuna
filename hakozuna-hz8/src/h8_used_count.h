#ifndef H8_USED_COUNT_H
#define H8_USED_COUNT_H

#include "h8_internal.h"

#include <stdlib.h>

static inline size_t h8_used_count_load_owner_relaxed(H8Span* span) {
  return atomic_load_explicit(&span->local_hot.local_used_count,
                              memory_order_relaxed);
}

static inline void h8_used_count_store_owner_relaxed(H8Span* span,
                                                     size_t value) {
  atomic_store_explicit(&span->local_hot.local_used_count, value,
                        memory_order_relaxed);
}

static inline size_t h8_used_count_load_cold_acquire(const H8Span* span) {
#if defined(H8_ENABLE_DEBUG_STATS)
  return atomic_load_explicit(&((H8Span*)span)->local_hot.local_used_count,
                              memory_order_acquire);
#else
  return h8_slot_allocated_count_quiescent((H8Span*)span);
#endif
}

static inline size_t h8_used_count_load_active_hint(const H8Span* span) {
  H8_DEBUG_INC(local_used_cold_active_hint);
  return h8_used_count_load_cold_acquire(span);
}

static inline size_t h8_used_count_load_owner_scan_locked(const H8Span* span) {
  H8_DEBUG_INC(local_used_cold_owner_scan_locked);
  return h8_used_count_load_cold_acquire(span);
}

static inline size_t h8_used_count_load_adoption_locked(const H8Span* span) {
  H8_DEBUG_INC(local_used_cold_adoption_locked);
  return h8_used_count_load_cold_acquire(span);
}

static inline size_t h8_used_count_load_owner_exit(const H8Span* span) {
  H8_DEBUG_INC(local_used_cold_owner_exit);
  return h8_used_count_load_cold_acquire(span);
}

static inline size_t h8_used_count_load_verify_quiescent(const H8Span* span) {
  H8_DEBUG_INC(local_used_cold_verify_quiescent);
  return h8_used_count_load_cold_acquire(span);
}

static inline void h8_used_count_init(H8Span* span) {
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_used_count_store_owner_relaxed(span, 0);
  span->local_hot.local_used_mirror = 0;
#else
  (void)span;
#endif
}

#if defined(H8_ENABLE_DEBUG_STATS)
static inline void h8_used_count_mirror_add(H8Span* span, size_t count) {
  size_t used = span->local_hot.local_used_mirror;
  if (H8_UNLIKELY(used + count > span->slot_count)) {
    H8_DEBUG_INC(local_used_mirror_mismatch);
    abort();
  }
  span->local_hot.local_used_mirror = used + count;
}

static inline bool h8_used_count_mirror_sub(H8Span* span, size_t count) {
  size_t used = span->local_hot.local_used_mirror;
  if (H8_UNLIKELY(used < count)) {
    H8_DEBUG_INC(local_used_mirror_underflow);
    return false;
  }
  span->local_hot.local_used_mirror = used - count;
  return true;
}

static inline void h8_used_count_mirror_check(H8Span* span, size_t used) {
  if (H8_UNLIKELY(span->local_hot.local_used_mirror != used)) {
    H8_DEBUG_INC(local_used_mirror_mismatch);
  }
}
#else
static inline void h8_used_count_mirror_add(H8Span* span, size_t count) {
  (void)span;
  (void)count;
}
static inline bool h8_used_count_mirror_sub(H8Span* span, size_t count) {
  (void)span;
  (void)count;
  return true;
}
static inline void h8_used_count_mirror_check(H8Span* span, size_t used) {
  (void)span;
  (void)used;
}
#endif

#endif
