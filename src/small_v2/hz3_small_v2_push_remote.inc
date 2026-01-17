// hz3_small_v2_push_remote.inc - push_remote_list implementation
// Part of hz3_small_v2.c (single TU split)

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

void hz3_small_v2_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)owner;
    (void)sc;
    (void)head;
    (void)tail;
    (void)n;
#else
    if (!head || !tail || n == 0) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }
#if HZ3_S98_PUSH_REMOTE_STATS
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s98, hz3_s98_atexit_dump);
    HZ3_DTOR_STAT_INC(g_s98_calls);
    HZ3_DTOR_STAT_ADD(g_s98_objs, n);
    if (n == 1) {
        HZ3_DTOR_STAT_INC(g_s98_n1_calls);
    }
    // Guard: only check my_shard if tcache is initialized (stats-only safety)
    if (t_hz3_cache.initialized && owner == (uint8_t)t_hz3_cache.my_shard) {
        HZ3_DTOR_STAT_INC(g_s98_dst_local);
    } else {
        HZ3_DTOR_STAT_INC(g_s98_dst_remote);
    }
    if (sc < 32) {
        HZ3_DTOR_STAT_ADD(g_s98_sc_lt32_objs, n);
    } else {
        HZ3_DTOR_STAT_ADD(g_s98_sc_ge32_objs, n);
    }
#endif

#if HZ3_S111_REMOTE_PUSH_N1
    // S111: n==1 fastpath - use push_one() instead of push_list()
    if (n == 1) {
#if HZ3_S111_REMOTE_PUSH_N1_FAILFAST
        if (head != tail) {
            fprintf(stderr, "[HZ3_S111_FAILFAST] n==1 but head!=tail\n");
            abort();
        }
#endif
        // Try S44 push_one
#if HZ3_S44_OWNER_STASH_PUSH && !HZ3_S44_OWNER_STASH_DISABLE
        if (hz3_owner_stash_push_one(owner, sc, head)) {
#if HZ3_S111_REMOTE_PUSH_N1_STATS && HZ3_S98_PUSH_REMOTE_STATS
            HZ3_DTOR_STAT_INC(g_s98_s44_ok);  // reuse S98 stat
#endif
            return;
        }
#if HZ3_S111_REMOTE_PUSH_N1_STATS && HZ3_S98_PUSH_REMOTE_STATS
        HZ3_DTOR_STAT_INC(g_s98_s44_overflow);
#endif
#endif
        // Try S42 push_one
#if HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE
        if (hz3_small_xfer_push_one(owner, sc, head)) {
#if HZ3_S111_REMOTE_PUSH_N1_STATS && HZ3_S98_PUSH_REMOTE_STATS
            HZ3_DTOR_STAT_INC(g_s98_s42_entry);
#endif
            return;
        }
#endif
        // Central push_one (final fallback)
#if HZ3_S111_REMOTE_PUSH_N1_STATS && HZ3_S98_PUSH_REMOTE_STATS
        HZ3_DTOR_STAT_INC(g_s98_central_direct);
#endif
        hz3_small_v2_central_init();
        hz3_small_v2_central_push_one(owner, sc, head);
        return;
    }
#endif  // HZ3_S111_REMOTE_PUSH_N1

    // --- Existing n>=1 path below ---
#if HZ3_LIST_FAILFAST
    // Common entry check: verify list is valid before S44/S42/central
    hz3_list_failfast("remote_list:entry", owner, sc, head, tail, n);
#endif
#if HZ3_S72_BOUNDARY_DEBUG
    hz3_small_v2_boundary_check_list("small_v2:remote_list", owner, sc, head, tail, n);
#endif
#if HZ3_S44_OWNER_STASH_PUSH && !HZ3_S44_OWNER_STASH_DISABLE
    // S44: Push to owner's stash (MPSC, no mutex)
    // If successful, return. If overflow, fall through to S42/central.
    if (hz3_owner_stash_push_list(owner, sc, head, tail, n)) {
#if HZ3_S98_PUSH_REMOTE_STATS
        HZ3_DTOR_STAT_INC(g_s98_s44_ok);
#endif
        return;
    }
#if HZ3_S98_PUSH_REMOTE_STATS
    HZ3_DTOR_STAT_INC(g_s98_s44_overflow);
#endif
#endif
#if HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE
    // S42: Push to transfer cache (falls back to central if full)
#if HZ3_S98_PUSH_REMOTE_STATS
    HZ3_DTOR_STAT_INC(g_s98_s42_entry);
#endif
    hz3_small_xfer_push_list(owner, sc, head, tail, n);
#else
    // Direct to central (S42 disabled)
#if HZ3_S98_PUSH_REMOTE_STATS
    HZ3_DTOR_STAT_INC(g_s98_central_direct);
#endif
    hz3_small_v2_central_init();
    hz3_small_v2_central_push_list(owner, sc, head, tail, n);
#endif
#endif
}

#endif  // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
