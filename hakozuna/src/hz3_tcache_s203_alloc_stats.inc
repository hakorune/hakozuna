// ============================================================================
// S203: Alloc Slow Counters (Medium) - TLS Version
// ============================================================================
#if HZ3_S203_COUNTERS
HZ3_DTOR_STATS_BEGIN(S203_ALLOC);
static _Thread_local uint64_t t_s203_alloc_slow_calls = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_inbox = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_central = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_segment = 0;
static _Thread_local uint64_t t_s203_s206_steal_attempts = 0;
static _Thread_local uint64_t t_s203_s206_steal_hits = 0;
static _Thread_local uint64_t t_s203_s208_reserve_attempts = 0;
static _Thread_local uint64_t t_s203_s208_reserve_hits = 0;
static _Thread_local uint64_t t_s203_s208_central_miss_after_reserve = 0;
static _Thread_local uint64_t t_s203_s223_want_boost_calls = 0;
static _Thread_local uint64_t t_s203_s236_mini_calls = 0;
static _Thread_local uint64_t t_s203_s236_mini_hit_inbox = 0;
static _Thread_local uint64_t t_s203_s236_mini_hit_central = 0;
static _Thread_local uint64_t t_s203_s236_mini_miss = 0;
static _Thread_local uint64_t t_s203_s236e_retry_calls = 0;
static _Thread_local uint64_t t_s203_s236e_retry_hits = 0;
static _Thread_local uint64_t t_s203_s236e_retry_skipped_no_supply = 0;
static _Thread_local uint64_t t_s203_s236f_retry_calls = 0;
static _Thread_local uint64_t t_s203_s236f_retry_hits = 0;
static _Thread_local uint64_t t_s203_s236f_retry_skipped_streak = 0;
static _Thread_local uint64_t t_s203_s236g_hint_checks = 0;
static _Thread_local uint64_t t_s203_s236g_hint_positive = 0;
static _Thread_local uint64_t t_s203_s236g_hint_empty_skips = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_calls = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_hits = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_skip_no_hint = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_skip_no_backlog = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_calls = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_inbox_extra_objs = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_central_extra_objs = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_zero_extra = 0;
static _Thread_local uint64_t t_s203_s65_cold_calls = 0;
static _Thread_local uint64_t t_s203_s65_cold_hits = 0;
static _Thread_local uint64_t t_s203_s65_cold_misses = 0;
static _Thread_local uint64_t t_s203_s65_cold_miss_to_segment = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_inbox_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_alloc_slow_from_central_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_alloc_slow_from_segment_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_s65_cold_calls_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_s65_cold_hits_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_s65_cold_misses_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_s65_cold_miss_to_segment_sc[HZ3_NUM_SC];

HZ3_DTOR_STAT(g_s203_alloc_slow_calls);
HZ3_DTOR_STAT(g_s203_alloc_slow_from_inbox);
HZ3_DTOR_STAT(g_s203_alloc_slow_from_central);
HZ3_DTOR_STAT(g_s203_alloc_slow_from_segment);
HZ3_DTOR_STAT(g_s203_s206_steal_attempts);
HZ3_DTOR_STAT(g_s203_s206_steal_hits);
HZ3_DTOR_STAT(g_s203_s208_reserve_attempts);
HZ3_DTOR_STAT(g_s203_s208_reserve_hits);
HZ3_DTOR_STAT(g_s203_s208_central_miss_after_reserve);
HZ3_DTOR_STAT(g_s203_s223_want_boost_calls);
HZ3_DTOR_STAT(g_s203_s236_mini_calls);
HZ3_DTOR_STAT(g_s203_s236_mini_hit_inbox);
HZ3_DTOR_STAT(g_s203_s236_mini_hit_central);
HZ3_DTOR_STAT(g_s203_s236_mini_miss);
HZ3_DTOR_STAT(g_s203_s236e_retry_calls);
HZ3_DTOR_STAT(g_s203_s236e_retry_hits);
HZ3_DTOR_STAT(g_s203_s236e_retry_skipped_no_supply);
HZ3_DTOR_STAT(g_s203_s236f_retry_calls);
HZ3_DTOR_STAT(g_s203_s236f_retry_hits);
HZ3_DTOR_STAT(g_s203_s236f_retry_skipped_streak);
HZ3_DTOR_STAT(g_s203_s236g_hint_checks);
HZ3_DTOR_STAT(g_s203_s236g_hint_positive);
HZ3_DTOR_STAT(g_s203_s236g_hint_empty_skips);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_calls);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_hits);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_skip_no_hint);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_skip_no_backlog);
HZ3_DTOR_STAT(g_s203_s236n_bulk_calls);
HZ3_DTOR_STAT(g_s203_s236n_bulk_inbox_extra_objs);
HZ3_DTOR_STAT(g_s203_s236n_bulk_central_extra_objs);
HZ3_DTOR_STAT(g_s203_s236n_bulk_zero_extra);
HZ3_DTOR_STAT(g_s203_s65_cold_calls);
HZ3_DTOR_STAT(g_s203_s65_cold_hits);
HZ3_DTOR_STAT(g_s203_s65_cold_misses);
HZ3_DTOR_STAT(g_s203_s65_cold_miss_to_segment);
static _Atomic uint64_t g_s203_alloc_slow_from_inbox_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_alloc_slow_from_central_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_alloc_slow_from_segment_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_s65_cold_calls_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_s65_cold_hits_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_s65_cold_misses_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_s65_cold_miss_to_segment_sc[HZ3_NUM_SC];
HZ3_DTOR_ATEXIT_FLAG(g_s203_alloc);

