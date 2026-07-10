#include "hz12_class_diag.h"
#include "hz12_size_class.h"

#include <stdatomic.h>
#include <stdio.h>

#if HZ12_CLASS_DIAG
typedef struct H12ClassDiagCounter {
  _Atomic uint64_t malloc_count;
  _Atomic uint64_t hit_count;
  _Atomic uint64_t refill_count;
  _Atomic uint64_t overflow_count;
  _Atomic uint64_t returned_pop_hit;
  _Atomic uint64_t returned_pop_miss;
  _Atomic uint64_t returned_push_range_calls;
  _Atomic uint64_t returned_push_range_items;
  _Atomic int64_t returned_sink_depth;
  _Atomic uint64_t returned_sink_depth_max;
} H12ClassDiagCounter;

static H12ClassDiagCounter hz12_class_diag_counters[HZ12_CLASS_COUNT];

static void hz12_class_diag_inc(_Atomic uint64_t* value) {
  atomic_fetch_add_explicit(value, 1u, memory_order_relaxed);
}

void hz12_class_diag_malloc(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_class_diag_inc(&hz12_class_diag_counters[class_id].malloc_count);
  }
}

void hz12_class_diag_hit(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_class_diag_inc(&hz12_class_diag_counters[class_id].hit_count);
  }
}

void hz12_class_diag_refill(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_class_diag_inc(&hz12_class_diag_counters[class_id].refill_count);
  }
}

void hz12_class_diag_overflow(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_class_diag_inc(&hz12_class_diag_counters[class_id].overflow_count);
  }
}

void hz12_class_diag_returned_pop_hit(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_class_diag_inc(&hz12_class_diag_counters[class_id].returned_pop_hit);
  }
}

void hz12_class_diag_returned_pop_miss(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_class_diag_inc(&hz12_class_diag_counters[class_id].returned_pop_miss);
  }
}

void hz12_class_diag_returned_push_range(uint8_t class_id, uint32_t count) {
  if (class_id < HZ12_CLASS_COUNT && count > 0u) {
    hz12_class_diag_inc(
        &hz12_class_diag_counters[class_id].returned_push_range_calls);
    atomic_fetch_add_explicit(
        &hz12_class_diag_counters[class_id].returned_push_range_items, count,
        memory_order_relaxed);
  }
}

void hz12_class_diag_returned_sink_depth(uint8_t class_id, int32_t delta) {
  if (class_id >= HZ12_CLASS_COUNT || delta == 0) {
    return;
  }
  H12ClassDiagCounter* c = &hz12_class_diag_counters[class_id];
  int64_t depth = atomic_fetch_add_explicit(&c->returned_sink_depth, delta,
                                            memory_order_relaxed) + delta;
  if (depth > 0) {
    uint64_t sample = (uint64_t)depth;
    uint64_t old = atomic_load_explicit(&c->returned_sink_depth_max,
                                        memory_order_relaxed);
    while (old < sample &&
           !atomic_compare_exchange_weak_explicit(
               &c->returned_sink_depth_max, &old, sample,
               memory_order_relaxed, memory_order_relaxed)) {
    }
  }
}

void hz12_class_diag_dump_stats(void) {
  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    uint64_t malloc_count = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].malloc_count, memory_order_relaxed);
    uint64_t hit_count = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].hit_count, memory_order_relaxed);
    uint64_t refill_count = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].refill_count, memory_order_relaxed);
    uint64_t overflow_count = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].overflow_count, memory_order_relaxed);
    uint64_t returned_pop_hit = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].returned_pop_hit,
        memory_order_relaxed);
    uint64_t returned_pop_miss = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].returned_pop_miss,
        memory_order_relaxed);
    uint64_t returned_push_range_calls = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].returned_push_range_calls,
        memory_order_relaxed);
    uint64_t returned_push_range_items = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].returned_push_range_items,
        memory_order_relaxed);
    int64_t returned_sink_depth = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].returned_sink_depth,
        memory_order_relaxed);
    uint64_t returned_sink_depth_max = atomic_load_explicit(
        &hz12_class_diag_counters[class_id].returned_sink_depth_max,
        memory_order_relaxed);
    if (malloc_count == 0u && hit_count == 0u && refill_count == 0u &&
        overflow_count == 0u && returned_pop_hit == 0u &&
        returned_pop_miss == 0u && returned_push_range_calls == 0u &&
        returned_push_range_items == 0u && returned_sink_depth == 0 &&
        returned_sink_depth_max == 0u) {
      continue;
    }
    fprintf(stdout,
            "[HZ12_CLASS_DIAG] class=%u slot=%zu malloc=%llu hit=%llu "
            "refill=%llu overflow=%llu returned_pop_hit=%llu "
            "returned_pop_miss=%llu returned_push_range_calls=%llu "
            "returned_push_range_items=%llu returned_sink_depth=%lld "
            "returned_sink_depth_max=%llu\n",
            (unsigned)class_id, hz12_class_slot_size((uint8_t)class_id),
            (unsigned long long)malloc_count,
            (unsigned long long)hit_count,
            (unsigned long long)refill_count,
            (unsigned long long)overflow_count,
            (unsigned long long)returned_pop_hit,
            (unsigned long long)returned_pop_miss,
            (unsigned long long)returned_push_range_calls,
            (unsigned long long)returned_push_range_items,
            (long long)returned_sink_depth,
            (unsigned long long)returned_sink_depth_max);
  }
}
#endif

