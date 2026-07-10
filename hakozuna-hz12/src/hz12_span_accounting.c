#include "hz12_span_accounting.h"

#include "hz12_span.h"

#include <stdatomic.h>

static _Atomic uint32_t h12_tracked_alloc[HZ12_SPAN_COUNT];
static _Atomic uint32_t h12_tracked_free[HZ12_SPAN_COUNT];
static _Atomic uint32_t h12_tracked_live[HZ12_SPAN_COUNT];
static _Atomic uint32_t h12_tracked_carved_highwater[HZ12_SPAN_COUNT];
static _Atomic uint8_t h12_tracked_class_plus_one[HZ12_SPAN_COUNT];
static _Atomic uint64_t h12_release_untracked;
static _Atomic uint64_t h12_release_underflow;
static _Atomic uint64_t h12_accounting_candidates;

static int h12_accounting_span_id(const void* ptr, uint32_t* out_id) {
  uintptr_t p;
  uintptr_t base;
  uintptr_t off;
  if (!ptr || !out_id || !hz12_arena_base) return 0;
  p = (uintptr_t)ptr;
  base = (uintptr_t)hz12_arena_base;
  if (p < base) return 0;
  off = p - base;
  if (off >= (uintptr_t)HZ12_ARENA_BYTES) return 0;
  *out_id = (uint32_t)(off >> HZ12_SPAN_SHIFT);
  return 1;
}

void h12_span_accounting_reset(void) {
  uint32_t i;
  for (i = 0u; i < HZ12_SPAN_COUNT; ++i) {
    atomic_store_explicit(&h12_tracked_alloc[i], 0u, memory_order_relaxed);
    atomic_store_explicit(&h12_tracked_free[i], 0u, memory_order_relaxed);
    atomic_store_explicit(&h12_tracked_live[i], 0u, memory_order_relaxed);
    atomic_store_explicit(&h12_tracked_carved_highwater[i], 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&h12_tracked_class_plus_one[i], 0u,
                          memory_order_relaxed);
  }
  atomic_store_explicit(&h12_release_untracked, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_release_underflow, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_accounting_candidates, 0u,
                        memory_order_relaxed);
}

void h12_span_accounting_on_alloc(void* ptr) {
  uint32_t span_id;
  uint8_t class_plus_one;
  if (!h12_accounting_span_id(ptr, &span_id)) return;
  class_plus_one = hz12_span_class[span_id];
  if (class_plus_one != 0u) {
    size_t slot_bytes;
    uint32_t carved;
    uint32_t current;
    atomic_store_explicit(&h12_tracked_class_plus_one[span_id],
                          class_plus_one, memory_order_relaxed);
    slot_bytes = hz12_class_slot_size((uint8_t)(class_plus_one - 1u));
    carved = slot_bytes != 0u
        ? (uint32_t)((((uintptr_t)ptr - (uintptr_t)hz12_arena_base) &
                      (HZ12_SPAN_BYTES - 1u)) / slot_bytes) + 1u
        : 0u;
    current = atomic_load_explicit(&h12_tracked_carved_highwater[span_id],
                                   memory_order_relaxed);
    while (current < carved &&
           !atomic_compare_exchange_weak_explicit(
               &h12_tracked_carved_highwater[span_id], &current, carved,
               memory_order_relaxed, memory_order_relaxed)) {
    }
  }
  atomic_fetch_add_explicit(&h12_tracked_alloc[span_id], 1u,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h12_tracked_live[span_id], 1u,
                            memory_order_relaxed);
}

void h12_span_accounting_on_release(void* ptr) {
  uint32_t span_id;
  uint32_t current;
  if (!h12_accounting_span_id(ptr, &span_id)) return;
  if (atomic_load_explicit(&h12_tracked_alloc[span_id],
                           memory_order_relaxed) == 0u) {
    atomic_fetch_add_explicit(&h12_release_untracked, 1u,
                              memory_order_relaxed);
    return;
  }
  current = atomic_load_explicit(&h12_tracked_live[span_id],
                                 memory_order_relaxed);
  while (current != 0u) {
    if (atomic_compare_exchange_weak_explicit(&h12_tracked_live[span_id],
                                              &current, current - 1u,
                                              memory_order_relaxed,
                                              memory_order_relaxed)) {
      atomic_fetch_add_explicit(&h12_tracked_free[span_id], 1u,
                                memory_order_relaxed);
      return;
    }
  }
  atomic_fetch_add_explicit(&h12_release_underflow, 1u,
                            memory_order_relaxed);
}