// Extern flush functions from other modules
void hz3_s203_remote_flush_tls(void);
void hz3_s203_remote_register_once(void);
void hz3_s203_inbox_flush_tls(void);
#if HZ3_S204_LARSON_DIAG
static void hz3_s204_flush_tls(void);
#endif

// Flush TLS to Global (called from thread dtor)
void hz3_s203_flush_tls(void) {
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_calls, t_s203_alloc_slow_calls);
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_from_inbox, t_s203_alloc_slow_from_inbox);
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_from_central, t_s203_alloc_slow_from_central);
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_from_segment, t_s203_alloc_slow_from_segment);
    HZ3_DTOR_STAT_ADD(g_s203_s206_steal_attempts, t_s203_s206_steal_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s206_steal_hits, t_s203_s206_steal_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s208_reserve_attempts, t_s203_s208_reserve_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s208_reserve_hits, t_s203_s208_reserve_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s208_central_miss_after_reserve,
                      t_s203_s208_central_miss_after_reserve);
    HZ3_DTOR_STAT_ADD(g_s203_s223_want_boost_calls,
                      t_s203_s223_want_boost_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_calls,
                      t_s203_s236_mini_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_hit_inbox,
                      t_s203_s236_mini_hit_inbox);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_hit_central,
                      t_s203_s236_mini_hit_central);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_miss,
                      t_s203_s236_mini_miss);
    HZ3_DTOR_STAT_ADD(g_s203_s236e_retry_calls,
                      t_s203_s236e_retry_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236e_retry_hits,
                      t_s203_s236e_retry_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236e_retry_skipped_no_supply,
                      t_s203_s236e_retry_skipped_no_supply);
    HZ3_DTOR_STAT_ADD(g_s203_s236f_retry_calls,
                      t_s203_s236f_retry_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236f_retry_hits,
                      t_s203_s236f_retry_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236f_retry_skipped_streak,
                      t_s203_s236f_retry_skipped_streak);
    HZ3_DTOR_STAT_ADD(g_s203_s236g_hint_checks,
                      t_s203_s236g_hint_checks);
    HZ3_DTOR_STAT_ADD(g_s203_s236g_hint_positive,
                      t_s203_s236g_hint_positive);
    HZ3_DTOR_STAT_ADD(g_s203_s236g_hint_empty_skips,
                      t_s203_s236g_hint_empty_skips);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_calls,
                      t_s203_s236i_second_inbox_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_hits,
                      t_s203_s236i_second_inbox_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_skip_no_hint,
                      t_s203_s236i_second_inbox_skip_no_hint);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_skip_no_backlog,
                      t_s203_s236i_second_inbox_skip_no_backlog);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_calls,
                      t_s203_s236n_bulk_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_inbox_extra_objs,
                      t_s203_s236n_bulk_inbox_extra_objs);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_central_extra_objs,
                      t_s203_s236n_bulk_central_extra_objs);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_zero_extra,
                      t_s203_s236n_bulk_zero_extra);
    HZ3_DTOR_STAT_ADD(g_s203_s65_cold_calls,
                      t_s203_s65_cold_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s65_cold_hits,
                      t_s203_s65_cold_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s65_cold_misses,
                      t_s203_s65_cold_misses);
    HZ3_DTOR_STAT_ADD(g_s203_s65_cold_miss_to_segment,
                      t_s203_s65_cold_miss_to_segment);
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s203_alloc_slow_from_inbox_sc[sc],
                                  t_s203_alloc_slow_from_inbox_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_alloc_slow_from_central_sc[sc],
                                  t_s203_alloc_slow_from_central_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_alloc_slow_from_segment_sc[sc],
                                  t_s203_alloc_slow_from_segment_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_s65_cold_calls_sc[sc],
                                  t_s203_s65_cold_calls_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_s65_cold_hits_sc[sc],
                                  t_s203_s65_cold_hits_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_s65_cold_misses_sc[sc],
                                  t_s203_s65_cold_misses_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_s65_cold_miss_to_segment_sc[sc],
                                  t_s203_s65_cold_miss_to_segment_sc[sc],
                                  memory_order_relaxed);
    }
    
    // Flush other modules too
    hz3_s203_remote_flush_tls();
    hz3_s203_inbox_flush_tls();
