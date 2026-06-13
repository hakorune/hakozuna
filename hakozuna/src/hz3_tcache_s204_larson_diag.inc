// ============================================================================
// S204: Larson Diagnostic Instrumentation
// ============================================================================
#if HZ3_S204_LARSON_DIAG
// Flag to indicate thread is in destructor (for split push stats)
_Thread_local bool t_s204_in_dtor = false;

HZ3_DTOR_STATS_BEGIN(S204_ALLOC);
static _Thread_local uint64_t t_s204_alloc_seen_nonempty_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s204_alloc_seen_nonempty_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s204_alloc_seen_nonempty_shard_sc[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Atomic uint64_t g_s204_alloc_seen_nonempty_shard_sc[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static void hz3_s204_flush_tls(void) {
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s204_alloc_seen_nonempty_sc[sc],
                                  t_s204_alloc_seen_nonempty_sc[sc],
                                  memory_order_relaxed);
    }
    for (uint32_t shard = 0; shard < (uint32_t)HZ3_NUM_SHARDS; shard++) {
        for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
            atomic_fetch_add_explicit(&g_s204_alloc_seen_nonempty_shard_sc[shard][sc],
                                      t_s204_alloc_seen_nonempty_shard_sc[shard][sc],
                                      memory_order_relaxed);
        }
    }
}


HZ3_DTOR_ATEXIT_FLAG(g_s204_alloc);

static void hz3_s204_atexit_dump(void) {
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t seen =
            atomic_load_explicit(&g_s204_alloc_seen_nonempty_sc[sc], memory_order_relaxed);
        if (seen > 0) {
            fprintf(stderr, "[HZ3_S204_ALLOC] sc=%u seen_nonempty=%llu\n",
                    sc, (unsigned long long)seen);
        }
    }
    for (uint32_t shard = 0; shard < (uint32_t)HZ3_NUM_SHARDS; shard++) {
        uint64_t shard_total = 0;
        for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
            uint64_t seen =
                atomic_load_explicit(&g_s204_alloc_seen_nonempty_shard_sc[shard][sc],
                                     memory_order_relaxed);
            if (seen > 0) {
                fprintf(stderr,
                        "[HZ3_S204_ALLOC_SHARD_SC] shard=%u sc=%u seen_nonempty=%llu\n",
                        shard, sc, (unsigned long long)seen);
                shard_total += seen;
            }
        }
        if (shard_total > 0) {
            fprintf(stderr, "[HZ3_S204_ALLOC_SHARD] shard=%u seen_nonempty=%llu\n",
                    shard, (unsigned long long)shard_total);
        }
    }
}
#define S204_ALLOC_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s204_alloc, hz3_s204_atexit_dump)
#define S204_ALLOC_INC_SEEN(sc) (t_s204_alloc_seen_nonempty_sc[(sc)]++)
#define S204_ALLOC_INC_SHARD_SC(shard, sc) do {                     \
    uint32_t s204_shard = (uint32_t)(shard);                        \
    uint32_t s204_sc = (uint32_t)(sc);                              \
    if (s204_shard < (uint32_t)HZ3_NUM_SHARDS &&                    \
        s204_sc < (uint32_t)HZ3_NUM_SC) {                           \
        t_s204_alloc_seen_nonempty_shard_sc[s204_shard][s204_sc]++; \
    }                                                                \
} while (0)
#else
#define S204_ALLOC_REGISTER() ((void)0)
#define S204_ALLOC_INC_SEEN(sc) ((void)0)
#define S204_ALLOC_INC_SHARD_SC(shard, sc) ((void)0)
static inline void hz3_s204_flush_tls(void) {}
#endif
