#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_inbox_push_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_push_live_owner_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_push_orphan_owner_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_orphan_to_central_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_live_to_central_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_exiting_to_central_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_drain_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_peak_objs[HZ3_NUM_SC];
static pthread_once_t g_s65_inbox_stats_once = PTHREAD_ONCE_INIT;

static void hz3_s65_inbox_update_peak(_Atomic(uint64_t)* peak, uint64_t value) {
    uint64_t old = atomic_load_explicit(peak, memory_order_relaxed);
    while (old < value &&
           !atomic_compare_exchange_weak_explicit(peak, &old, value,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
    }
}

static void hz3_s65_inbox_add_current(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    uint64_t after = atomic_fetch_add_explicit(&g_s65_inbox_current_objs[sc], n, memory_order_relaxed) + n;
    hz3_s65_inbox_update_peak(&g_s65_inbox_peak_objs[sc], after);
}

static void hz3_s65_inbox_sub_current(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    uint64_t old = atomic_load_explicit(&g_s65_inbox_current_objs[sc], memory_order_relaxed);
    for (;;) {
        uint64_t next = old > n ? old - n : 0;
        if (atomic_compare_exchange_weak_explicit(&g_s65_inbox_current_objs[sc], &old, next,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return;
        }
    }
}

static void hz3_s65_inbox_note_push(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_inbox_push_objs[sc], n, memory_order_relaxed);
    hz3_s65_inbox_add_current(sc, n);
}

static void hz3_s65_inbox_note_push_owner(uint8_t owner, int sc, uint64_t n) {
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    _Atomic(uint64_t)* counter = hz3_shard_live_count(owner) > 0
        ? &g_s65_inbox_push_live_owner_objs[sc]
        : &g_s65_inbox_push_orphan_owner_objs[sc];
    atomic_fetch_add_explicit(counter, n, memory_order_relaxed);
}

static void hz3_s65_inbox_note_orphan_to_central(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_inbox_orphan_to_central_objs[sc],
                              n,
                              memory_order_relaxed);
}

static void hz3_s65_inbox_note_live_to_central(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_inbox_live_to_central_objs[sc],
                              n,
                              memory_order_relaxed);
}

static void hz3_s65_inbox_note_exiting_to_central(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_inbox_exiting_to_central_objs[sc],
                              n,
                              memory_order_relaxed);
}

static void hz3_s65_inbox_note_drain(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_inbox_drain_objs[sc], n, memory_order_relaxed);
    hz3_s65_inbox_sub_current(sc, n);
}

static uint64_t hz3_s65_inbox_sc_bytes(int sc, uint64_t objs) {
    return objs * (uint64_t)hz3_sc_to_size(sc);
}

