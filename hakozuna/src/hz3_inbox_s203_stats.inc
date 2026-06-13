static inline int hz3_s231_fast_sc_enabled(int sc) {
#if HZ3_S231_INBOX_FAST_MPSC
    return (sc >= HZ3_S231_SC_MIN && sc <= HZ3_S231_SC_MAX);
#else
    (void)sc;
    return 0;
#endif
}

// Day 5: pthread_once for thread-safe initialization
static pthread_once_t g_hz3_inbox_once = PTHREAD_ONCE_INIT;

#if HZ3_S203_COUNTERS
// S203: Inbox Counters (Medium) - TLS Version
static _Thread_local uint64_t t_s203_inbox_push_calls = 0;
static _Thread_local uint64_t t_s203_inbox_push_objs = 0;
static _Thread_local uint64_t t_s203_inbox_push_retries = 0;
static _Thread_local uint64_t t_s203_inbox_drain_calls = 0;
static _Thread_local uint64_t t_s203_inbox_drain_nonempty = 0;
static _Thread_local uint64_t t_s203_inbox_drain_objs = 0;
static _Thread_local uint64_t t_s203_inbox_push_sc_objs[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_inbox_drain_sc_objs[HZ3_NUM_SC];

static _Atomic uint64_t g_s203_inbox_push_calls = 0;
static _Atomic uint64_t g_s203_inbox_push_objs = 0;
static _Atomic uint64_t g_s203_inbox_push_retries = 0;
static _Atomic uint64_t g_s203_inbox_drain_calls = 0;
static _Atomic uint64_t g_s203_inbox_drain_nonempty = 0;
static _Atomic uint64_t g_s203_inbox_drain_objs = 0;
static _Atomic uint64_t g_s203_inbox_push_sc_objs[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_inbox_drain_sc_objs[HZ3_NUM_SC];

#if HZ3_S204_LARSON_DIAG
extern _Thread_local bool t_s204_in_dtor;
static _Thread_local uint64_t t_s204_inbox_push_dtor_sc_objs[HZ3_NUM_SC];
static _Atomic uint64_t g_s204_inbox_push_dtor_sc_objs[HZ3_NUM_SC];
#endif

static pthread_once_t g_s203_inbox_stats_once = PTHREAD_ONCE_INIT;

// Flush TLS to Global (called from thread dtor)
void hz3_s203_inbox_flush_tls(void) {
    atomic_fetch_add_explicit(&g_s203_inbox_push_calls, t_s203_inbox_push_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_push_objs, t_s203_inbox_push_objs, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_push_retries, t_s203_inbox_push_retries, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_drain_calls, t_s203_inbox_drain_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_drain_nonempty, t_s203_inbox_drain_nonempty, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_drain_objs, t_s203_inbox_drain_objs, memory_order_relaxed);
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s203_inbox_push_sc_objs[sc],
                                  t_s203_inbox_push_sc_objs[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_inbox_drain_sc_objs[sc],
                                  t_s203_inbox_drain_sc_objs[sc],
                                  memory_order_relaxed);
#if HZ3_S204_LARSON_DIAG
        atomic_fetch_add_explicit(&g_s204_inbox_push_dtor_sc_objs[sc],
                                  t_s204_inbox_push_dtor_sc_objs[sc],
                                  memory_order_relaxed);
#endif
    }
}

static void hz3_s203_inbox_stats_dump(void) {
    fprintf(stderr,
            "[HZ3_S203] inbox_push_calls=%llu inbox_push_objs=%llu inbox_push_retries=%llu "
            "inbox_drain_calls=%llu inbox_drain_nonempty=%llu inbox_drain_objs=%llu\n",
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_push_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_push_objs, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_push_retries, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_drain_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_drain_nonempty, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_drain_objs, memory_order_relaxed));
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t push_objs =
            atomic_load_explicit(&g_s203_inbox_push_sc_objs[sc], memory_order_relaxed);
        uint64_t drain_objs =
            atomic_load_explicit(&g_s203_inbox_drain_sc_objs[sc], memory_order_relaxed);
        if (push_objs == 0 && drain_objs == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S203_INBOX_SC] sc=%u push_objs=%llu drain_objs=%llu\n",
                sc,
                (unsigned long long)push_objs,
                (unsigned long long)drain_objs);
#if HZ3_S204_LARSON_DIAG
        uint64_t dtor_push =
            atomic_load_explicit(&g_s204_inbox_push_dtor_sc_objs[sc], memory_order_relaxed);
        if (dtor_push > 0) {
             fprintf(stderr,
                "[HZ3_S204_INBOX_SC] sc=%u push_dtor_objs=%llu\n",
                sc, (unsigned long long)dtor_push);
        }
#endif
    }
}

static void hz3_s203_inbox_stats_register(void) {
    atexit(hz3_s203_inbox_stats_dump);
}

static void hz3_s203_inbox_stats_register_once(void) {
    pthread_once(&g_s203_inbox_stats_once, hz3_s203_inbox_stats_register);
}

bool hz3_inbox_peek_check(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return false;
    if (sc < 0 || sc >= HZ3_NUM_SC) return false;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t lane_max = (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    if (hz3_s231_fast_sc_enabled(sc)) {
        lane_max = 1;
    }
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        if (atomic_load_explicit(&inbox->head[lane], memory_order_relaxed) != NULL) {
            return true;
        }
    }
    return false;
}

#else
static inline void hz3_s203_inbox_stats_register_once(void) {}
bool hz3_inbox_peek_check(uint8_t shard, int sc) { (void)shard; (void)sc; return false; }
void hz3_s203_inbox_flush_tls(void) {}
#endif

uint32_t hz3_inbox_count_approx(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    if (hz3_s231_fast_sc_enabled(sc)) {
        return atomic_load_explicit(&inbox->count[0], memory_order_relaxed);
    }
    uint32_t total = 0;
    for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
        total += atomic_load_explicit(&inbox->count[lane], memory_order_relaxed);
    }
    return total;
}

uint32_t hz3_inbox_drain_seq_load(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    return atomic_load_explicit(&g_hz3_inbox[shard][sc].drain_seq, memory_order_acquire);
}