#if HZ12_MATRIX_ATTRIB_DIAG
typedef struct H12MatrixDiagCounter {
  _Atomic uint64_t cache_refill_samples;
  _Atomic uint64_t cache_count_at_refill_total;
  _Atomic uint64_t cache_count_at_refill_max;
  _Atomic uint64_t cache_after_batch_samples;
  _Atomic uint64_t cache_count_after_batch_total;
  _Atomic uint64_t cache_count_after_batch_max;
  _Atomic uint64_t returned_one_hit;
  _Atomic uint64_t returned_one_miss;
  _Atomic uint64_t returned_batch_call;
  _Atomic uint64_t returned_batch_items;
  _Atomic uint64_t returned_batch_miss;
  _Atomic uint64_t current_hit;
  _Atomic uint64_t bump_batch_calls;
  _Atomic uint64_t bump_batch_items;
  _Atomic uint64_t bump_batch_max;
  _Atomic uint64_t span_new;
  _Atomic uint64_t sys_fallback;
} H12MatrixDiagCounter;

static H12MatrixDiagCounter hz12_matrix_diag_counters[HZ12_CLASS_COUNT];

static void hz12_matrix_diag_inc(_Atomic uint64_t* value) {
  atomic_fetch_add_explicit(value, 1u, memory_order_relaxed);
}

static void hz12_matrix_diag_add(_Atomic uint64_t* value, uint64_t amount) {
  atomic_fetch_add_explicit(value, amount, memory_order_relaxed);
}

static void hz12_matrix_diag_max(_Atomic uint64_t* value, uint64_t sample) {
  uint64_t old = atomic_load_explicit(value, memory_order_relaxed);
  while (old < sample &&
         !atomic_compare_exchange_weak_explicit(value, &old, sample,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}

void hz12_matrix_diag_cache_at_refill(uint8_t class_id, uint32_t count) {
  if (class_id >= HZ12_CLASS_COUNT) return;
  H12MatrixDiagCounter* c = &hz12_matrix_diag_counters[class_id];
  hz12_matrix_diag_inc(&c->cache_refill_samples);
  hz12_matrix_diag_add(&c->cache_count_at_refill_total, count);
  hz12_matrix_diag_max(&c->cache_count_at_refill_max, count);
}

void hz12_matrix_diag_cache_after_batch(uint8_t class_id, uint32_t count) {
  if (class_id >= HZ12_CLASS_COUNT) return;
  H12MatrixDiagCounter* c = &hz12_matrix_diag_counters[class_id];
  hz12_matrix_diag_inc(&c->cache_after_batch_samples);
  hz12_matrix_diag_add(&c->cache_count_after_batch_total, count);
  hz12_matrix_diag_max(&c->cache_count_after_batch_max, count);
}

void hz12_matrix_diag_returned_one(uint8_t class_id, int hit) {
  if (class_id >= HZ12_CLASS_COUNT) return;
  H12MatrixDiagCounter* c = &hz12_matrix_diag_counters[class_id];
  hz12_matrix_diag_inc(hit ? &c->returned_one_hit : &c->returned_one_miss);
}

void hz12_matrix_diag_returned_batch(uint8_t class_id, uint32_t count) {
  if (class_id >= HZ12_CLASS_COUNT) return;
  H12MatrixDiagCounter* c = &hz12_matrix_diag_counters[class_id];
  hz12_matrix_diag_inc(&c->returned_batch_call);
  if (count == 0u) {
    hz12_matrix_diag_inc(&c->returned_batch_miss);
  } else {
    hz12_matrix_diag_add(&c->returned_batch_items, count);
  }
}

void hz12_matrix_diag_current_hit(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_matrix_diag_inc(&hz12_matrix_diag_counters[class_id].current_hit);
  }
}

void hz12_matrix_diag_bump_batch(uint8_t class_id, uint32_t count) {
  if (class_id >= HZ12_CLASS_COUNT || count <= 1u) return;
  H12MatrixDiagCounter* c = &hz12_matrix_diag_counters[class_id];
  hz12_matrix_diag_inc(&c->bump_batch_calls);
  hz12_matrix_diag_add(&c->bump_batch_items, count - 1u);
  hz12_matrix_diag_max(&c->bump_batch_max, count);
}

void hz12_matrix_diag_span_new(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_matrix_diag_inc(&hz12_matrix_diag_counters[class_id].span_new);
  }
}