H12WholeSpanShadow h12_span_accounting_scan(void) {
  H12WholeSpanShadow result = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
  uint32_t span_id;
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT; ++span_id) {
    uint32_t allocated = atomic_load_explicit(&h12_tracked_alloc[span_id],
                                              memory_order_relaxed);
    uint32_t released;
    uint32_t live;
    uint8_t class_plus_one;
    size_t slot_bytes;
    uint32_t slot_capacity;
    if (allocated == 0u) continue;
    released = atomic_load_explicit(&h12_tracked_free[span_id],
                                    memory_order_relaxed);
    live = atomic_load_explicit(&h12_tracked_live[span_id],
                                memory_order_relaxed);
    result.tracked_spans += 1u;
    result.tracked_allocations += allocated;
    result.tracked_releases += released;
    result.tracked_live_objects += live;
    class_plus_one = atomic_load_explicit(
        &h12_tracked_class_plus_one[span_id], memory_order_relaxed);
    if (class_plus_one == 0u) continue;
    slot_bytes = hz12_class_slot_size((uint8_t)(class_plus_one - 1u));
    if (slot_bytes == 0u) continue;
    slot_capacity = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
    if (atomic_load_explicit(&h12_tracked_carved_highwater[span_id],
                             memory_order_relaxed) == slot_capacity) {
      result.full_capacity_spans += 1u;
      if (released == allocated && live == 0u) {
        result.accounting_candidates += 1u;
      }
    }
  }
  result.release_untracked = atomic_load_explicit(&h12_release_untracked,
                                                  memory_order_relaxed);
  result.release_underflow = atomic_load_explicit(&h12_release_underflow,
                                                  memory_order_relaxed);
  atomic_store_explicit(&h12_accounting_candidates,
                        result.accounting_candidates, memory_order_relaxed);
  return result;
}

int h12_span_accounting_query(const void* ptr,
                              H12SpanAccountingRecord* out) {
  uint32_t span_id;
  uint8_t class_plus_one;
  size_t slot_bytes;
  if (!out || !h12_accounting_span_id(ptr, &span_id)) return 0;
  class_plus_one = atomic_load_explicit(
      &h12_tracked_class_plus_one[span_id], memory_order_relaxed);
  if (class_plus_one == 0u) return 0;
  slot_bytes = hz12_class_slot_size((uint8_t)(class_plus_one - 1u));
  if (slot_bytes == 0u) return 0;
  out->span_id = span_id;
  out->class_id = (uint8_t)(class_plus_one - 1u);
  out->slot_capacity = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  out->allocated = atomic_load_explicit(&h12_tracked_alloc[span_id],
                                        memory_order_relaxed);
  out->released = atomic_load_explicit(&h12_tracked_free[span_id],
                                       memory_order_relaxed);
  out->live = atomic_load_explicit(&h12_tracked_live[span_id],
                                   memory_order_relaxed);
  out->carved_slots = atomic_load_explicit(
      &h12_tracked_carved_highwater[span_id], memory_order_relaxed);
  out->accounting_candidate =
      (uint8_t)(out->carved_slots == out->slot_capacity &&
                out->released == out->allocated && out->live == 0u);
  return 1;
}

int h12_span_accounting_recycle(const void* ptr, uint8_t class_id) {
  uint32_t span_id;
  if (class_id >= HZ12_CLASS_COUNT ||
      !h12_accounting_span_id(ptr, &span_id)) {
    return 0;
  }
  atomic_store_explicit(&h12_tracked_alloc[span_id], 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_tracked_free[span_id], 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_tracked_live[span_id], 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_tracked_carved_highwater[span_id], 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_tracked_class_plus_one[span_id],
                        (uint8_t)(class_id + 1u), memory_order_relaxed);
  return 1;
}

void h12_span_accounting_dump(FILE* out) {
  H12WholeSpanShadow result;
  if (!out) return;
  result = h12_span_accounting_scan();
  fprintf(out,
          "[HZ12_SPAN_ACCOUNTING] tracked_spans=%u full_capacity_spans=%u "
          "accounting_candidates=%u alloc=%llu free=%llu live=%llu "
          "release_untracked=%llu release_underflow=%llu\n",
          result.tracked_spans, result.full_capacity_spans,
          result.accounting_candidates,
          (unsigned long long)result.tracked_allocations,
          (unsigned long long)result.tracked_releases,
          (unsigned long long)result.tracked_live_objects,
          (unsigned long long)result.release_untracked,
          (unsigned long long)result.release_underflow);
}
