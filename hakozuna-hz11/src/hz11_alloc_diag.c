/* HZ11 allocator diagnostics: source-diag per-class counters + dump.
 * Extracted from hz11_transfer_cache.c (HZ11SourceAttribution-L1 era) for
 * modularity. Self-contained: own counters, own helpers, no references to
 * transfer-cache or central-stack internals. */
#include "hz11_transfer_cache.h"

#if HZ11_TRANSFER_CENTRAL_SPAN

#include <stdatomic.h>
#include <stdio.h>

#if HZ11_SPAN_SOURCE_DIAG
static _Atomic uint64_t hz11_diag_span_create[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_span_reuse[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_current_exhaust[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_current_replace[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_transfer_hit[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_transfer_miss[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_central_hit[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_central_miss[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_arena_carve[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_sweep_count[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_sweep_scanned[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_meta_lock[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_diag_created_high_water[HZ11_CLASS_COUNT];

static void hz11_diag_inc(_Atomic uint64_t* slot) {
  atomic_fetch_add_explicit(slot, 1u, memory_order_relaxed);
}

static void hz11_diag_add(_Atomic uint64_t* slot, uint64_t value) {
  atomic_fetch_add_explicit(slot, value, memory_order_relaxed);
}

static void hz11_diag_atomic_max_u64(_Atomic uint64_t* dst, uint64_t value) {
  uint64_t cur = atomic_load_explicit(dst, memory_order_relaxed);
  while (cur < value &&
         !atomic_compare_exchange_weak_explicit(dst, &cur, value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}
#endif

void hz11_span_source_diag_transfer_refill(uint8_t class_id, uint32_t hit) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  hz11_diag_inc(hit ? &hz11_diag_transfer_hit[class_id]
                    : &hz11_diag_transfer_miss[class_id]);
#else
  (void)class_id; (void)hit;
#endif
}

void hz11_span_source_diag_central_refill(uint8_t class_id, uint32_t hit) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  hz11_diag_inc(hit ? &hz11_diag_central_hit[class_id]
                    : &hz11_diag_central_miss[class_id]);
#else
  (void)class_id; (void)hit;
#endif
}

void hz11_span_source_diag_current_exhaust(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_current_exhaust[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_current_replace(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_current_replace[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_arena_carve(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  uint64_t created =
      atomic_fetch_add_explicit(&hz11_diag_span_create[class_id], 1u,
                                memory_order_relaxed) + 1u;
  hz11_diag_inc(&hz11_diag_arena_carve[class_id]);
  hz11_diag_atomic_max_u64(&hz11_diag_created_high_water[class_id], created);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_span_reuse(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_span_reuse[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_sweep(uint8_t class_id, uint32_t scanned) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id >= HZ11_CLASS_COUNT) return;
  hz11_diag_inc(&hz11_diag_sweep_count[class_id]);
  hz11_diag_add(&hz11_diag_sweep_scanned[class_id], scanned);
#else
  (void)class_id; (void)scanned;
#endif
}

void hz11_span_source_diag_meta_lock(uint8_t class_id) {
#if HZ11_SPAN_SOURCE_DIAG
  if (class_id < HZ11_CLASS_COUNT) hz11_diag_inc(&hz11_diag_meta_lock[class_id]);
#else
  (void)class_id;
#endif
}

void hz11_span_source_diag_dump(void) {
#if HZ11_SPAN_SOURCE_DIAG
  fprintf(stderr, "hz11_span_source_begin\n");
  for (uint32_t class_id = 0u; class_id < HZ11_CLASS_COUNT; ++class_id) {
    uint64_t span_create = atomic_load_explicit(&hz11_diag_span_create[class_id],
                                                memory_order_relaxed);
    uint64_t span_reuse = atomic_load_explicit(&hz11_diag_span_reuse[class_id],
                                               memory_order_relaxed);
    uint64_t current_exhaust =
        atomic_load_explicit(&hz11_diag_current_exhaust[class_id],
                             memory_order_relaxed);
    uint64_t current_replace =
        atomic_load_explicit(&hz11_diag_current_replace[class_id],
                             memory_order_relaxed);
    uint64_t transfer_hit =
        atomic_load_explicit(&hz11_diag_transfer_hit[class_id],
                             memory_order_relaxed);
    uint64_t transfer_miss =
        atomic_load_explicit(&hz11_diag_transfer_miss[class_id],
                             memory_order_relaxed);
    uint64_t central_hit = atomic_load_explicit(&hz11_diag_central_hit[class_id],
                                                memory_order_relaxed);
    uint64_t central_miss =
        atomic_load_explicit(&hz11_diag_central_miss[class_id],
                             memory_order_relaxed);
    uint64_t arena_carve =
        atomic_load_explicit(&hz11_diag_arena_carve[class_id],
                             memory_order_relaxed);
    uint64_t sweep_count =
        atomic_load_explicit(&hz11_diag_sweep_count[class_id],
                             memory_order_relaxed);
    uint64_t sweep_scanned =
        atomic_load_explicit(&hz11_diag_sweep_scanned[class_id],
                             memory_order_relaxed);
    uint64_t meta_lock = atomic_load_explicit(&hz11_diag_meta_lock[class_id],
                                              memory_order_relaxed);
    uint64_t created_high =
        atomic_load_explicit(&hz11_diag_created_high_water[class_id],
                             memory_order_relaxed);
    uint64_t live_current =
        current_replace > current_exhaust ? current_replace - current_exhaust : 0u;
    if (span_create == 0u && span_reuse == 0u && current_exhaust == 0u &&
        current_replace == 0u && transfer_hit == 0u && transfer_miss == 0u &&
        central_hit == 0u && central_miss == 0u && arena_carve == 0u &&
        sweep_count == 0u && sweep_scanned == 0u && meta_lock == 0u) {
      continue;
    }
    fprintf(stderr,
            "hz11_span_source_class class=%u span_create=%llu span_reuse=%llu "
            "current_span_exhaust=%llu current_span_replace=%llu "
            "transfer_refill_hit=%llu transfer_refill_miss=%llu "
            "central_refill_hit=%llu central_refill_miss=%llu "
            "arena_carve=%llu live_current_span=%llu created_high_water=%llu "
            "sweep_count=%llu sweep_scanned=%llu meta_lock=%llu\n",
            (unsigned)class_id, (unsigned long long)span_create,
            (unsigned long long)span_reuse,
            (unsigned long long)current_exhaust,
            (unsigned long long)current_replace,
            (unsigned long long)transfer_hit,
            (unsigned long long)transfer_miss,
            (unsigned long long)central_hit,
            (unsigned long long)central_miss,
            (unsigned long long)arena_carve,
            (unsigned long long)live_current,
            (unsigned long long)created_high,
            (unsigned long long)sweep_count,
            (unsigned long long)sweep_scanned,
            (unsigned long long)meta_lock);
  }
  fprintf(stderr, "hz11_span_source_end\n");
#endif
}

#endif /* HZ11_TRANSFER_CENTRAL_SPAN */