void hz12_matrix_diag_sys_fallback(uint8_t class_id) {
  if (class_id < HZ12_CLASS_COUNT) {
    hz12_matrix_diag_inc(&hz12_matrix_diag_counters[class_id].sys_fallback);
  }
}

void hz12_matrix_diag_dump_stats(void) {
  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    H12MatrixDiagCounter* c = &hz12_matrix_diag_counters[class_id];
    uint64_t samples = atomic_load_explicit(&c->cache_refill_samples,
                                            memory_order_relaxed);
    uint64_t at_total = atomic_load_explicit(&c->cache_count_at_refill_total,
                                             memory_order_relaxed);
    uint64_t at_max = atomic_load_explicit(&c->cache_count_at_refill_max,
                                           memory_order_relaxed);
    uint64_t after_samples = atomic_load_explicit(&c->cache_after_batch_samples,
                                                  memory_order_relaxed);
    uint64_t after_total = atomic_load_explicit(&c->cache_count_after_batch_total,
                                                memory_order_relaxed);
    uint64_t after_max = atomic_load_explicit(&c->cache_count_after_batch_max,
                                              memory_order_relaxed);
    uint64_t one_hit = atomic_load_explicit(&c->returned_one_hit,
                                            memory_order_relaxed);
    uint64_t one_miss = atomic_load_explicit(&c->returned_one_miss,
                                             memory_order_relaxed);
    uint64_t batch_call = atomic_load_explicit(&c->returned_batch_call,
                                               memory_order_relaxed);
    uint64_t batch_items = atomic_load_explicit(&c->returned_batch_items,
                                                memory_order_relaxed);
    uint64_t batch_miss = atomic_load_explicit(&c->returned_batch_miss,
                                               memory_order_relaxed);
    uint64_t current_hit = atomic_load_explicit(&c->current_hit,
                                                memory_order_relaxed);
    uint64_t bump_calls = atomic_load_explicit(&c->bump_batch_calls,
                                               memory_order_relaxed);
    uint64_t bump_items = atomic_load_explicit(&c->bump_batch_items,
                                               memory_order_relaxed);
    uint64_t bump_max = atomic_load_explicit(&c->bump_batch_max,
                                             memory_order_relaxed);
    uint64_t span_new = atomic_load_explicit(&c->span_new,
                                             memory_order_relaxed);
    uint64_t sys_fallback = atomic_load_explicit(&c->sys_fallback,
                                                 memory_order_relaxed);
    if (samples == 0u && after_samples == 0u && one_hit == 0u &&
        one_miss == 0u && batch_call == 0u && batch_items == 0u &&
        batch_miss == 0u && current_hit == 0u && bump_calls == 0u &&
        bump_items == 0u && span_new == 0u &&
        sys_fallback == 0u) {
      continue;
    }
    fprintf(stdout,
            "[HZ12_MATRIX_ATTRIB] class=%u slot=%zu refill_samples=%llu "
            "cache_at_total=%llu cache_at_max=%llu after_batch_samples=%llu "
            "cache_after_total=%llu cache_after_max=%llu returned_one_hit=%llu "
            "returned_one_miss=%llu returned_batch_call=%llu "
            "returned_batch_items=%llu returned_batch_miss=%llu "
            "current_hit=%llu bump_batch_calls=%llu "
            "bump_batch_items=%llu bump_batch_max=%llu span_new=%llu "
            "sys_fallback=%llu\n",
            (unsigned)class_id, hz12_class_slot_size((uint8_t)class_id),
            (unsigned long long)samples, (unsigned long long)at_total,
            (unsigned long long)at_max, (unsigned long long)after_samples,
            (unsigned long long)after_total, (unsigned long long)after_max,
            (unsigned long long)one_hit, (unsigned long long)one_miss,
            (unsigned long long)batch_call, (unsigned long long)batch_items,
            (unsigned long long)batch_miss, (unsigned long long)current_hit,
            (unsigned long long)bump_calls, (unsigned long long)bump_items,
            (unsigned long long)bump_max, (unsigned long long)span_new,
            (unsigned long long)sys_fallback);
  }
}
#endif

