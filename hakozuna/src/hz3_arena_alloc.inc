void* hz3_arena_alloc(uint32_t* idx_out) {
    hz3_arena_init_slow();

    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        hz3_oom_note("arena_alloc_base_null", g_hz3_arena.slots, g_hz3_arena.size);
        return NULL;
    }

#if HZ3_S47_SEGMENT_QUARANTINE
    // S47: Headroom check - trigger soft compact before exhaustion
    // Only run when TLS is initialized (original behavior)
    if (t_hz3_cache.initialized) {
        uint32_t used = hz3_arena_used_slots();
        uint32_t total = hz3_arena_total_slots();
        uint32_t headroom = hz3_s47_policy_get_headroom_slots();
        if (used >= total - headroom) {
            hz3_s47_compact_soft(t_hz3_cache.my_shard);
        }
    }
#endif

    // First attempt
    void* seg = hz3_arena_try_alloc_slot(idx_out);
    if (seg) {
        return seg;
    }

#if HZ3_S47_ARENA_GATE
    // S47-2: ArenaGateBox - leader election with retry loop
    for (int gate_attempt = 0; gate_attempt < 4; gate_attempt++) {
        uint32_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(
                &g_hz3_arena_gate, &expected, 1,
                memory_order_acquire, memory_order_relaxed)) {
            // ===== Leader =====
            atomic_fetch_add_explicit(&g_hz3_gate_leader_count, 1, memory_order_relaxed);
            uint32_t gen_before = hz3_arena_free_gen();
            run_pressure_compaction();
#if HZ3_OOM_SHOT && HZ3_S47_SEGMENT_QUARANTINE
            // S47-4: One-shot snapshot after pressure compaction
            {
                static _Atomic(int) snapshot_done = 0;
                if (atomic_exchange_explicit(&snapshot_done, 1, memory_order_relaxed) == 0) {
                    fprintf(stderr, "[SNAPSHOT] max_potential=%u pinned=%u avoid_hits=%u avoid_used=%u\n",
                            hz3_s47_get_max_potential(),
                            hz3_s47_get_pinned_candidates(),
                            hz3_s47_get_avoid_hits(),
                            hz3_s47_get_avoid_used());
                }
            }
#endif
            if (hz3_arena_free_gen() != gen_before) {
                atomic_fetch_add_explicit(&g_hz3_gate_seg_freed, 1, memory_order_relaxed);
            }
            // Gate release (must always execute)
            atomic_store_explicit(&g_hz3_arena_gate, 0, memory_order_release);
        } else {
            // ===== Follower =====
            // Light compaction: flush local bins to feed central
#if HZ3_MEM_BUDGET_ENABLE
            hz3_mem_budget_emergency_flush();
#if HZ3_S45_EMERGENCY_FLUSH_REMOTE && HZ3_PTAG_DSTBIN_ENABLE
            if (t_hz3_cache.initialized) {
                hz3_dstbin_flush_remote_all();
            }
#endif
#endif
            // Wait for free_gen change OR gate release
            uint32_t gen0 = hz3_arena_free_gen();
            int wait_ok = 0;
            for (int i = 0; i < HZ3_S47_GATE_WAIT_LOOPS; i++) {
                if (hz3_arena_free_gen() != gen0) {
                    wait_ok = 1;
                    break;  // Segment freed
                }
                if (atomic_load_explicit(&g_hz3_arena_gate, memory_order_relaxed) == 0) {
                    break;  // Leader finished, retry election
                }
                sched_yield();
            }
            if (wait_ok) {
                atomic_fetch_add_explicit(&g_hz3_gate_follower_wait_ok, 1, memory_order_relaxed);
            } else {
                atomic_fetch_add_explicit(&g_hz3_gate_follower_timeout, 1, memory_order_relaxed);
            }
        }
        // Try alloc after each gate attempt
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) {
            return seg;
        }
    }
#else  // !HZ3_S47_ARENA_GATE
#if HZ3_S47_SEGMENT_QUARANTINE
    // S47-PolicyBox: Aggressive drain before PRESSURE_BOX
    // Only run when TLS is initialized
    if (t_hz3_cache.initialized) {
        hz3_s47_policy_on_alloc_full();
        hz3_s47_compact_hard(t_hz3_cache.my_shard);
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) {
            hz3_s47_policy_on_wait_success();
            return seg;
        }
    }
#endif

#if HZ3_ARENA_PRESSURE_BOX
    // Pressure Box: broadcast arena exhaustion to all threads via epoch.
    // Other threads respond in their fast path by dropping to slow path and flushing.
    atomic_fetch_add_explicit(&g_hz3_arena_pressure_epoch, 1, memory_order_release);

    // Yield + aggressive focused reclaim: give threads time to respond, then reclaim.
    for (int retry = 0; retry < 32; retry++) {
        sched_yield();
#if HZ3_MEM_BUDGET_ENABLE && HZ3_S45_FOCUS_RECLAIM
        // Multiple passes of focused reclaim per yield (objects flow in gradually)
        for (int pass = 0; pass < 8; pass++) {
            Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();
            if (prog.freed_bytes > 0) {
                seg = hz3_arena_try_alloc_slot(idx_out);
                if (seg) return seg;
            }
            if (prog.sampled == 0) break;
        }
#endif
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) return seg;
    }
#endif

