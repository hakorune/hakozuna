// =============================================================================
// hz3_owner_stash_pop_batch: Pop up to `want` objects from owner stash
// =============================================================================
int hz3_owner_stash_pop_batch(uint8_t owner, int sc, void** out, int want) {
    // Struct failfast: check ranges before anything
    STASH_CHECK_RANGE("pop:entry", owner, sc);

    if (want <= 0) {
        return 0;
    }
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

#if HZ3_S162_OWNER_STASH_INIT_FASTPATH
    if (!t_hz3_cache.initialized) {
        hz3_owner_stash_init();
    }
#else
    hz3_owner_stash_init();
#endif

#if HZ3_S121_PAGE_LOCAL_REMOTE
#include "hz3_owner_stash_pop_s121_body.inc"
#endif  // HZ3_S121_PAGE_LOCAL_REMOTE

#if HZ3_S67_STATS
    HZ3_DTOR_STAT_INC(g_s67_pop_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s67, hz3_s67_atexit_dump);
#endif
#if HZ3_S112_STATS
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s112, hz3_s112_atexit_dump);
#endif
#if HZ3_S170_S112_BOUNDED_STATS
    S170_STAT_REGISTER();
#endif
#if HZ3_S94_STATS
    if (sc < HZ3_S94_SPILL_SC_MAX) {
        S94_STAT_INC(g_s94_pop_calls);
        S94_STAT_REGISTER();
    }
#endif
#if HZ3_S44_4_STATS
    S44_4_STAT_REGISTER();
    S44_4_STAT_INC(g_s44_4_pop_calls);
    S44_4_STAT_ADD(g_s44_4_want_sum32, (uint32_t)want);
    if (want == 32) {
        S44_4_STAT_INC(g_s44_4_want_32_count);
    }
#endif

#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Round-robin instance selection for pop (try 1 instance only)
    Hz3TCache* tc_s144 = &t_hz3_cache;
    Hz3OwnerStashBin* bin = hz3_s144_get_bin_pop(owner, sc, &tc_s144->owner_stash_rr);
#else
    Hz3OwnerStashBin* bin = &g_hz3_owner_stash[owner][sc];
#endif

    int got = 0;  // S48/S67/S67-2: defined at function start

    _Atomic(void*)* headp = &bin->head;

#if HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2
#include "hz3_owner_stash_pop_s93_body.inc"
#else  // !HZ3_S93_OWNER_STASH_PACKET && !HZ3_S93_OWNER_STASH_PACKET_V2
#include "hz3_owner_stash_pop_std_body.inc"
#endif  // HZ3_S93_OWNER_STASH_PACKET
}