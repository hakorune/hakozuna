#if HZ3_S197_MEDIUM_INBOX_CAS_STATS
static _Atomic uint64_t g_s197_push_calls = 0;
static _Atomic uint64_t g_s197_push_objs = 0;
static _Atomic uint64_t g_s197_push_retry_calls = 0;
static _Atomic uint64_t g_s197_push_retries = 0;
static _Atomic uint64_t g_s197_push_max_retry = 0;
static _Atomic uint64_t g_s197_push_retry_hist_0 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_1 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_2_3 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_4_7 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_8p = 0;
static _Atomic uint64_t g_s197_drain_calls = 0;
static _Atomic uint64_t g_s197_drain_nonempty = 0;
static _Atomic uint64_t g_s197_drain_objs = 0;
static _Atomic uint64_t g_s197_push_sc_calls[HZ3_NUM_SC];
static _Atomic uint64_t g_s197_push_sc_objs[HZ3_NUM_SC];
static _Atomic uint64_t g_s197_push_sc_retries[HZ3_NUM_SC];
static _Atomic uint64_t g_s197_drain_sc_objs[HZ3_NUM_SC];
static pthread_once_t g_s197_stats_once = PTHREAD_ONCE_INIT;

static void hz3_s197_stats_max_retry_update(uint64_t retries) {
    uint64_t cur = atomic_load_explicit(&g_s197_push_max_retry, memory_order_relaxed);
    while (retries > cur &&
           !atomic_compare_exchange_weak_explicit(&g_s197_push_max_retry, &cur, retries,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed)) {
    }
}

static void hz3_s197_stats_dump(void) {
    uint64_t push_calls =
        atomic_load_explicit(&g_s197_push_calls, memory_order_relaxed);
    uint64_t push_retries =
        atomic_load_explicit(&g_s197_push_retries, memory_order_relaxed);
    uint64_t retry_calls =
        atomic_load_explicit(&g_s197_push_retry_calls, memory_order_relaxed);
    uint64_t retry_max =
        atomic_load_explicit(&g_s197_push_max_retry, memory_order_relaxed);
    uint64_t drain_calls =
        atomic_load_explicit(&g_s197_drain_calls, memory_order_relaxed);
    uint64_t drain_nonempty =
        atomic_load_explicit(&g_s197_drain_nonempty, memory_order_relaxed);
    uint64_t drain_objs =
        atomic_load_explicit(&g_s197_drain_objs, memory_order_relaxed);
    double retry_per_call = (push_calls > 0)
                                ? ((double)push_retries / (double)push_calls)
                                : 0.0;
    double retry_call_pct = (push_calls > 0)
                                ? (100.0 * (double)retry_calls / (double)push_calls)
                                : 0.0;
    fprintf(stderr,
            "[HZ3_S197_INBOX_CAS] push_calls=%llu push_objs=%llu retry_calls=%llu "
            "push_retries=%llu retry_per_call=%.4f retry_call_pct=%.2f max_retry=%llu "
            "hist0=%llu hist1=%llu hist2_3=%llu hist4_7=%llu hist8p=%llu "
            "drain_calls=%llu drain_nonempty=%llu drain_objs=%llu\n",
            (unsigned long long)push_calls,
            (unsigned long long)atomic_load_explicit(&g_s197_push_objs, memory_order_relaxed),
            (unsigned long long)retry_calls,
            (unsigned long long)push_retries,
            retry_per_call,
            retry_call_pct,
            (unsigned long long)retry_max,
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_0, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_1, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_2_3, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_4_7, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_8p, memory_order_relaxed),
            (unsigned long long)drain_calls,
            (unsigned long long)drain_nonempty,
            (unsigned long long)drain_objs);

    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t sc_calls =
            atomic_load_explicit(&g_s197_push_sc_calls[sc], memory_order_relaxed);
        uint64_t sc_objs =
            atomic_load_explicit(&g_s197_push_sc_objs[sc], memory_order_relaxed);
        uint64_t sc_retries =
            atomic_load_explicit(&g_s197_push_sc_retries[sc], memory_order_relaxed);
        uint64_t sc_drain_objs =
            atomic_load_explicit(&g_s197_drain_sc_objs[sc], memory_order_relaxed);
        if (sc_calls == 0 && sc_objs == 0 && sc_retries == 0 && sc_drain_objs == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S197_INBOX_CAS_SC] sc=%u push_calls=%llu push_objs=%llu "
                "push_retries=%llu drain_objs=%llu\n",
                sc,
                (unsigned long long)sc_calls,
                (unsigned long long)sc_objs,
                (unsigned long long)sc_retries,
                (unsigned long long)sc_drain_objs);
    }
}