#if HZ3_MEM_BUDGET_ENABLE
    // Emergency flush (event-only): feed central before reclaim.
    hz3_mem_budget_emergency_flush();

#if HZ3_S45_EMERGENCY_FLUSH_REMOTE && HZ3_PTAG_DSTBIN_ENABLE
    // S45-FOCUS: Flush remote stash to central (increase sampling pool)
    // TLS guard: remote flush touches TLS, skip if not initialized
    if (t_hz3_cache.initialized) {
        hz3_dstbin_flush_remote_all();
    }
#endif

#if HZ3_S45_FOCUS_RECLAIM
    // S45-FOCUS: Multi-pass focused reclaim
    // Each pass samples from central and focuses on ONE segment
    // Progress tracking: sampled==0 || freed_pages==0 → break immediately
    for (int pass = 0; pass < HZ3_S45_FOCUS_PASSES; pass++) {
        Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();

        // Check for segment release success
        if (prog.freed_bytes > 0) {
            seg = hz3_arena_try_alloc_slot(idx_out);
            if (seg) {
                return seg;
            }
            // Slot taken by another thread → fall through to Phase 2
            break;
        }

        // Progress check: no samples OR no pages freed → stop early
        if (prog.sampled == 0 || prog.freed_pages == 0) {
            break;
        }
        // sampled > 0 && freed_pages > 0 but segment not released
        // → continue to next pass (pages accumulate in segment)
    }
#endif

    // S45 Phase 2: Pop medium runs from central and return to segments
    // This increases segment free_pages, enabling Phase 1 reclaim
    (void)hz3_mem_budget_reclaim_medium_runs();

    // S45 Phase 1: Free fully-empty segments back to arena
    size_t reclaimed = hz3_mem_budget_reclaim_segments(HZ3_SEG_SIZE);
    if (reclaimed > 0) {
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) {
            return seg;
        }
    }
#endif
#endif  // HZ3_S47_ARENA_GATE

#if HZ3_S47_SEGMENT_QUARANTINE
    // S47-PolicyBox: Signal final allocation failure
    hz3_s47_policy_on_alloc_failed();
#endif

#if HZ3_OOM_SHOT || HZ3_OOM_FAILFAST
    if (atomic_exchange_explicit(&g_hz3_arena_alloc_full_diag_fired, 1, memory_order_relaxed) == 0) {
        uint32_t used_slots = 0;
        uint32_t total_slots = g_hz3_arena.slots;
        uint32_t headroom_slots = 0;
        uint32_t free_gen = 0;
        uint32_t max_potential = 0;
        uint32_t pinned_candidates = 0;
        uint32_t avoid_hits = 0;
        uint32_t avoid_used = 0;
        uint32_t gate_leader = 0;
        uint32_t gate_ok = 0;
        uint32_t gate_timeout = 0;
        uint32_t gate_seg_freed = 0;
        uint32_t pressure_epoch = 0;
        uint32_t pack_hits = 0;
        uint32_t pack_misses = 0;

#if HZ3_S47_SEGMENT_QUARANTINE
        used_slots = hz3_arena_used_slots();
        total_slots = hz3_arena_total_slots();
        headroom_slots = hz3_s47_policy_get_headroom_slots();
        free_gen = hz3_arena_free_gen();
        max_potential = hz3_s47_get_max_potential();
        pinned_candidates = hz3_s47_get_pinned_candidates();
        avoid_hits = hz3_s47_get_avoid_hits();
        avoid_used = hz3_s47_get_avoid_used();
#endif

#if HZ3_S47_ARENA_GATE
        gate_leader = atomic_load_explicit(&g_hz3_gate_leader_count, memory_order_relaxed);
        gate_ok = atomic_load_explicit(&g_hz3_gate_follower_wait_ok, memory_order_relaxed);
        gate_timeout = atomic_load_explicit(&g_hz3_gate_follower_timeout, memory_order_relaxed);
        gate_seg_freed = atomic_load_explicit(&g_hz3_gate_seg_freed, memory_order_relaxed);
#endif

#if HZ3_ARENA_PRESSURE_BOX
        pressure_epoch = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_relaxed);
#endif

#if HZ3_S49_SEGMENT_PACKING
        pack_hits = hz3_pack_get_hits();
        pack_misses = hz3_pack_get_misses();
#endif

        fprintf(stderr,
                "[HZ3_ARENA_ALLOC_FULL] used=%u/%u headroom=%u seg_size=0x%llx pages_per_seg=%u "
                "max_potential=%u pinned=%u avoid_hits=%u avoid_used=%u "
                "gate_leader=%u gate_ok=%u gate_timeout=%u gate_seg_freed=%u free_gen=%u "
                "pressure_epoch=%u pack_hit=%u pack_miss=%u\n",
                used_slots, total_slots, headroom_slots,
                (unsigned long long)HZ3_SEG_SIZE, (unsigned)HZ3_PAGES_PER_SEG,
                max_potential, pinned_candidates, avoid_hits, avoid_used,
                gate_leader, gate_ok, gate_timeout, gate_seg_freed, free_gen,
                pressure_epoch, pack_hits, pack_misses);
    }
#endif  // HZ3_OOM_SHOT || HZ3_OOM_FAILFAST

    hz3_oom_note("arena_alloc_full", g_hz3_arena.slots, g_hz3_arena.size);
    return NULL;
}
