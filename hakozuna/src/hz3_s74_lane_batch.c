// hz3_s74_lane_batch.c - S74 Lane Batch stats (refill/flush)
#include "hz3_s74_lane_batch.h"

#if HZ3_S74_LANE_BATCH && HZ3_S74_STATS

#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

static _Atomic uint64_t g_s74_refill_calls;
static _Atomic uint64_t g_s74_refill_burst_total;
static _Atomic uint64_t g_s74_refill_burst_max;
static _Atomic uint64_t g_s74_flush_calls;
static _Atomic uint64_t g_s74_flush_batch_total;
static _Atomic uint64_t g_s74_flush_batch_max;
static _Atomic uint64_t g_s74_lease_calls;

static pthread_once_t g_s74_atexit_once = PTHREAD_ONCE_INIT;

static void hz3_s74_atomic_max(_Atomic uint64_t* dst, uint64_t val) {
    uint64_t cur = atomic_load_explicit(dst, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                dst, &cur, val,
                memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_s74_atexit_dump(void) {
    uint64_t refill_calls = atomic_load_explicit(&g_s74_refill_calls, memory_order_relaxed);
    uint64_t refill_total = atomic_load_explicit(&g_s74_refill_burst_total, memory_order_relaxed);
    uint64_t refill_max = atomic_load_explicit(&g_s74_refill_burst_max, memory_order_relaxed);
    uint64_t flush_calls = atomic_load_explicit(&g_s74_flush_calls, memory_order_relaxed);
    uint64_t flush_total = atomic_load_explicit(&g_s74_flush_batch_total, memory_order_relaxed);
    uint64_t flush_max = atomic_load_explicit(&g_s74_flush_batch_max, memory_order_relaxed);
    uint64_t lease_calls = atomic_load_explicit(&g_s74_lease_calls, memory_order_relaxed);

    if (refill_calls == 0 && flush_calls == 0 && lease_calls == 0) {
        return;
    }

    fprintf(stderr,
            "[HZ3_S74] refill_calls=%" PRIu64 " refill_burst_total=%" PRIu64 " refill_burst_max=%" PRIu64
            " flush_calls=%" PRIu64 " flush_batch_total=%" PRIu64 " flush_batch_max=%" PRIu64
            " lease_calls=%" PRIu64 "\n",
            refill_calls, refill_total, refill_max,
            flush_calls, flush_total, flush_max,
            lease_calls);
}

static void hz3_s74_register_atexit(void) {
    atexit(hz3_s74_atexit_dump);
}

static inline void hz3_s74_stats_register_atexit(void) {
    pthread_once(&g_s74_atexit_once, hz3_s74_register_atexit);
}

void hz3_s74_stats_refill(uint32_t burst) {
    if (burst == 0) {
        return;
    }
    hz3_s74_stats_register_atexit();
    atomic_fetch_add_explicit(&g_s74_refill_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s74_refill_burst_total, burst, memory_order_relaxed);
    hz3_s74_atomic_max(&g_s74_refill_burst_max, burst);
}

void hz3_s74_stats_flush(uint32_t batch) {
    if (batch == 0) {
        return;
    }
    hz3_s74_stats_register_atexit();
    atomic_fetch_add_explicit(&g_s74_flush_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s74_flush_batch_total, batch, memory_order_relaxed);
    hz3_s74_atomic_max(&g_s74_flush_batch_max, batch);
}

void hz3_s74_stats_lease_call(void) {
    hz3_s74_stats_register_atexit();
    atomic_fetch_add_explicit(&g_s74_lease_calls, 1, memory_order_relaxed);
}

#endif  // HZ3_S74_LANE_BATCH && HZ3_S74_STATS