static void hz3_s197_stats_register(void) {
    atexit(hz3_s197_stats_dump);
}

static inline void hz3_s197_stats_register_once(void) {
    pthread_once(&g_s197_stats_once, hz3_s197_stats_register);
}

static inline void hz3_s197_stats_on_push(int sc, uint32_t n, uint64_t retries) {
    atomic_fetch_add_explicit(&g_s197_push_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s197_push_objs, n, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s197_push_retries, retries, memory_order_relaxed);
    if (retries > 0) {
        atomic_fetch_add_explicit(&g_s197_push_retry_calls, 1, memory_order_relaxed);
    }
    if (retries == 0) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_0, 1, memory_order_relaxed);
    } else if (retries == 1) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_1, 1, memory_order_relaxed);
    } else if (retries <= 3) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_2_3, 1, memory_order_relaxed);
    } else if (retries <= 7) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_4_7, 1, memory_order_relaxed);
    } else {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_8p, 1, memory_order_relaxed);
    }
    if (sc >= 0 && sc < HZ3_NUM_SC) {
        atomic_fetch_add_explicit(&g_s197_push_sc_calls[sc], 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s197_push_sc_objs[sc], n, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s197_push_sc_retries[sc], retries, memory_order_relaxed);
    }
    hz3_s197_stats_max_retry_update(retries);
}

static inline void hz3_s197_stats_on_drain(int sc, uint64_t drained_objs, int nonempty) {
    atomic_fetch_add_explicit(&g_s197_drain_calls, 1, memory_order_relaxed);
    if (nonempty) {
        atomic_fetch_add_explicit(&g_s197_drain_nonempty, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s197_drain_objs, drained_objs, memory_order_relaxed);
        if (sc >= 0 && sc < HZ3_NUM_SC) {
            atomic_fetch_add_explicit(&g_s197_drain_sc_objs[sc], drained_objs, memory_order_relaxed);
        }
    }
}
#else
static inline void hz3_s197_stats_register_once(void) {}
static inline void hz3_s197_stats_on_push(int sc, uint32_t n, uint64_t retries) {
    (void)sc;
    (void)n;
    (void)retries;
}
static inline void hz3_s197_stats_on_drain(int sc, uint64_t drained_objs, int nonempty) {
    (void)sc;
    (void)drained_objs;
    (void)nonempty;
}
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS
static _Atomic uint64_t g_s195_push_calls = 0;
static _Atomic uint64_t g_s195_push_objs = 0;
static _Atomic uint64_t g_s195_push_retries = 0;
static _Atomic uint64_t g_s195_push_max_retry = 0;
static _Atomic uint64_t g_s195_drain_calls = 0;
static _Atomic uint64_t g_s195_drain_nonempty = 0;
static _Atomic uint64_t g_s195_drain_objs = 0;
static _Atomic uint64_t g_s195_lane_pushes[HZ3_S195_MEDIUM_INBOX_SHARDS];
static pthread_once_t g_s195_stats_once = PTHREAD_ONCE_INIT;

