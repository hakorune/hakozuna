#if HZ3_S189_MEDIUM_TRANSFERCACHE
typedef struct {
    hz3_lock_t lock;
    void* head;
    uint32_t count;
} Hz3CentralXferBin;

static Hz3CentralXferBin g_hz3_central_xfer[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Atomic uint64_t g_s189_xfer_push_calls = 0;
static _Atomic uint64_t g_s189_xfer_push_objs = 0;
static _Atomic uint64_t g_s189_xfer_push_drop = 0;
static _Atomic uint64_t g_s189_xfer_pop_calls = 0;
static _Atomic uint64_t g_s189_xfer_pop_hits = 0;
static _Atomic uint64_t g_s189_xfer_pop_objs = 0;
static hz3_once_t g_s189_xfer_atexit_once = HZ3_ONCE_INIT;

static inline int hz3_s189_sc_ok(int sc) {
    return (sc >= HZ3_S189_SC_MIN && sc <= HZ3_S189_SC_MAX);
}

static void hz3_s189_xfer_dump(void) {
    fprintf(stderr,
            "[HZ3_S189_XFER] push_calls=%lu push_objs=%lu push_drop=%lu "
            "pop_calls=%lu pop_hits=%lu pop_objs=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_drop, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_objs, memory_order_relaxed));
}

static void hz3_s189_xfer_register_atexit(void) {
    atexit(hz3_s189_xfer_dump);
}
#endif

// Day 5: hz3_once for thread-safe initialization
static hz3_once_t g_hz3_central_once = HZ3_ONCE_INIT;

static void hz3_central_do_init(void) {
#if HZ3_S65_CENTRAL_COLD_ENABLE && HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    hz3_central_cold_external_pool_init();
#endif
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3CentralBin* bin = &g_hz3_central[shard][sc];
            hz3_lock_init(&bin->lock);
            bin->head = NULL;
            bin->count = 0;
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
            {
                Hz3CentralBin* aligned = &g_hz3_central_aligned[shard][sc];
                hz3_lock_init(&aligned->lock);
                aligned->head = NULL;
                aligned->count = 0;
            }
#endif
#if HZ3_S222_CENTRAL_ATOMIC_FAST
            if (hz3_s222_sc_ok(sc)) {
                atomic_store_explicit(&g_hz3_central_fast[shard][sc].head, NULL, memory_order_relaxed);
            }
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
            if (hz3_s189_sc_ok(sc)) {
                Hz3CentralXferBin* xfer = &g_hz3_central_xfer[shard][sc];
                hz3_lock_init(&xfer->lock);
                xfer->head = NULL;
                xfer->count = 0;
            }
#endif
#if HZ3_S65_CENTRAL_COLD_ENABLE
            {
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
                Hz3CentralColdExternalBin* cold = &g_hz3_central_cold_ext[shard][sc];
                hz3_lock_init(&cold->lock);
                cold->ready_head = NULL;
                cold->ready_tail = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
                cold->decommitted_head = NULL;
                cold->ready_count = 0;
                cold->decommitted_count = 0;
                cold->inflight_decommitting = 0;
                cold->inflight_recommitting = 0;
#endif
                cold->count = 0;
#else
                Hz3CentralBin* cold = &g_hz3_central_cold[shard][sc];
                hz3_lock_init(&cold->lock);
                cold->head = NULL;
                cold->count = 0;
#endif
            }
#endif
        }
    }
}

void hz3_central_init(void) {
    hz3_once(&g_hz3_central_once, hz3_central_do_init);
#if HZ3_S65_STATS
    hz3_once(&g_s65_central_stats_once, hz3_s65_central_retention_register_once);
#endif
#if HZ3_S222_CENTRAL_ATOMIC_FAST && HZ3_S222_STATS
    hz3_once(&g_s222_stats_once, hz3_s222_register_atexit);
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
    hz3_once(&g_s189_xfer_atexit_once, hz3_s189_xfer_register_atexit);
#endif
}
