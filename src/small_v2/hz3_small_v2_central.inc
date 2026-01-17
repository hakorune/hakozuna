// hz3_small_v2_central.inc - Central bin initialization and operations
// Part of hz3_small_v2.c (single TU split)

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
static pthread_once_t g_hz3_small_v2_central_once = PTHREAD_ONCE_INIT;
static Hz3CentralBin g_hz3_small_v2_central[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];

static void hz3_small_v2_central_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
            hz3_central_bin_init(&g_hz3_small_v2_central[shard][sc]);
        }
    }
}

static inline void hz3_small_v2_central_init(void) {
    pthread_once(&g_hz3_small_v2_central_once, hz3_small_v2_central_do_init);
}

void hz3_small_v2_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n) {
#if HZ3_S72_BOUNDARY_DEBUG
    hz3_small_v2_boundary_check_list("small_v2:central_push", shard, sc, head, tail, n);
#endif
    hz3_central_bin_push_list(&g_hz3_small_v2_central[shard][sc], head, tail, n);
}

// S111: Push a single object to central (n==1 fastpath)
static inline void hz3_small_v2_central_push_one(uint8_t shard, int sc, void* obj) {
    hz3_central_bin_push_one(&g_hz3_small_v2_central[shard][sc], obj);
}

#if HZ3_S145_CENTRAL_LOCAL_CACHE
// S145-A: Pop from TLS cache or grab bulk from central
// Returns the number of items popped (always 1 on hit, or 0..want on central fetch)
static inline int
hz3_s145_central_cache_pop(uint8_t shard, int sc, void** out, int want) {
    Hz3TCache* tc = &t_hz3_cache;

    // Shard collision check FIRST: if collision, flush cache and fallback
    // (cache is shard-unaware, collision makes cached items potentially stale)
    if (hz3_shard_live_count(shard) > 1) {
        // Flush any cached items back to central before fallback
        if (tc->s145_cache_count[sc] > 0) {
            void* head = tc->s145_cache_head[sc];
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_small_v2_central_push_list(shard, sc, head, tail, n);
            tc->s145_cache_head[sc] = NULL;
            tc->s145_cache_count[sc] = 0;
        }
        // Fallback to direct central pop (no caching)
        return hz3_central_bin_pop_batch(&g_hz3_small_v2_central[shard][sc], out, want);
    }

    // Fast path: TLS cache hit (collision already checked above)
    if (tc->s145_cache_count[sc] > 0) {
        void* item = tc->s145_cache_head[sc];
        tc->s145_cache_head[sc] = hz3_obj_get_next(item);
        tc->s145_cache_count[sc]--;
        out[0] = item;
        return 1;
    }

    // Slow path: grab bulk batch from central
    void* bulk[HZ3_S145_CACHE_BATCH];
    int got = hz3_central_bin_pop_batch(&g_hz3_small_v2_central[shard][sc], bulk, HZ3_S145_CACHE_BATCH);

    if (got == 0) {
        return 0;  // Central bin empty
    }

    // Return what caller wants, cache the remainder
    int ret_count = (want < got) ? want : got;
    for (int i = 0; i < ret_count; i++) {
        out[i] = bulk[i];
    }

    // Store remainder in TLS cache (even if got < batch)
    int cache_count = got - ret_count;
    if (cache_count > 0) {
        // Link the cached items using helper
        for (int i = ret_count; i < got - 1; i++) {
            hz3_obj_set_next(bulk[i], bulk[i + 1]);
        }
        hz3_obj_set_next(bulk[got - 1], NULL);

        tc->s145_cache_head[sc] = bulk[ret_count];
        tc->s145_cache_count[sc] = (uint16_t)cache_count;
    }

    return ret_count;
}
#endif  // HZ3_S145_CENTRAL_LOCAL_CACHE

static inline int hz3_small_v2_central_pop_batch(uint8_t shard, int sc, void** out, int want) {
    return hz3_central_bin_pop_batch(&g_hz3_small_v2_central[shard][sc], out, want);
}
#endif
