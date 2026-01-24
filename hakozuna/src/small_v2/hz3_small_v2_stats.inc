// hz3_small_v2_stats.inc - TINY_STATS, OOM_SHOT, S98 stats
// Part of hz3_small_v2.c (single TU split)

// S28: Tiny slow path stats (event-only)
#if HZ3_TINY_STATS
static _Atomic uint64_t g_tiny_slow_calls[HZ3_SMALL_NUM_SC];
static _Atomic uint64_t g_tiny_central_hit[HZ3_SMALL_NUM_SC];
static _Atomic uint64_t g_tiny_page_alloc[HZ3_SMALL_NUM_SC];
static pthread_once_t g_tiny_stats_atexit_once = PTHREAD_ONCE_INIT;

static void hz3_tiny_stats_dump(void) {
    uint64_t total_slow = 0, total_central_hit = 0, total_page_alloc = 0;
    fprintf(stderr, "[hz3] S28 tiny stats (sc 0-15 = 16-256 bytes):\n");
    for (int sc = 0; sc < 16; sc++) {  // tiny = sc 0-15
        uint64_t slow = atomic_load(&g_tiny_slow_calls[sc]);
        uint64_t hit = atomic_load(&g_tiny_central_hit[sc]);
        uint64_t page = atomic_load(&g_tiny_page_alloc[sc]);
        if (slow > 0) {
            fprintf(stderr, "  sc=%2d (%3zuB): slow=%lu central_hit=%lu page_alloc=%lu\n",
                    sc, (size_t)(sc + 1) * 16, slow, hit, page);
        }
        total_slow += slow;
        total_central_hit += hit;
        total_page_alloc += page;
    }
    fprintf(stderr, "[hz3] tiny totals: slow=%lu central_hit=%lu page_alloc=%lu\n",
            total_slow, total_central_hit, total_page_alloc);
    if (total_slow > 0) {
        fprintf(stderr, "[hz3] tiny central_hit_rate=%.2f%% page_alloc_rate=%.2f%%\n",
                100.0 * total_central_hit / total_slow,
                100.0 * total_page_alloc / total_slow);
    }
}

static void hz3_tiny_stats_register_atexit(void) {
    atexit(hz3_tiny_stats_dump);
}
#endif

#if HZ3_OOM_SHOT
// OOM triage (event-only): track small_v2 page allocs by sc, dump once on OOM.
static _Atomic uint64_t g_small_v2_page_alloc_sc[HZ3_SMALL_NUM_SC];
static _Atomic int g_small_v2_oom_dumped = 0;

static void hz3_small_v2_oom_dump(int sc) {
    if (atomic_exchange_explicit(&g_small_v2_oom_dumped, 1, memory_order_relaxed) != 0) {
        return;
    }
    fprintf(stderr, "[HZ3_OOM_SMALL_V2] sc=%d size=%zu\n",
            sc, hz3_small_sc_to_size(sc));
    for (int i = 0; i < HZ3_SMALL_NUM_SC; i++) {
        uint64_t cnt = atomic_load_explicit(&g_small_v2_page_alloc_sc[i], memory_order_relaxed);
        if (cnt == 0) {
            continue;
        }
        fprintf(stderr, "  sc=%d size=%zu page_alloc=%lu\n",
                i, hz3_small_sc_to_size(i), cnt);
    }
}
#endif

// ============================================================================
// S98: push_remote_list stats (research, SSOT observation)
// ============================================================================
#if HZ3_S98_PUSH_REMOTE_STATS
HZ3_DTOR_STAT(g_s98_calls);
HZ3_DTOR_STAT(g_s98_objs);
HZ3_DTOR_STAT(g_s98_n1_calls);
HZ3_DTOR_STAT(g_s98_dst_local);
HZ3_DTOR_STAT(g_s98_dst_remote);
HZ3_DTOR_STAT(g_s98_s44_ok);
HZ3_DTOR_STAT(g_s98_s44_overflow);
HZ3_DTOR_STAT(g_s98_s42_entry);     // S111-0: entered S42 xfer path
HZ3_DTOR_STAT(g_s98_central_direct); // S111-0: direct to central (S42 disabled or S44 only)
HZ3_DTOR_STAT(g_s98_sc_lt32_objs);
HZ3_DTOR_STAT(g_s98_sc_ge32_objs);
HZ3_DTOR_ATEXIT_FLAG(g_s98);

static void hz3_s98_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s98_calls);
    uint32_t objs = HZ3_DTOR_STAT_LOAD(g_s98_objs);
    uint32_t n1_calls = HZ3_DTOR_STAT_LOAD(g_s98_n1_calls);
    uint32_t dst_local = HZ3_DTOR_STAT_LOAD(g_s98_dst_local);
    uint32_t dst_remote = HZ3_DTOR_STAT_LOAD(g_s98_dst_remote);
    uint32_t s44_ok = HZ3_DTOR_STAT_LOAD(g_s98_s44_ok);
    uint32_t s44_overflow = HZ3_DTOR_STAT_LOAD(g_s98_s44_overflow);
    uint32_t s42_entry = HZ3_DTOR_STAT_LOAD(g_s98_s42_entry);
    uint32_t central_direct = HZ3_DTOR_STAT_LOAD(g_s98_central_direct);
    uint32_t sc_lt32 = HZ3_DTOR_STAT_LOAD(g_s98_sc_lt32_objs);
    uint32_t sc_ge32 = HZ3_DTOR_STAT_LOAD(g_s98_sc_ge32_objs);

    if (calls == 0) {
        return;
    }

    int n1_pct = (int)((uint64_t)n1_calls * 100 / calls);
    int local_pct = (int)((uint64_t)dst_local * 100 / calls);
    int remote_pct = (int)((uint64_t)dst_remote * 100 / calls);
    int s44_ok_pct = (s44_ok + s44_overflow > 0)
                     ? (int)((uint64_t)s44_ok * 100 / (s44_ok + s44_overflow))
                     : 0;
    int sc_lt32_pct = (objs > 0)
                      ? (int)((uint64_t)sc_lt32 * 100 / objs)
                      : 0;
    int sc_ge32_pct = (objs > 0)
                      ? (int)((uint64_t)sc_ge32 * 100 / objs)
                      : 0;

    fprintf(stderr,
            "[HZ3_S98] calls=%u objs=%u n1_pct=%d%% local_pct=%d%% remote_pct=%d%% "
            "s44_ok_pct=%d%% s42=%u central=%u sc_lt32_pct=%d%% sc_ge32_pct=%d%%\n",
            calls, objs, n1_pct, local_pct, remote_pct, s44_ok_pct,
            s42_entry, central_direct, sc_lt32_pct, sc_ge32_pct);
}
#endif  // HZ3_S98_PUSH_REMOTE_STATS
