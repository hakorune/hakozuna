#if HZ3_S65_STATS || HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL
#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_dtor_inbox_calls;
static _Atomic(uint64_t) g_s65_dtor_inbox_nonempty_calls;
static _Atomic(uint64_t) g_s65_dtor_inbox_before_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_dtor_inbox_drained_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_dtor_inbox_after_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_dtor_inbox_max_before_objs;
static _Atomic(uint64_t) g_s65_dtor_inbox_max_after_objs;
static pthread_once_t g_s65_dtor_inbox_once = PTHREAD_ONCE_INIT;

static void hz3_s65_dtor_inbox_update_max(_Atomic(uint64_t)* max_value, uint64_t value) {
    uint64_t old = atomic_load_explicit(max_value, memory_order_relaxed);
    while (old < value &&
           !atomic_compare_exchange_weak_explicit(max_value, &old, value,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
    }
}

static uint64_t hz3_s65_dtor_inbox_sc_bytes(int sc, uint64_t objs) {
    return objs * (uint64_t)hz3_sc_to_size(sc);
}

static void hz3_s65_dtor_inbox_dump(void) {
    uint64_t calls = atomic_load_explicit(&g_s65_dtor_inbox_calls, memory_order_relaxed);
    uint64_t nonempty_calls =
        atomic_load_explicit(&g_s65_dtor_inbox_nonempty_calls, memory_order_relaxed);
    uint64_t total_before = 0;
    uint64_t total_drained = 0;
    uint64_t total_after = 0;
    uint64_t total_before_bytes = 0;
    uint64_t total_drained_bytes = 0;
    uint64_t total_after_bytes = 0;

    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t before =
            atomic_load_explicit(&g_s65_dtor_inbox_before_objs[sc], memory_order_relaxed);
        uint64_t drained =
            atomic_load_explicit(&g_s65_dtor_inbox_drained_objs[sc], memory_order_relaxed);
        uint64_t after =
            atomic_load_explicit(&g_s65_dtor_inbox_after_objs[sc], memory_order_relaxed);
        total_before += before;
        total_drained += drained;
        total_after += after;
        total_before_bytes += hz3_s65_dtor_inbox_sc_bytes(sc, before);
        total_drained_bytes += hz3_s65_dtor_inbox_sc_bytes(sc, drained);
        total_after_bytes += hz3_s65_dtor_inbox_sc_bytes(sc, after);
    }

    if (calls == 0 && total_before == 0 && total_drained == 0 && total_after == 0) {
        return;
    }

    fprintf(stderr,
            "[HZ3_S65_DTOR_INBOX] calls=%" PRIu64 " nonempty_calls=%" PRIu64
            " before=%" PRIu64 " before_bytes=%" PRIu64
            " drained=%" PRIu64 " drained_bytes=%" PRIu64
            " after=%" PRIu64 " after_bytes=%" PRIu64
            " max_before=%" PRIu64 " max_after=%" PRIu64
            " drain_to_central=%d reclaim_after_drain=%d\n",
            calls,
            nonempty_calls,
            total_before,
            total_before_bytes,
            total_drained,
            total_drained_bytes,
            total_after,
            total_after_bytes,
            atomic_load_explicit(&g_s65_dtor_inbox_max_before_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s65_dtor_inbox_max_after_objs, memory_order_relaxed),
            HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL,
            HZ3_S65_DTOR_RECLAIM_AFTER_INBOX_DRAIN);

    fprintf(stderr, "  by_sc:");
    int printed = 0;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t before =
            atomic_load_explicit(&g_s65_dtor_inbox_before_objs[sc], memory_order_relaxed);
        uint64_t drained =
            atomic_load_explicit(&g_s65_dtor_inbox_drained_objs[sc], memory_order_relaxed);
        uint64_t after =
            atomic_load_explicit(&g_s65_dtor_inbox_after_objs[sc], memory_order_relaxed);
        if (before > 0 || drained > 0 || after > 0) {
            fprintf(stderr,
                    " sc%d=%" PRIu64 "b/%" PRIu64 "d/%" PRIu64 "a",
                    sc,
                    before,
                    drained,
                    after);
            printed = 1;
        }
    }
    if (!printed) {
        fprintf(stderr, " none");
    }
    fprintf(stderr, "\n");
}

static void hz3_s65_dtor_inbox_register_once(void) {
    atexit(hz3_s65_dtor_inbox_dump);
}
#endif  // HZ3_S65_STATS

static void hz3_s65_dtor_inbox_probe(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) {
        return;
    }

#if HZ3_S65_STATS
    pthread_once(&g_s65_dtor_inbox_once, hz3_s65_dtor_inbox_register_once);
    atomic_fetch_add_explicit(&g_s65_dtor_inbox_calls, 1, memory_order_relaxed);
#endif

    uint64_t call_before = 0;
    uint64_t call_after = 0;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint32_t before = hz3_inbox_count_approx(shard, sc);
        uint32_t drained = 0;
#if HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL
        if (before > 0) {
            drained = hz3_inbox_drain_to_central(shard, sc);
        }
#endif
        uint32_t after = hz3_inbox_count_approx(shard, sc);

        call_before += before;
        call_after += after;
#if HZ3_S65_STATS
        if (before > 0) {
            atomic_fetch_add_explicit(&g_s65_dtor_inbox_before_objs[sc],
                                      (uint64_t)before,
                                      memory_order_relaxed);
        }
        if (drained > 0) {
            atomic_fetch_add_explicit(&g_s65_dtor_inbox_drained_objs[sc],
                                      (uint64_t)drained,
                                      memory_order_relaxed);
        }
        if (after > 0) {
            atomic_fetch_add_explicit(&g_s65_dtor_inbox_after_objs[sc],
                                      (uint64_t)after,
                                      memory_order_relaxed);
        }
#else
        (void)drained;
#endif
    }

#if HZ3_S65_STATS
    if (call_before > 0) {
        atomic_fetch_add_explicit(&g_s65_dtor_inbox_nonempty_calls, 1, memory_order_relaxed);
    }
    hz3_s65_dtor_inbox_update_max(&g_s65_dtor_inbox_max_before_objs, call_before);
    hz3_s65_dtor_inbox_update_max(&g_s65_dtor_inbox_max_after_objs, call_after);
#endif
}
#endif  // HZ3_S65_STATS || HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL
