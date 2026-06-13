static inline void hz3_remote_stash_dispatch_list(uint8_t dst, uint32_t bin,
                                                 void* head, void* tail, uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }
    S196_STAT_REGISTER();

    if (bin < HZ3_SMALL_NUM_SC) {
        S196_STAT_INC(g_s196_dispatch_small_calls);
        S196_STAT_ADD(g_s196_dispatch_small_objs, n);
#if HZ3_HZ4_BRIDGE
        hz3_hz4_bridge_push_remote_list_small(dst, (int)bin, head, tail, n);
#elif HZ3_LANE_T16_R90_PAGE_REMOTE
        hz3_lane16_push_remote_list_small(dst, (int)bin, head, tail, n);
#else
        hz3_small_v2_push_remote_list(dst, (int)bin, head, tail, n);
#endif
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        S196_STAT_INC(g_s196_dispatch_sub4k_calls);
        S196_STAT_ADD(g_s196_dispatch_sub4k_objs, n);
        int sc = (int)bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
    } else if (hz3_bin_is_medium_aligned(bin)) {
        int sc = hz3_bin_medium_aligned_sc(bin);
        hz3_inbox_aligned_push_list(dst, sc, head, tail, n);
#endif
    } else {
        int sc = (int)bin - HZ3_MEDIUM_BIN_BASE;
        int to_central = 0;
        int to_mailbox_s236 = 0;
        int to_central_s209 = 0;
        int to_central_s210 = 0;
        int to_central_s230 = 0;
        S196_STAT_INC(g_s196_dispatch_medium_calls);
        S196_STAT_ADD(g_s196_dispatch_medium_objs, n);
        S196_MEDIUM_SC_ADD(sc, n);

#if HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
        if (hz3_s205_medium_should_rescue(dst, sc, n)) {
            to_central = 1;
        }
#endif
#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG
        if (!to_central && hz3_s209_medium_should_rescue(dst, sc, n)) {
            to_central = 1;
            to_central_s209 = 1;
        }
#endif
#if HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
        if (!to_central && hz3_s210_medium_should_rescue(dst, sc, n)) {
            to_central = 1;
            to_central_s210 = 1;
        }
#endif
#if HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
        if (!to_central && sc >= HZ3_S230_SC_MIN && sc <= HZ3_S230_SC_MAX) {
#if HZ3_S230_SKIP_SELF_DST
            if (dst != t_hz3_cache.my_shard)
#endif
            {
#if HZ3_S230_ONLY_N_EQ_1
                if (n == 1 && head == tail)
#endif
                {
                    to_central = 1;
                    to_central_s230 = 1;
                }
            }
        }
#endif
#if HZ3_S236_MEDIUM_MAILBOX
        if (!to_central && !to_mailbox_s236 &&
            sc >= HZ3_S236_SC_MIN && sc <= HZ3_S236_SC_MAX) {
#if HZ3_S236_SKIP_SELF_DST
            if (dst != t_hz3_cache.my_shard)
#endif
            {
#if HZ3_S236_ONLY_N_EQ_1
                if (n == 1 && head == tail)
#endif
                {
                    if (hz3_s236_medium_mailbox_try_push(dst, sc, head)) {
                        to_mailbox_s236 = 1;
                    }
                }
            }
        }
#endif
#if HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL
        if (!to_central && n >= (uint32_t)HZ3_S202_MEDIUM_TO_CENTRAL_MIN_N) {
            to_central = 1;
        }
#endif

#if HZ3_S203_COUNTERS
        S203_STAT_INC(medium_dispatch_calls);
        S203_STAT_ADD(medium_dispatch_objs, n);
        t_s203_medium_dispatch_sc_calls[sc]++;
        t_s203_medium_dispatch_sc_objs[sc] += n;
        S204_SHARD_INC(dst);
        if (n == 1) S203_STAT_INC(medium_dispatch_n_hist_1);
        else if (n <= 3) S203_STAT_INC(medium_dispatch_n_hist_2_3);
        else if (n <= 7) S203_STAT_INC(medium_dispatch_n_hist_4_7);
        else if (n <= 15) S203_STAT_INC(medium_dispatch_n_hist_8_15);
        else S203_STAT_INC(medium_dispatch_n_hist_16_plus);

        if (to_mailbox_s236) {
            S203_STAT_INC(medium_dispatch_to_mailbox_calls);
            S203_STAT_ADD(medium_dispatch_to_mailbox_objs, n);
        } else if (to_central) {
            S203_STAT_INC(medium_dispatch_to_central_calls);
            S203_STAT_ADD(medium_dispatch_to_central_objs, n);
            if (to_central_s209) {
                S203_STAT_INC(medium_dispatch_to_central_s209_calls);
                S203_STAT_ADD(medium_dispatch_to_central_s209_objs, n);
            }
            if (to_central_s210) {
                S203_STAT_INC(medium_dispatch_to_central_s210_calls);
                S203_STAT_ADD(medium_dispatch_to_central_s210_objs, n);
            }
            if (to_central_s230) {
                S203_STAT_INC(medium_dispatch_to_central_s230_calls);
                S203_STAT_ADD(medium_dispatch_to_central_s230_objs, n);
            }
        } else {
            S203_STAT_INC(medium_dispatch_to_inbox_calls);
            S203_STAT_ADD(medium_dispatch_to_inbox_objs, n);
        }
#endif
        (void)to_central_s209;
        (void)to_central_s210;
        (void)to_central_s230;
        (void)to_mailbox_s236;

        if (to_mailbox_s236) {
            return;
        }
        if (to_central) {
            hz3_central_push_list((int)dst, sc, head, tail, n);
            return;
        }
        hz3_medium_dispatch_push(dst, sc, head, tail, n);
    }
}