static void hz3_s195_stats_max_retry_update(uint64_t retries) {
    uint64_t cur = atomic_load_explicit(&g_s195_push_max_retry, memory_order_relaxed);
    while (retries > cur &&
           !atomic_compare_exchange_weak_explicit(&g_s195_push_max_retry, &cur, retries,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed)) {
    }
}

static void hz3_s195_stats_dump(void) {
    fprintf(stderr,
            "[hz3] S195 inbox stats: push_calls=%llu push_objs=%llu push_retries=%llu "
            "max_retry=%llu drain_calls=%llu drain_nonempty=%llu drain_objs=%llu shards=%u\n",
            (unsigned long long)atomic_load_explicit(&g_s195_push_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_push_objs, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_push_retries, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_push_max_retry, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_drain_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_drain_nonempty, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_drain_objs, memory_order_relaxed),
            (unsigned)HZ3_S195_MEDIUM_INBOX_SHARDS);
    fprintf(stderr, "[hz3] S195 inbox lane_pushes:");
    for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
        fprintf(stderr, " [%u]=%llu", (unsigned)lane,
                (unsigned long long)atomic_load_explicit(&g_s195_lane_pushes[lane], memory_order_relaxed));
    }
    fprintf(stderr, "\n");
}

static void hz3_s195_stats_register(void) {
    atexit(hz3_s195_stats_dump);
}
#endif

static inline uint32_t hz3_inbox_lane_select(uint8_t owner, int sc, void* head, void* tail) {
#if HZ3_S195_MEDIUM_INBOX_SHARDS == 1
    (void)owner;
    (void)sc;
    (void)head;
    (void)tail;
    return 0;
#else
    uintptr_t x = (uintptr_t)head;
    x ^= ((uintptr_t)tail >> 4);
    x ^= ((uintptr_t)owner << 16);
    x ^= ((uintptr_t)(uint32_t)sc << 8);
    x ^= (x >> 33);
    x *= 0xff51afd7ed558ccdULL;
    x ^= (x >> 33);
    return (uint32_t)x & (HZ3_S195_MEDIUM_INBOX_SHARDS - 1u);
#endif
}

static void hz3_inbox_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
            for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
                atomic_store_explicit(&inbox->head[lane], NULL, memory_order_relaxed);
                atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);
            }
            atomic_store_explicit(&inbox->seq, 0, memory_order_relaxed);
            atomic_store_explicit(&inbox->drain_seq, 0, memory_order_relaxed);
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
            Hz3Inbox* aligned = &g_hz3_inbox_medium_aligned[shard][sc];
            for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
                atomic_store_explicit(&aligned->head[lane], NULL, memory_order_relaxed);
                atomic_store_explicit(&aligned->count[lane], 0, memory_order_relaxed);
            }
            atomic_store_explicit(&aligned->seq, 0, memory_order_relaxed);
            atomic_store_explicit(&aligned->drain_seq, 0, memory_order_relaxed);
#endif
        }
        Hz3InboxMixed* mixed = &g_hz3_inbox_medium_mixed[shard];
        for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
            atomic_store_explicit(&mixed->head[lane], NULL, memory_order_relaxed);
            atomic_store_explicit(&mixed->count[lane], 0, memory_order_relaxed);
        }
        atomic_store_explicit(&mixed->seq, 0, memory_order_relaxed);
        atomic_store_explicit(&mixed->drain_seq, 0, memory_order_relaxed);
    }
}

void hz3_inbox_init(void) {
    pthread_once(&g_hz3_inbox_once, hz3_inbox_do_init);
#if HZ3_S65_STATS
    pthread_once(&g_s65_inbox_stats_once, hz3_s65_inbox_register_once);
#endif
#if HZ3_S195_MEDIUM_INBOX_STATS
    pthread_once(&g_s195_stats_once, hz3_s195_stats_register);
#endif
#if HZ3_S203_COUNTERS
    hz3_s203_inbox_stats_register_once();
#endif
}