static void hz3_s65_inbox_dump(void) {
    uint64_t total_push = 0;
    uint64_t total_drain = 0;
    uint64_t total_current = 0;
    uint64_t total_peak = 0;
    uint64_t total_current_bytes = 0;
    uint64_t total_peak_bytes = 0;
    uint64_t total_push_live_owner = 0;
    uint64_t total_push_orphan_owner = 0;
    uint64_t total_orphan_to_central = 0;
    uint64_t total_live_to_central = 0;
    uint64_t total_exiting_to_central = 0;
    uint64_t snapshot_current[HZ3_NUM_SC] = {0};
    uint64_t live_snapshot_current[HZ3_NUM_SC] = {0};
    uint64_t orphan_snapshot_current[HZ3_NUM_SC] = {0};
    uint64_t snapshot_total_current = 0;
    uint64_t snapshot_total_bytes = 0;
    uint64_t live_snapshot_total_current = 0;
    uint64_t live_snapshot_total_bytes = 0;
    uint64_t orphan_snapshot_total_current = 0;
    uint64_t orphan_snapshot_total_bytes = 0;
    uint64_t mixed_snapshot_objs = 0;
    uint64_t mixed_live_snapshot_objs = 0;
    uint64_t mixed_orphan_snapshot_objs = 0;
    uint64_t live_shards = 0;
    uint64_t live_nonempty_shards = 0;
    uint64_t orphan_nonempty_shards = 0;
    uint8_t shard_live[HZ3_NUM_SHARDS] = {0};
    uint8_t live_shard_nonempty[HZ3_NUM_SHARDS] = {0};
    uint8_t orphan_shard_nonempty[HZ3_NUM_SHARDS] = {0};

    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        if (hz3_shard_live_count((uint8_t)shard) > 0) {
            shard_live[shard] = 1;
            live_shards++;
        }
    }

    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t push = atomic_load_explicit(&g_s65_inbox_push_objs[sc], memory_order_relaxed);
        uint64_t live_push =
            atomic_load_explicit(&g_s65_inbox_push_live_owner_objs[sc], memory_order_relaxed);
        uint64_t orphan_push =
            atomic_load_explicit(&g_s65_inbox_push_orphan_owner_objs[sc], memory_order_relaxed);
        uint64_t orphan_to_central =
            atomic_load_explicit(&g_s65_inbox_orphan_to_central_objs[sc], memory_order_relaxed);
        uint64_t live_to_central =
            atomic_load_explicit(&g_s65_inbox_live_to_central_objs[sc], memory_order_relaxed);
        uint64_t exiting_to_central =
            atomic_load_explicit(&g_s65_inbox_exiting_to_central_objs[sc], memory_order_relaxed);
        uint64_t drain = atomic_load_explicit(&g_s65_inbox_drain_objs[sc], memory_order_relaxed);
        uint64_t cur = atomic_load_explicit(&g_s65_inbox_current_objs[sc], memory_order_relaxed);
        uint64_t peak = atomic_load_explicit(&g_s65_inbox_peak_objs[sc], memory_order_relaxed);
        total_push += push;
        total_push_live_owner += live_push;
        total_push_orphan_owner += orphan_push;
        total_orphan_to_central += orphan_to_central;
        total_live_to_central += live_to_central;
        total_exiting_to_central += exiting_to_central;
        total_drain += drain;
        total_current += cur;
        total_peak += peak;
        total_current_bytes += hz3_s65_inbox_sc_bytes(sc, cur);
        total_peak_bytes += hz3_s65_inbox_sc_bytes(sc, peak);
        for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
            for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
                uint64_t count =
                    atomic_load_explicit(&g_hz3_inbox[shard][sc].count[lane], memory_order_relaxed);
                snapshot_current[sc] += count;
                if (count > 0) {
                    if (shard_live[shard]) {
                        live_snapshot_current[sc] += count;
                        live_shard_nonempty[shard] = 1;
                    } else {
                        orphan_snapshot_current[sc] += count;
                        orphan_shard_nonempty[shard] = 1;
                    }
                }
            }
        }
        snapshot_total_current += snapshot_current[sc];
        snapshot_total_bytes += hz3_s65_inbox_sc_bytes(sc, snapshot_current[sc]);
        live_snapshot_total_current += live_snapshot_current[sc];
        live_snapshot_total_bytes += hz3_s65_inbox_sc_bytes(sc, live_snapshot_current[sc]);
        orphan_snapshot_total_current += orphan_snapshot_current[sc];
        orphan_snapshot_total_bytes += hz3_s65_inbox_sc_bytes(sc, orphan_snapshot_current[sc]);
    }

    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
            uint64_t count =
                atomic_load_explicit(&g_hz3_inbox_medium_mixed[shard].count[lane],
                                     memory_order_relaxed);
            mixed_snapshot_objs += count;
            if (count > 0) {
                if (shard_live[shard]) {
                    mixed_live_snapshot_objs += count;
                    live_shard_nonempty[shard] = 1;
                } else {
                    mixed_orphan_snapshot_objs += count;
                    orphan_shard_nonempty[shard] = 1;
                }
            }
        }
    }

    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        if (live_shard_nonempty[shard]) {
            live_nonempty_shards++;
        }
        if (orphan_shard_nonempty[shard]) {
            orphan_nonempty_shards++;
        }
    }

    if (total_push == 0 && total_drain == 0 && total_current == 0 && total_peak == 0 &&
        snapshot_total_current == 0 && mixed_snapshot_objs == 0 &&
        total_orphan_to_central == 0 && total_live_to_central == 0 &&
        total_exiting_to_central == 0) {
        return;
    }

    fprintf(stderr,
            "[HZ3_S65_INBOX] push=%" PRIu64 " drain=%" PRIu64
            " current=%" PRIu64 " peak=%" PRIu64
            " current_bytes=%" PRIu64 " peak_bytes=%" PRIu64
            " snapshot_current=%" PRIu64 " snapshot_bytes=%" PRIu64
            " mixed_snapshot=%" PRIu64 "\n",
            total_push,
            total_drain,
            total_current,
            total_peak,
            total_current_bytes,
            total_peak_bytes,
            snapshot_total_current,
            snapshot_total_bytes,
            mixed_snapshot_objs);
    fprintf(stderr, "  by_sc:");
    int printed = 0;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t push = atomic_load_explicit(&g_s65_inbox_push_objs[sc], memory_order_relaxed);
        uint64_t drain = atomic_load_explicit(&g_s65_inbox_drain_objs[sc], memory_order_relaxed);
        uint64_t cur = atomic_load_explicit(&g_s65_inbox_current_objs[sc], memory_order_relaxed);
        uint64_t peak = atomic_load_explicit(&g_s65_inbox_peak_objs[sc], memory_order_relaxed);
        uint64_t snap = snapshot_current[sc];
        if (push > 0 || drain > 0 || cur > 0 || peak > 0 || snap > 0) {
            fprintf(stderr,
                    " sc%d=%" PRIu64 "p/%" PRIu64 "d/%" PRIu64 "c/%" PRIu64 "k/%" PRIu64 "s",
                    sc,
                    push,
                    drain,
                    cur,
                    peak,
                    snap);
            printed = 1;
        }
    }
    if (!printed) {
        fprintf(stderr, " none");
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "  bytes_by_sc:");
    printed = 0;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t cur = atomic_load_explicit(&g_s65_inbox_current_objs[sc], memory_order_relaxed);
        uint64_t peak = atomic_load_explicit(&g_s65_inbox_peak_objs[sc], memory_order_relaxed);
        uint64_t snap = snapshot_current[sc];
        if (cur > 0 || peak > 0 || snap > 0) {
            fprintf(stderr,
                    " sc%d=%" PRIu64 "cb/%" PRIu64 "kb/%" PRIu64 "sb",
                    sc,
                    hz3_s65_inbox_sc_bytes(sc, cur),
                    hz3_s65_inbox_sc_bytes(sc, peak),
                    hz3_s65_inbox_sc_bytes(sc, snap));
            printed = 1;
        }
    }
    if (!printed) {
        fprintf(stderr, " none");
    }
    fprintf(stderr, "\n");

    if (total_push_live_owner > 0 || total_push_orphan_owner > 0 ||
        total_orphan_to_central > 0 || total_live_to_central > 0 ||
        total_exiting_to_central > 0) {
        fprintf(stderr,
                "[HZ3_S65_INBOX_OWNER_PUSH] live_owner_push=%" PRIu64
                " orphan_owner_push=%" PRIu64
                " orphan_to_central=%" PRIu64
                " live_to_central=%" PRIu64
                " exiting_to_central=%" PRIu64 "\n",
                total_push_live_owner,
                total_push_orphan_owner,
                total_orphan_to_central,
                total_live_to_central,
                total_exiting_to_central);
        fprintf(stderr, "  owner_push_by_sc:");
        printed = 0;
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            uint64_t live_push =
                atomic_load_explicit(&g_s65_inbox_push_live_owner_objs[sc], memory_order_relaxed);
            uint64_t orphan_push =
                atomic_load_explicit(&g_s65_inbox_push_orphan_owner_objs[sc], memory_order_relaxed);
            uint64_t orphan_to_central =
                atomic_load_explicit(&g_s65_inbox_orphan_to_central_objs[sc],
                                     memory_order_relaxed);
            uint64_t live_to_central =
                atomic_load_explicit(&g_s65_inbox_live_to_central_objs[sc],
                                     memory_order_relaxed);
            uint64_t exiting_to_central =
                atomic_load_explicit(&g_s65_inbox_exiting_to_central_objs[sc],
                                     memory_order_relaxed);
            if (live_push > 0 || orphan_push > 0 || orphan_to_central > 0 ||
                live_to_central > 0 || exiting_to_central > 0) {
                fprintf(stderr,
                        " sc%d=%" PRIu64 "live/%" PRIu64 "orphan/%" PRIu64
                        "central/%" PRIu64 "live_central/%" PRIu64 "exiting_central",
                        sc,
                        live_push,
                        orphan_push,
                        orphan_to_central,
                        live_to_central,
                        exiting_to_central);
                printed = 1;
            }
        }
        if (!printed) {
            fprintf(stderr, " none");
        }
        fprintf(stderr, "\n");

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM && HZ3_S65_MEDIUM_RECLAIM
        fprintf(stderr,
                "[HZ3_S65_INBOX_RECLAIM_POLICY] scale=%u max_runs=%u stride=%u batch_runs=%u",
                (unsigned)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE,
                (unsigned)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS,
                (unsigned)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE,
                (unsigned)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS);