#if HZ3_S204_LARSON_DIAG
    hz3_s204_flush_tls();
#endif
}

static void hz3_s203_alloc_atexit_dump(void) {
    uint64_t inbox_band[4] = {0, 0, 0, 0};
    uint64_t central_band[4] = {0, 0, 0, 0};
    uint64_t segment_band[4] = {0, 0, 0, 0};
    const char* band_name[4] = {"le32k", "32k_64k", "64k_128k", "gt128k"};

    fprintf(stderr, "[HZ3_S203] alloc_slow_calls=%u "
            "from_inbox=%u from_central=%u from_segment=%u\n",
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_from_inbox),
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_from_central),
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_from_segment));
    if (HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_attempts) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_hits) > 0) {
        fprintf(stderr, "[HZ3_S203_S206] steal_attempts=%u steal_hits=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_hits));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_attempts) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_hits) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s208_central_miss_after_reserve) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S208] reserve_attempts=%u reserve_hits=%u "
                "central_miss_after_reserve=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s208_central_miss_after_reserve));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s223_want_boost_calls) > 0) {
        fprintf(stderr, "[HZ3_S203_S223] want_boost_calls=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s223_want_boost_calls));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236B] mini_calls=%u hit_inbox=%u hit_central=%u miss=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_hit_inbox),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_hit_central),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_miss));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236E] retry_calls=%u retry_hits=%u retry_skipped_no_supply=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_skipped_no_supply));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_calls) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_skipped_streak) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236F] retry_calls=%u retry_hits=%u retry_skipped_streak=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_skipped_streak));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_checks) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236G] hint_checks=%u hint_positive=%u hint_empty_skips=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_checks),
                HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_positive),
                HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_empty_skips));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_calls) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_hint) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_backlog) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236I] second_inbox_calls=%u second_inbox_hits=%u "
                "skip_no_hint=%u skip_no_backlog=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_hint),
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_backlog));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236N] bulk_calls=%u inbox_extra_objs=%u "
                "central_extra_objs=%u zero_extra=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_inbox_extra_objs),
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_central_extra_objs),
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_zero_extra));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s65_cold_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S65_COLD] calls=%u hits=%u misses=%u miss_to_segment=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s65_cold_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s65_cold_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s65_cold_misses),
                HZ3_DTOR_STAT_LOAD(g_s203_s65_cold_miss_to_segment));
    }
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t from_inbox =
            atomic_load_explicit(&g_s203_alloc_slow_from_inbox_sc[sc], memory_order_relaxed);
        uint64_t from_central =
            atomic_load_explicit(&g_s203_alloc_slow_from_central_sc[sc], memory_order_relaxed);
        uint64_t from_segment =
            atomic_load_explicit(&g_s203_alloc_slow_from_segment_sc[sc], memory_order_relaxed);
        if (from_inbox == 0 && from_central == 0 && from_segment == 0) {
            continue;
        }
        size_t sz = hz3_sc_to_size((int)sc);
        int band = 3;
        if (sz <= (32u << 10)) {
            band = 0;
        } else if (sz <= (64u << 10)) {
            band = 1;
        } else if (sz <= (128u << 10)) {
            band = 2;
        }
        inbox_band[band] += from_inbox;
        central_band[band] += from_central;
        segment_band[band] += from_segment;
        fprintf(stderr,
                "[HZ3_S203_ALLOC_SC] sc=%u from_inbox=%llu from_central=%llu from_segment=%llu\n",
                sc,
                (unsigned long long)from_inbox,
                (unsigned long long)from_central,
                (unsigned long long)from_segment);
    }
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t cold_calls =
            atomic_load_explicit(&g_s203_s65_cold_calls_sc[sc], memory_order_relaxed);
        uint64_t cold_hits =
            atomic_load_explicit(&g_s203_s65_cold_hits_sc[sc], memory_order_relaxed);
        uint64_t cold_misses =
            atomic_load_explicit(&g_s203_s65_cold_misses_sc[sc], memory_order_relaxed);
        uint64_t cold_miss_to_segment =
            atomic_load_explicit(&g_s203_s65_cold_miss_to_segment_sc[sc], memory_order_relaxed);
        if (cold_calls == 0 &&
            cold_hits == 0 &&
            cold_misses == 0 &&
            cold_miss_to_segment == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S203_S65_COLD_SC] sc=%u calls=%llu hits=%llu misses=%llu miss_to_segment=%llu\n",
                sc,
                (unsigned long long)cold_calls,
                (unsigned long long)cold_hits,
                (unsigned long long)cold_misses,
                (unsigned long long)cold_miss_to_segment);
    }
    for (int b = 0; b < 4; b++) {
        if (inbox_band[b] == 0 && central_band[b] == 0 && segment_band[b] == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S203_ALLOC_BAND] band=%s from_inbox=%llu from_central=%llu from_segment=%llu\n",
                band_name[b],
                (unsigned long long)inbox_band[b],
                (unsigned long long)central_band[b],
                (unsigned long long)segment_band[b]);
    }
}
#define S203_ALLOC_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s203_alloc, hz3_s203_alloc_atexit_dump)
#define S203_ALLOC_INC(name) (t_s203_##name++)
#define S203_ALLOC_ADD(name, value) (t_s203_##name += (uint64_t)(value))
#define S203_ALLOC_INC_SC(src, sc) (t_s203_alloc_slow_from_##src##_sc[(sc)]++)
#define S203_ALLOC_S65_COLD_CALL(sc) do {                     \
    uint32_t s203_sc_ = (uint32_t)(sc);                       \
    t_s203_s65_cold_calls++;                                  \
    if (s203_sc_ < (uint32_t)HZ3_NUM_SC) {                    \
        t_s203_s65_cold_calls_sc[s203_sc_]++;                 \
    }                                                         \
} while (0)
#define S203_ALLOC_S65_COLD_HIT(sc) do {                      \
    uint32_t s203_sc_ = (uint32_t)(sc);                       \
    t_s203_s65_cold_hits++;                                   \
    if (s203_sc_ < (uint32_t)HZ3_NUM_SC) {                    \
        t_s203_s65_cold_hits_sc[s203_sc_]++;                  \
    }                                                         \
} while (0)
#define S203_ALLOC_S65_COLD_MISS(sc) do {                     \
    uint32_t s203_sc_ = (uint32_t)(sc);                       \
    t_s203_s65_cold_misses++;                                 \
    if (s203_sc_ < (uint32_t)HZ3_NUM_SC) {                    \
        t_s203_s65_cold_misses_sc[s203_sc_]++;                \
    }                                                         \
} while (0)
#define S203_ALLOC_S65_COLD_MISS_TO_SEGMENT(sc) do {          \
    uint32_t s203_sc_ = (uint32_t)(sc);                       \
    t_s203_s65_cold_miss_to_segment++;                        \
    if (s203_sc_ < (uint32_t)HZ3_NUM_SC) {                    \
        t_s203_s65_cold_miss_to_segment_sc[s203_sc_]++;       \
    }                                                         \
} while (0)
// Hook for thread dtor (need to find where hz3_tcache_fini is defined/used)
#else
#define S203_ALLOC_REGISTER() ((void)0)
#define S203_ALLOC_INC(name) ((void)0)
#define S203_ALLOC_ADD(name, value) ((void)0)
#define S203_ALLOC_INC_SC(src, sc) ((void)0)
#define S203_ALLOC_S65_COLD_CALL(sc) ((void)0)
#define S203_ALLOC_S65_COLD_HIT(sc) ((void)0)
#define S203_ALLOC_S65_COLD_MISS(sc) ((void)0)
#define S203_ALLOC_S65_COLD_MISS_TO_SEGMENT(sc) ((void)0)
void hz3_s203_flush_tls(void) {}
#endif
