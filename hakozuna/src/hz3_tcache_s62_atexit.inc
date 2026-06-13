// ============================================================================
// S62-1G: SingleThreadRetireGate - Check if total live threads == 1
// ============================================================================

#if HZ3_S62_SINGLE_THREAD_GATE
static inline int hz3_s62_single_thread_ok(void) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        total += hz3_shard_live_count((uint8_t)i);
    }

    // Defense: total==0 should never happen (executing thread hasn't decremented yet).
    // Treat as unsafe (assume multi-thread) to avoid unexpected S62 execution.
    if (total == 0) {
#if HZ3_S62_SINGLE_THREAD_FAILFAST
        fprintf(stderr, "[HZ3_S62_SINGLE_THREAD_FAILFAST] total_live==0 during destructor (impossible)\n");
        abort();
#endif
        return 0;
    }

    return (total <= 1);
}
#endif  // HZ3_S62_SINGLE_THREAD_GATE

#if HZ3_S62_ATEXIT_GATE
// ============================================================================
// S62-1A: AtExitGate - One-shot atexit handler
// ============================================================================
static void hz3_s62_atexit_handler(void) {
    // One-shot guard: prevent double execution
    int expected = 0;
    if (!atomic_compare_exchange_strong(&g_s62_atexit_executed, &expected, 1)) {
        return;
    }

    // Uninitialized guard: skip if tcache never initialized
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S62_ATEXIT_LOG
    fprintf(stderr, "[HZ3_S62_ATEXIT] S62 cleanup at process exit\n");
#endif

    // Safety check: verify single-thread context
#if HZ3_S62_SINGLE_THREAD_GATE
    int single = hz3_s62_single_thread_ok();
#if HZ3_S62_SINGLE_THREAD_FAILFAST
    if (!single) {
        abort();  // Multi-thread detected: failfast
    }
#endif
    if (!single) {
        return;  // Multi-thread detected: skip S62
    }
#endif

    // Execute S62 sequence: retire → purge
#if HZ3_S62_RETIRE && !HZ3_S62_RETIRE_DISABLE
    hz3_s62_retire();
#endif
#if HZ3_S62_SUB4K_RETIRE && !HZ3_S62_SUB4K_RETIRE_DISABLE
    hz3_s62_sub4k_retire();
#endif
#if HZ3_S62_PURGE && !HZ3_S62_PURGE_DISABLE
    hz3_s62_purge();
#endif
}

static void hz3_s62_atexit_register(void) {
    atexit(hz3_s62_atexit_handler);
}
#endif  // HZ3_S62_ATEXIT_GATE