#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS > 1
        fprintf(stderr,
                " batch_flushes=%" PRIu64
                " batch_budget=%" PRIu64
                " batch_pending_peak=%" PRIu64,
                atomic_load_explicit(&g_s65_inbox_to_central_reclaim_batch_flushes,
                                     memory_order_relaxed),
                atomic_load_explicit(&g_s65_inbox_to_central_reclaim_batch_budget,
                                     memory_order_relaxed),
                atomic_load_explicit(&g_s65_inbox_to_central_reclaim_batch_pending_peak,
                                     memory_order_relaxed));
#endif
        fprintf(stderr, "\n");
#endif
    }

    if (snapshot_total_current > 0 || mixed_snapshot_objs > 0) {
        fprintf(stderr,
                "[HZ3_S65_INBOX_OWNER_SNAPSHOT] live_shards=%" PRIu64
                " live_nonempty_shards=%" PRIu64
                " orphan_nonempty_shards=%" PRIu64
                " live_objs=%" PRIu64 " live_bytes=%" PRIu64
                " orphan_objs=%" PRIu64 " orphan_bytes=%" PRIu64
                " mixed_live=%" PRIu64 " mixed_orphan=%" PRIu64 "\n",
                live_shards,
                live_nonempty_shards,
                orphan_nonempty_shards,
                live_snapshot_total_current,
                live_snapshot_total_bytes,
                orphan_snapshot_total_current,
                orphan_snapshot_total_bytes,
                mixed_live_snapshot_objs,
                mixed_orphan_snapshot_objs);
        fprintf(stderr, "  live_by_sc:");
        printed = 0;
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            if (live_snapshot_current[sc] > 0) {
                fprintf(stderr, " sc%d=%" PRIu64, sc, live_snapshot_current[sc]);
                printed = 1;
            }
        }
        if (!printed) {
            fprintf(stderr, " none");
        }
        fprintf(stderr, "\n");

        fprintf(stderr, "  orphan_by_sc:");
        printed = 0;
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            if (orphan_snapshot_current[sc] > 0) {
                fprintf(stderr, " sc%d=%" PRIu64, sc, orphan_snapshot_current[sc]);
                printed = 1;
            }
        }
        if (!printed) {
            fprintf(stderr, " none");
        }
        fprintf(stderr, "\n");

        fprintf(stderr, "  owner_bytes_by_sc:");
        printed = 0;
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            if (live_snapshot_current[sc] > 0 || orphan_snapshot_current[sc] > 0) {
                fprintf(stderr,
                        " sc%d=%" PRIu64 "liveb/%" PRIu64 "orphanb",
                        sc,
                        hz3_s65_inbox_sc_bytes(sc, live_snapshot_current[sc]),
                        hz3_s65_inbox_sc_bytes(sc, orphan_snapshot_current[sc]));
                printed = 1;
            }
        }
        if (!printed) {
            fprintf(stderr, " none");
        }
        fprintf(stderr, "\n");
    }
}