static inline int hz3_remote_stash_try_direct_n1(uint8_t dst, uint32_t bin, void* ptr) {
#if HZ3_HZ4_BRIDGE
    if (bin < HZ3_SMALL_NUM_SC) {
        S196_STAT_REGISTER();
        S196_STAT_INC(g_s196_direct_n1_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_objs);
        hz3_hz4_bridge_push_remote_list_small(dst, (int)bin, ptr, ptr, 1);
        return 1;
    }
#elif HZ3_LANE_T16_R90_PAGE_REMOTE
    if (bin < HZ3_SMALL_NUM_SC) {
        S196_STAT_REGISTER();
        S196_STAT_INC(g_s196_direct_n1_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_objs);
        hz3_lane16_push_remote_list_small(dst, (int)bin, ptr, ptr, 1);
        return 1;
    }
#endif
#if HZ3_S161_REMOTE_STASH_N1_DIRECT && HZ3_S44_OWNER_STASH_PUSH && !HZ3_S44_OWNER_STASH_DISABLE
    if (bin < HZ3_SMALL_NUM_SC) {
        if (hz3_owner_stash_push_one(dst, (int)bin, ptr)) {
            S196_STAT_REGISTER();
            S196_STAT_INC(g_s196_direct_n1_small_calls);
            S196_STAT_INC(g_s196_dispatch_small_calls);
            S196_STAT_INC(g_s196_dispatch_small_objs);
            return 1;
        }
    }
#else
    (void)dst;
    (void)bin;
    (void)ptr;
#endif
    return 0;
}

static inline uint16_t hz3_remote_stash_entry_key_load(const Hz3RemoteStashEntry* entry) {
#if HZ3_RBUF_KEY
    return entry->key;
#else
    return (uint16_t)(((uint16_t)entry->dst << 8) | (uint16_t)entry->bin);
#endif
}

static inline int hz3_remote_stash_n1_direct_allowed(uint16_t h, uint16_t t) {
#if HZ3_S178_N1_DIRECT_RING_GUARD
    uint32_t remaining = (uint32_t)((h - t) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    return remaining <= (uint32_t)HZ3_S178_N1_DIRECT_MAX_RING;
#else
    (void)h;
    (void)t;
    return 1;
#endif
}
