#ifndef H8_USED_COUNT_H
#define H8_USED_COUNT_H

#include "h8_internal.h"

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
  return atomic_load_explicit(&((H8Span*)span)->local_hot.local_used_count,
                              memory_order_acquire);
}

static inline void h8_used_count_init(H8Span* span) {
  h8_used_count_store_owner_relaxed(span, 0);
}

#endif