static void hz3_s65_inbox_register_once(void) {
    atexit(hz3_s65_inbox_dump);
}
#else
#define hz3_s65_inbox_note_push(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_inbox_note_push_owner(owner, sc, n) do { (void)(owner); (void)(sc); (void)(n); } while (0)
#define hz3_s65_inbox_note_orphan_to_central(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_inbox_note_live_to_central(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_inbox_note_exiting_to_central(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_inbox_note_drain(sc, n) do { (void)(sc); (void)(n); } while (0)
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM && HZ3_S65_MEDIUM_RECLAIM && \
    HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE > 1
static _Atomic(uint64_t) g_s65_inbox_to_central_reclaim_stride_ticket;
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM && HZ3_S65_MEDIUM_RECLAIM && \
    HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS > 1
static inline void hz3_s65_inbox_reclaim_batch_note_pending(uint64_t pending) {
    uint64_t old =
        atomic_load_explicit(&g_s65_inbox_to_central_reclaim_batch_pending_peak,
                             memory_order_relaxed);
    while (old < pending &&
           !atomic_compare_exchange_weak_explicit(
               &g_s65_inbox_to_central_reclaim_batch_pending_peak,
               &old,
               pending,
               memory_order_relaxed,
               memory_order_relaxed)) {
    }
}

