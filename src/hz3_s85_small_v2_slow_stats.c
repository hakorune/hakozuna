#include "hz3_s85_small_v2_slow_stats.h"

#if HZ3_S85_SMALL_V2_SLOW_STATS && HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

#include "hz3_small.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    _Atomic uint64_t calls;
    _Atomic uint64_t xfer_first;
    _Atomic uint64_t stash_first;
    _Atomic uint64_t central_first;
    _Atomic uint64_t page_first;

    _Atomic uint64_t xfer_got;
    _Atomic uint64_t stash_got;
    _Atomic uint64_t central_got;

    _Atomic uint64_t page_alloc_attempt;
    _Atomic uint64_t page_alloc_fail;
} Hz3S85ScStats;

static Hz3S85ScStats g_s85_sc[HZ3_SMALL_NUM_SC];
static pthread_once_t g_s85_once = PTHREAD_ONCE_INIT;

static void hz3_s85_dump(void) {
    uint64_t total_calls = 0;
    uint64_t total_xfer_first = 0;
    uint64_t total_stash_first = 0;
    uint64_t total_central_first = 0;
    uint64_t total_page_first = 0;

    uint64_t total_xfer_got = 0;
    uint64_t total_stash_got = 0;
    uint64_t total_central_got = 0;

    uint64_t total_page_alloc_attempt = 0;
    uint64_t total_page_alloc_fail = 0;

    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        total_calls += atomic_load_explicit(&g_s85_sc[sc].calls, memory_order_relaxed);
        total_xfer_first += atomic_load_explicit(&g_s85_sc[sc].xfer_first, memory_order_relaxed);
        total_stash_first += atomic_load_explicit(&g_s85_sc[sc].stash_first, memory_order_relaxed);
        total_central_first += atomic_load_explicit(&g_s85_sc[sc].central_first, memory_order_relaxed);
        total_page_first += atomic_load_explicit(&g_s85_sc[sc].page_first, memory_order_relaxed);

        total_xfer_got += atomic_load_explicit(&g_s85_sc[sc].xfer_got, memory_order_relaxed);
        total_stash_got += atomic_load_explicit(&g_s85_sc[sc].stash_got, memory_order_relaxed);
        total_central_got += atomic_load_explicit(&g_s85_sc[sc].central_got, memory_order_relaxed);

        total_page_alloc_attempt += atomic_load_explicit(&g_s85_sc[sc].page_alloc_attempt, memory_order_relaxed);
        total_page_alloc_fail += atomic_load_explicit(&g_s85_sc[sc].page_alloc_fail, memory_order_relaxed);
    }

    if (total_calls == 0) {
        return;
    }

    fprintf(stderr,
            "[HZ3_S85_SMALL_V2_SLOW] calls=%lu first(xfer=%lu stash=%lu central=%lu page=%lu) "
            "got(xfer=%lu stash=%lu central=%lu) page_alloc(attempt=%lu fail=%lu)\n",
            total_calls,
            total_xfer_first, total_stash_first, total_central_first, total_page_first,
            total_xfer_got, total_stash_got, total_central_got,
            total_page_alloc_attempt, total_page_alloc_fail);

    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        uint64_t calls = atomic_load_explicit(&g_s85_sc[sc].calls, memory_order_relaxed);
        if (calls == 0) {
            continue;
        }
        uint64_t xfer_first = atomic_load_explicit(&g_s85_sc[sc].xfer_first, memory_order_relaxed);
        uint64_t stash_first = atomic_load_explicit(&g_s85_sc[sc].stash_first, memory_order_relaxed);
        uint64_t central_first = atomic_load_explicit(&g_s85_sc[sc].central_first, memory_order_relaxed);
        uint64_t page_first = atomic_load_explicit(&g_s85_sc[sc].page_first, memory_order_relaxed);
        uint64_t page_alloc_attempt = atomic_load_explicit(&g_s85_sc[sc].page_alloc_attempt, memory_order_relaxed);
        uint64_t page_alloc_fail = atomic_load_explicit(&g_s85_sc[sc].page_alloc_fail, memory_order_relaxed);
        if (page_alloc_attempt == 0 && xfer_first == 0 && stash_first == 0 && central_first == 0) {
            continue;
        }
        fprintf(stderr,
                "  sc=%d size=%zu calls=%lu first(x=%lu s=%lu c=%lu p=%lu) page_alloc(at=%lu fail=%lu)\n",
                sc, hz3_small_sc_to_size(sc), calls,
                xfer_first, stash_first, central_first, page_first,
                page_alloc_attempt, page_alloc_fail);
    }
}

static void hz3_s85_register_atexit(void) {
    atexit(hz3_s85_dump);
}

void hz3_s85_small_v2_slow_record(int sc,
                                 int got_xfer,
                                 int got_stash,
                                 int got_central,
                                 int did_page_alloc,
                                 int page_alloc_ok) {
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }
    pthread_once(&g_s85_once, hz3_s85_register_atexit);

    atomic_fetch_add_explicit(&g_s85_sc[sc].calls, 1, memory_order_relaxed);

    int first_set = 0;
    if (got_xfer > 0) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].xfer_first, 1, memory_order_relaxed);
        first_set = 1;
    } else if (got_stash > 0) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].stash_first, 1, memory_order_relaxed);
        first_set = 1;
    } else if (got_central > 0) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].central_first, 1, memory_order_relaxed);
        first_set = 1;
    }

    if (!first_set && did_page_alloc) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].page_first, 1, memory_order_relaxed);
    }

    if (got_xfer > 0) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].xfer_got, (uint64_t)got_xfer, memory_order_relaxed);
    }
    if (got_stash > 0) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].stash_got, (uint64_t)got_stash, memory_order_relaxed);
    }
    if (got_central > 0) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].central_got, (uint64_t)got_central, memory_order_relaxed);
    }

    if (did_page_alloc) {
        atomic_fetch_add_explicit(&g_s85_sc[sc].page_alloc_attempt, 1, memory_order_relaxed);
        if (!page_alloc_ok) {
            atomic_fetch_add_explicit(&g_s85_sc[sc].page_alloc_fail, 1, memory_order_relaxed);
        }
    }
}

#endif

