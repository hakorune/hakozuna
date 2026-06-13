// S45: Slot search helper (extracted for reclaim retry)
static void* hz3_arena_try_alloc_slot(uint32_t* idx_out) {
    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        return NULL;
    }

    hz3_lock_acquire(&g_hz3_arena_lock);
    uint32_t start = g_hz3_arena.alloc_cursor;
    for (uint32_t pass = 0; pass < 2; pass++) {
        uint32_t begin = (pass == 0) ? start : 0;
        uint32_t end = (pass == 0) ? g_hz3_arena.slots : start;
        for (uint32_t i = begin; i < end; i++) {
            if (atomic_load_explicit(&g_hz3_arena.used[i], memory_order_relaxed)) {
                continue;
            }
            void* addr = (char*)base_ptr + (size_t)i * HZ3_SEG_SIZE;
            void* mapped = mmap(addr, HZ3_SEG_SIZE, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                                -1, 0);
            if (mapped == MAP_FAILED) {
#if HZ3_ARENA_ALLOC_FAIL_SHOT
                {
                    static _Atomic uint32_t g_arena_alloc_fail_shot = 0;
                    uint32_t shot = atomic_fetch_add_explicit(&g_arena_alloc_fail_shot, 1, memory_order_relaxed);
                    if (shot < (uint32_t)HZ3_ARENA_ALLOC_FAIL_MAX) {
#if defined(_WIN32)
                        DWORD err = GetLastError();
                        FILE* log = fopen("hz3_arena_fail.log", "a");
                        if (log) {
                            fprintf(log, "[HZ3_ARENA_ALLOC_FAIL] addr=%p idx=%u err=%lu\n",
                                    addr, (unsigned)i, (unsigned long)err);
                            fflush(log);
                            fclose(log);
                        }
                        fprintf(stderr, "[HZ3_ARENA_ALLOC_FAIL] addr=%p idx=%u err=%lu\n",
                                addr, (unsigned)i, (unsigned long)err);
#else
                        fprintf(stderr, "[HZ3_ARENA_ALLOC_FAIL] addr=%p idx=%u errno=%d\n",
                                addr, (unsigned)i, errno);
#endif
                    }
                }
#endif
                continue;
            }
            atomic_store_explicit(&g_hz3_arena.used[i], 1, memory_order_release);
#if HZ3_S47_SEGMENT_QUARANTINE
            atomic_fetch_add_explicit(&g_hz3_arena_used_slots, 1, memory_order_relaxed);
#endif
            g_hz3_arena.alloc_cursor = (i + 1 < g_hz3_arena.slots) ? (i + 1) : 0;
            hz3_lock_release(&g_hz3_arena_lock);
            if (idx_out) {
                *idx_out = i;
            }
            return mapped;
        }
    }
    hz3_lock_release(&g_hz3_arena_lock);
    return NULL;
}

#if HZ3_S47_ARENA_GATE
// S47-2: ArenaGateBox - pressure compaction (called by leader or follower rescue)
// TLS safety: checks t_hz3_cache.initialized before using shard
static void run_pressure_compaction(void) {
#if HZ3_S47_SEGMENT_QUARANTINE
    // TLS safe: only call shard-dependent functions if initialized
    if (t_hz3_cache.initialized) {
        hz3_s47_policy_on_alloc_full();
        hz3_s47_compact_hard(t_hz3_cache.my_shard);
    }
#endif

#if HZ3_ARENA_PRESSURE_BOX
    // Broadcast pressure to all threads
    atomic_fetch_add_explicit(&g_hz3_arena_pressure_epoch, 1, memory_order_release);

    // Yield + focused reclaim (no try_alloc here - caller handles retry)
    for (int retry = 0; retry < 32; retry++) {
        sched_yield();
#if HZ3_MEM_BUDGET_ENABLE && HZ3_S45_FOCUS_RECLAIM
        for (int pass = 0; pass < 8; pass++) {
            Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();
            if (prog.sampled == 0) break;
        }
#endif
    }
#endif

#if HZ3_MEM_BUDGET_ENABLE
    // Emergency flush
    hz3_mem_budget_emergency_flush();

#if HZ3_S45_EMERGENCY_FLUSH_REMOTE && HZ3_PTAG_DSTBIN_ENABLE
    // TLS safe: remote flush requires initialized TLS
    if (t_hz3_cache.initialized) {
        hz3_dstbin_flush_remote_all();
    }
#endif

#if HZ3_S45_FOCUS_RECLAIM
    // Multi-pass focused reclaim
    for (int pass = 0; pass < HZ3_S45_FOCUS_PASSES; pass++) {
        Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();
        if (prog.sampled == 0 || prog.freed_pages == 0) {
            break;
        }
    }
#endif

    // Medium runs reclaim
    (void)hz3_mem_budget_reclaim_medium_runs();

    // Segment reclaim
    (void)hz3_mem_budget_reclaim_segments(HZ3_SEG_SIZE);
#endif
}
#endif  // HZ3_S47_ARENA_GATE
