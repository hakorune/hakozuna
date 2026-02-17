// Extracted from hz4_large_paths.inc (move-only): retry/acquire path.
#if HZ4_LARGE_CACHE_BOX && HZ4_LARGE_BATCH_ACQUIRE_BOX
static inline void* hz4_large_batch_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_BATCH_ACQUIRE_MAX_PAGES) {
        return NULL;
    }
    uint8_t* span = (uint8_t*)hz4_os_large_acquire(total * (size_t)HZ4_LARGE_BATCH_ACQUIRE_BLOCKS);
    if (!span) {
        return NULL;
    }
    for (size_t i = 1; i < (size_t)HZ4_LARGE_BATCH_ACQUIRE_BLOCKS; i++) {
        void* block = (void*)(span + (i * total));
        (void)hz4_large_cache_try_release(block, total);
    }
    return (void*)span;
}
#endif

static inline void* hz4_large_os_acquire_with_retry(size_t total) {
    void* base = NULL;
#if HZ4_LARGE_FAIL_RESCUE_BOX
    int rescue_attempted = 0;
    int rescue_failed = 0;
#endif
#if HZ4_LARGE_CACHE_BOX && HZ4_LARGE_BATCH_ACQUIRE_BOX && !HZ4_LARGE_BATCH_ACQUIRE_ON_FAILURE_ONLY
    base = hz4_large_batch_try_acquire(total);
    if (base) {
    #if HZ4_LARGE_FAIL_RESCUE_BOX
        g_hz4_large_fail_streak_tls = 0;
    #endif
        return base;
    }
#endif
    base = hz4_os_large_acquire(total);
#if HZ4_LARGE_CACHE_BOX
    if (!base) {
#if HZ4_LARGE_FAIL_RESCUE_BOX
        if (g_hz4_large_fail_streak_tls < UINT32_MAX) {
            g_hz4_large_fail_streak_tls++;
        }
        if (g_hz4_large_fail_streak_tls >= (uint32_t)HZ4_LARGE_FAIL_RESCUE_TRIGGER_STREAK) {
            int allow_rescue = 1;
        #if HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
            allow_rescue = hz4_large_rescue_gate_allow(g_hz4_large_fail_streak_tls);
        #endif
            if (allow_rescue) {
        #if (HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL > 1) || (HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF > 1)
            // Archived S218-B5 path (research-only).
            g_hz4_large_rescue_budget_tls++;
            if (g_hz4_large_rescue_budget_tls >= g_hz4_large_rescue_interval_tls) {
                rescue_attempted = 1;
                hz4_os_stats_large_rescue_attempt();
                base = hz4_large_cache_try_acquire_rescue(total);
                if (base) {
                #if HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
                    hz4_large_rescue_gate_record(1);
                #endif
                    // Success: reset streak, increase interval (backoff).
                    g_hz4_large_fail_streak_tls = 0;
                    g_hz4_large_rescue_budget_tls = 0;
                    hz4_os_stats_large_rescue_success();
                    if (g_hz4_large_rescue_interval_tls < HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF) {
                        g_hz4_large_rescue_interval_tls *= 2;
                    }
                    return base;
                }
            }
        #else
            // Default S218-B4 behavior: no budget suppression.
            rescue_attempted = 1;
            hz4_os_stats_large_rescue_attempt();
            base = hz4_large_cache_try_acquire_rescue(total);
            if (base) {
            #if HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
                hz4_large_rescue_gate_record(1);
            #endif
                g_hz4_large_fail_streak_tls = 0;
                hz4_os_stats_large_rescue_success();
                return base;
            }
        #endif
            }
        }
        if (rescue_attempted && !base) {
        #if HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
            hz4_large_rescue_gate_record(0);
        #endif
            rescue_failed = 1;
            hz4_os_stats_large_rescue_fail();
        }
#endif
    #if HZ4_LARGE_FAIL_RESCUE_BOX
        if (rescue_failed) {
            hz4_os_stats_large_rescue_fallback_flush();
        }
    #endif
#if HZ4_LARGE_OVERFLOW_TLS_BOX
        hz4_large_overflow_tls_flush_all();
#endif
        hz4_large_cache_flush_global_all();
#if HZ4_LARGE_CACHE_BOX && HZ4_LARGE_BATCH_ACQUIRE_BOX
        base = hz4_large_batch_try_acquire(total);
        if (base) {
        #if HZ4_LARGE_FAIL_RESCUE_BOX
            g_hz4_large_fail_streak_tls = 0;
            // Archived budget mode keeps its own interval state.
            if (rescue_failed) {
                hz4_os_stats_large_rescue_route_batch_hit();
            }
        #endif
            return base;
        }
#endif
        base = hz4_os_large_acquire(total);
    #if HZ4_LARGE_FAIL_RESCUE_BOX
        if (rescue_failed) {
            if (base) {
                hz4_os_stats_large_rescue_route_mmap_retry_ok();
            } else {
                hz4_os_stats_large_rescue_route_mmap_retry_fail();
            }
        }
    #endif
    }
#endif