static inline uint64_t hz3_s65_inbox_reclaim_batch_take(uint8_t owner,
                                                        int sc,
                                                        uint64_t add) {
    _Atomic(uint64_t)* pending =
        &g_s65_inbox_to_central_reclaim_pending[owner][sc];
    uint64_t after =
        atomic_fetch_add_explicit(pending, add, memory_order_relaxed) + add;
    hz3_s65_inbox_reclaim_batch_note_pending(after);
    if (after < (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS) {
        return 0;
    }

    uint64_t old = atomic_load_explicit(pending, memory_order_relaxed);
    while (old >= (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS) {
        uint64_t take = old;
        if (take > (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS) {
            take = (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS;
        }
        uint64_t desired = old - take;
        if (atomic_compare_exchange_weak_explicit(pending,
                                                  &old,
                                                  desired,
                                                  memory_order_acq_rel,
                                                  memory_order_relaxed)) {
            atomic_fetch_add_explicit(
                &g_s65_inbox_to_central_reclaim_batch_flushes,
                1,
                memory_order_relaxed);
            atomic_fetch_add_explicit(
                &g_s65_inbox_to_central_reclaim_batch_budget,
                take,
                memory_order_relaxed);
            hz3_s65_inbox_reclaim_batch_note_pending(desired);
            return take;
        }
    }

    return 0;
}
#endif

static inline void hz3_s65_inbox_reclaim_after_to_central(uint8_t owner,
                                                          int sc,
                                                          uint32_t n,
                                                          uint32_t owner_live_count) {
#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM && HZ3_S65_MEDIUM_RECLAIM
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
#if !HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_ALLOW_LIVE_EXITING
    if (owner_live_count > 0) {
        return;
    }
#else
    (void)owner_live_count;
#endif

    size_t sc_pages = hz3_sc_to_pages(sc);
    uint32_t sc_page_bit =
        sc_pages < 32 ? (uint32_t)(1u << (uint32_t)sc_pages) : 0u;
    if (sc_pages < (size_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES ||
        sc_pages > (size_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES ||
        (sc_page_bit & (uint32_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_PAGE_MASK) == 0) {
        return;
    }

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS > 1
    uint64_t add = (uint64_t)n * (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE;
    uint64_t budget = hz3_s65_inbox_reclaim_batch_take(owner, sc, add);
    if (budget == 0) {
        return;
    }
#else
#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE > 1
    uint64_t ticket = atomic_fetch_add_explicit(
                          &g_s65_inbox_to_central_reclaim_stride_ticket,
                          1,
                          memory_order_relaxed) +
                      1u;
    if ((ticket % (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE) != 0) {
        return;
    }
#endif

    uint64_t budget = (uint64_t)n * (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE;
    if (budget > (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS) {
        budget = (uint64_t)HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS;
    }
#endif
    hz3_s65_medium_reclaim_shard_sc(owner,
                                    sc,
                                    (uint32_t)budget,
                                    HZ3_S65_RECLAIM_REASON_INBOX_TO_CENTRAL);
#else
    (void)owner;
    (void)sc;
    (void)n;
    (void)owner_live_count;
#endif
}
