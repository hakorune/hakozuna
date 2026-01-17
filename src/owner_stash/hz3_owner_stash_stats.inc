// =============================================================================
// S67: Spill stats (pop_batch only)
// =============================================================================
#if HZ3_S67_STATS
HZ3_DTOR_STATS_BEGIN(S67);
HZ3_DTOR_STAT(g_s67_pop_calls);
HZ3_DTOR_STAT(g_s67_spill_array_hit_calls);
HZ3_DTOR_STAT(g_s67_spill_array_take_total);
HZ3_DTOR_STAT(g_s67_spill_overflow_hit_calls);
HZ3_DTOR_STAT(g_s67_spill_overflow_take_total);
HZ3_DTOR_STAT(g_s67_bounded_drain_calls);
HZ3_DTOR_STAT(g_s67_bounded_drain_detach_total);
HZ3_DTOR_STAT(g_s67_bounded_drain_cas_retry_total);
HZ3_DTOR_STAT(g_s67_unexpected_remainder);
HZ3_DTOR_ATEXIT_FLAG(g_s67);

static void hz3_s67_atexit_dump(void) {
    uint32_t pop_calls = HZ3_DTOR_STAT_LOAD(g_s67_pop_calls);
    uint32_t spill_array_hit = HZ3_DTOR_STAT_LOAD(g_s67_spill_array_hit_calls);
    uint32_t spill_array_take = HZ3_DTOR_STAT_LOAD(g_s67_spill_array_take_total);
    uint32_t spill_overflow_hit = HZ3_DTOR_STAT_LOAD(g_s67_spill_overflow_hit_calls);
    uint32_t spill_overflow_take = HZ3_DTOR_STAT_LOAD(g_s67_spill_overflow_take_total);
    uint32_t drain_calls = HZ3_DTOR_STAT_LOAD(g_s67_bounded_drain_calls);
    uint32_t drain_detach = HZ3_DTOR_STAT_LOAD(g_s67_bounded_drain_detach_total);
    uint32_t drain_retry = HZ3_DTOR_STAT_LOAD(g_s67_bounded_drain_cas_retry_total);
    uint32_t unexpected = HZ3_DTOR_STAT_LOAD(g_s67_unexpected_remainder);

    fprintf(stderr,
            "[HZ3_S67] pop_calls=%u spill_array_hit=%u spill_array_take=%u "
            "spill_overflow_hit=%u spill_overflow_take=%u "
            "bounded_drain_calls=%u bounded_drain_detach=%u bounded_drain_cas_retry=%u "
            "unexpected_remainder=%u\n",
            pop_calls, spill_array_hit, spill_array_take,
            spill_overflow_hit, spill_overflow_take,
            drain_calls, drain_detach, drain_retry,
            unexpected);
}

#define S67_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S67_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#else
#define S67_STAT_INC(name) ((void)0)
#define S67_STAT_ADD(name, val) ((void)0)
#endif

// =============================================================================
// S112: Full drain exchange stats (pop_batch only)
// =============================================================================
#if HZ3_S112_STATS
HZ3_DTOR_STATS_BEGIN(S112);
HZ3_DTOR_STAT(g_s112_exchange_calls);
HZ3_DTOR_STAT(g_s112_exchange_empty);
HZ3_DTOR_STAT(g_s112_overflow_set_calls);
HZ3_DTOR_STAT(g_s112_overflow_old_nonnull);
HZ3_DTOR_ATEXIT_FLAG(g_s112);

static void hz3_s112_atexit_dump(void) {
    fprintf(stderr,
            "[HZ3_S112] exchange_calls=%u exchange_empty=%u overflow_set_calls=%u overflow_old_nonnull=%u\n",
            HZ3_DTOR_STAT_LOAD(g_s112_exchange_calls),
            HZ3_DTOR_STAT_LOAD(g_s112_exchange_empty),
            HZ3_DTOR_STAT_LOAD(g_s112_overflow_set_calls),
            HZ3_DTOR_STAT_LOAD(g_s112_overflow_old_nonnull));
}

#define S112_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#else
#define S112_STAT_INC(name) ((void)0)
#endif

// =============================================================================
// S94 Stats (atexit one-shot)
// =============================================================================
#if HZ3_S94_STATS
HZ3_DTOR_STATS_BEGIN(S94);
HZ3_DTOR_STAT(g_s94_pop_calls);
HZ3_DTOR_STAT(g_s94_spill_hit_calls);
HZ3_DTOR_STAT(g_s94_spill_take_total);
HZ3_DTOR_STAT(g_s94_overflow_set_calls);
HZ3_DTOR_STAT(g_s94_overflow_take_total);
HZ3_DTOR_STAT(g_s94_overflow_old_nonnull);
HZ3_DTOR_STAT(g_s94_overflow_tail_scan_calls);
HZ3_DTOR_ATEXIT_FLAG(g_s94);

static void hz3_s94_atexit_dump(void) {
    fprintf(stderr, "[HZ3_S94] SC_MAX=%u CAP=%u "
            "pop_calls=%u spill_hit_calls=%u spill_take_total=%u "
            "overflow_set_calls=%u overflow_take_total=%u "
            "overflow_old_nonnull=%u overflow_tail_scan_calls=%u\n",
            (unsigned)HZ3_S94_SPILL_SC_MAX, (unsigned)HZ3_S94_SPILL_CAP,
            HZ3_DTOR_STAT_LOAD(g_s94_pop_calls),
            HZ3_DTOR_STAT_LOAD(g_s94_spill_hit_calls),
            HZ3_DTOR_STAT_LOAD(g_s94_spill_take_total),
            HZ3_DTOR_STAT_LOAD(g_s94_overflow_set_calls),
            HZ3_DTOR_STAT_LOAD(g_s94_overflow_take_total),
            HZ3_DTOR_STAT_LOAD(g_s94_overflow_old_nonnull),
            HZ3_DTOR_STAT_LOAD(g_s94_overflow_tail_scan_calls));
}

#define S94_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S94_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S94_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s94, hz3_s94_atexit_dump)
#else
#define S94_STAT_INC(name) ((void)0)
#define S94_STAT_ADD(name, val) ((void)0)
#define S94_STAT_REGISTER() ((void)0)
#endif  // HZ3_S94_STATS

// =============================================================================
// S96: OwnerStashPush SSOT (atexit one-shot)
// =============================================================================
#if HZ3_S96_OWNER_STASH_PUSH_STATS
HZ3_DTOR_STATS_BEGIN(S96);
HZ3_DTOR_STAT(g_s96_push_calls);
HZ3_DTOR_STAT(g_s96_push_objs_total);
HZ3_DTOR_STAT(g_s96_push_n1_calls);
HZ3_DTOR_STAT(g_s96_push_n_max);
HZ3_DTOR_STAT(g_s96_push_hist_1);
HZ3_DTOR_STAT(g_s96_push_hist_2);
HZ3_DTOR_STAT(g_s96_push_hist_3_4);
HZ3_DTOR_STAT(g_s96_push_hist_5_8);
HZ3_DTOR_STAT(g_s96_push_hist_9_16);
HZ3_DTOR_STAT(g_s96_push_hist_17_32);
HZ3_DTOR_STAT(g_s96_push_hist_33p);
HZ3_DTOR_STAT(g_s96_cas_retry_total);
HZ3_DTOR_STAT(g_s96_cas_retry_max);
HZ3_DTOR_STAT(g_s96_cas_retry_gt0_calls);
HZ3_DTOR_STAT(g_s96_push_sc_lt32_objs);
HZ3_DTOR_STAT(g_s96_push_sc_ge32_objs);
HZ3_DTOR_ATEXIT_FLAG(g_s96);

static inline void hz3_s96_stat_update_max(_Atomic(uint32_t)* stat, uint32_t val) {
    uint32_t cur = atomic_load_explicit(stat, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                stat, &cur, val, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_s96_atexit_dump(void) {
    fprintf(stderr,
            "[HZ3_S96_PUSH] calls=%u objs=%u n1=%u nmax=%u "
            "cas_retry_total=%u cas_retry_max=%u cas_retry_gt0=%u "
            "sc_lt32_objs=%u sc_ge32_objs=%u "
            "hist=1:%u 2:%u 3-4:%u 5-8:%u 9-16:%u 17-32:%u 33+:%u\n",
            HZ3_DTOR_STAT_LOAD(g_s96_push_calls),
            HZ3_DTOR_STAT_LOAD(g_s96_push_objs_total),
            HZ3_DTOR_STAT_LOAD(g_s96_push_n1_calls),
            HZ3_DTOR_STAT_LOAD(g_s96_push_n_max),
            HZ3_DTOR_STAT_LOAD(g_s96_cas_retry_total),
            HZ3_DTOR_STAT_LOAD(g_s96_cas_retry_max),
            HZ3_DTOR_STAT_LOAD(g_s96_cas_retry_gt0_calls),
            HZ3_DTOR_STAT_LOAD(g_s96_push_sc_lt32_objs),
            HZ3_DTOR_STAT_LOAD(g_s96_push_sc_ge32_objs),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_1),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_2),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_3_4),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_5_8),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_9_16),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_17_32),
            HZ3_DTOR_STAT_LOAD(g_s96_push_hist_33p));
}

#define S96_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S96_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S96_STAT_MAX(name, val) hz3_s96_stat_update_max(&(name), (uint32_t)(val))
#define S96_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s96, hz3_s96_atexit_dump)
#else
#define S96_STAT_INC(name) ((void)0)
#define S96_STAT_ADD(name, val) ((void)0)
#define S96_STAT_MAX(name, val) ((void)0)
#define S96_STAT_REGISTER() ((void)0)
#endif  // HZ3_S96_OWNER_STASH_PUSH_STATS

#if HZ3_S96_OWNER_STASH_PUSH_SHOT
static int g_s96_push_shot_fired = 0;
#endif

// =============================================================================
// S44-4: MicroOptBox observation stats (atexit one-shot)
// =============================================================================
#if HZ3_S44_4_STATS
HZ3_DTOR_STATS_BEGIN(S44_4);
HZ3_DTOR_STAT(g_s44_4_pop_calls);           // pop_batch call count
HZ3_DTOR_STAT(g_s44_4_want_sum32);          // sum of want (32-bit, overflow OK for bench scale)
HZ3_DTOR_STAT(g_s44_4_want_32_count);       // count of want==32 calls
HZ3_DTOR_STAT(g_s44_4_drained_sum);         // sum of drained list lengths
HZ3_DTOR_STAT(g_s44_4_drained_max);         // max drained list length
HZ3_DTOR_STAT(g_s44_4_spill_hit_count);     // times spill returned >=1 obj
HZ3_DTOR_STAT(g_s44_4_fastpop_miss_count);  // drain entry but stash was empty (quick check passed, exchange got NULL)
HZ3_DTOR_ATEXIT_FLAG(g_s44_4);

static inline void hz3_s44_4_stat_update_max(_Atomic(uint32_t)* stat, uint32_t val) {
    uint32_t cur = atomic_load_explicit(stat, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                stat, &cur, val, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_s44_4_atexit_dump(void) {
    uint32_t pop_calls = HZ3_DTOR_STAT_LOAD(g_s44_4_pop_calls);
    uint32_t want_sum = HZ3_DTOR_STAT_LOAD(g_s44_4_want_sum32);
    uint32_t want_32 = HZ3_DTOR_STAT_LOAD(g_s44_4_want_32_count);
    uint32_t drained_sum = HZ3_DTOR_STAT_LOAD(g_s44_4_drained_sum);
    uint32_t drained_max = HZ3_DTOR_STAT_LOAD(g_s44_4_drained_max);
    uint32_t spill_hit = HZ3_DTOR_STAT_LOAD(g_s44_4_spill_hit_count);
    uint32_t fastpop_miss = HZ3_DTOR_STAT_LOAD(g_s44_4_fastpop_miss_count);

    // Compute averages (guard div-by-zero)
    double want_avg = (pop_calls > 0) ? (double)want_sum / pop_calls : 0.0;
    double want_32_pct = (pop_calls > 0) ? 100.0 * want_32 / pop_calls : 0.0;
    // drained_avg: only count calls that actually drained (pop_calls - spill_hit - fastpop_miss is approximate)
    uint32_t drain_calls = (pop_calls > spill_hit) ? (pop_calls - spill_hit) : 0;
    double drained_avg = (drain_calls > 0) ? (double)drained_sum / drain_calls : 0.0;
    double spill_hit_pct = (pop_calls > 0) ? 100.0 * spill_hit / pop_calls : 0.0;
    double fastpop_miss_pct = (pop_calls > 0) ? 100.0 * fastpop_miss / pop_calls : 0.0;

    fprintf(stderr,
            "[HZ3_S44_4] pop_calls=%u want_avg=%.1f want_32_pct=%.1f "
            "drained_avg=%.1f drained_max=%u spill_hit_pct=%.1f fastpop_miss_pct=%.1f\n",
            pop_calls, want_avg, want_32_pct,
            drained_avg, drained_max, spill_hit_pct, fastpop_miss_pct);
}

#define S44_4_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S44_4_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S44_4_STAT_MAX(name, val) hz3_s44_4_stat_update_max(&(name), (uint32_t)(val))
#define S44_4_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s44_4, hz3_s44_4_atexit_dump)
#else
#define S44_4_STAT_INC(name) ((void)0)
#define S44_4_STAT_ADD(name, val) ((void)0)
#define S44_4_STAT_MAX(name, val) ((void)0)
#define S44_4_STAT_REGISTER() ((void)0)
#endif  // HZ3_S44_4_STATS

// =============================================================================
// S121: Page-Local Remote stats (atexit one-shot)
// =============================================================================
#if HZ3_S121_STATS
HZ3_DTOR_STATS_BEGIN(S121);
HZ3_DTOR_STAT(g_s121_stash_push_objs);          // total objects pushed via push_list
HZ3_DTOR_STAT(g_s121_stash_pop_calls);          // pop_batch call count
HZ3_DTOR_STAT(g_s121_stash_pop_objs);           // total objects returned by pop_batch
HZ3_DTOR_STAT(g_s121_pageq_enq);                // page enqueue count
HZ3_DTOR_STAT(g_s121_pageq_deq);                // page dequeue count
HZ3_DTOR_STAT(g_s121_page_drain_objs_total);    // total objects drained from pages
HZ3_DTOR_STAT(g_s121_page_drain_objs_max);      // max objects drained from a single page
HZ3_DTOR_STAT(g_s121_page_state_0to1);          // state transitions 0→1 (first remote)
HZ3_DTOR_STAT(g_s121_page_state_1to0);          // state transitions 1→0 (drain complete)
HZ3_DTOR_STAT(g_s121_page_requeue);             // page re-enqueued due to concurrent push
HZ3_DTOR_STAT(g_s121_push_old_null);            // S121-B: push to empty page (triggers enqueue)
HZ3_DTOR_STAT(g_s121_a2_force_enq);             // S121-A2: forced enqueue count
HZ3_DTOR_STAT(g_s121_a2_scan_pages);            // S121-A2: pages scanned in fallback
// Drain level (local to this file)
HZ3_DTOR_STAT(g_s121_drain_empty_calls);        // PageQ empty drain calls
HZ3_DTOR_STAT(g_s121_drain_success_calls);      // PageQ successful drain calls
// Ring send level (defined in hz3_tcache_remote.c, extern here)
extern _Atomic(uint32_t) g_s121_ring_send_success;
extern _Atomic(uint32_t) g_s121_ring_overflow_detected;
extern _Atomic(uint32_t) g_s121_ring_overflow_failed;
HZ3_DTOR_ATEXIT_FLAG(g_s121);

static inline void hz3_s121_stat_update_max(_Atomic(uint32_t)* stat, uint32_t val) {
    uint32_t cur = atomic_load_explicit(stat, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                stat, &cur, val, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_s121_atexit_dump(void) {
    uint32_t push_objs = HZ3_DTOR_STAT_LOAD(g_s121_stash_push_objs);
    uint32_t pop_calls = HZ3_DTOR_STAT_LOAD(g_s121_stash_pop_calls);
    uint32_t pop_objs = HZ3_DTOR_STAT_LOAD(g_s121_stash_pop_objs);
    uint32_t pageq_enq = HZ3_DTOR_STAT_LOAD(g_s121_pageq_enq);
    uint32_t pageq_deq = HZ3_DTOR_STAT_LOAD(g_s121_pageq_deq);
    uint32_t drain_total = HZ3_DTOR_STAT_LOAD(g_s121_page_drain_objs_total);
    uint32_t drain_max = HZ3_DTOR_STAT_LOAD(g_s121_page_drain_objs_max);
    uint32_t state_0to1 = HZ3_DTOR_STAT_LOAD(g_s121_page_state_0to1);
    uint32_t state_1to0 = HZ3_DTOR_STAT_LOAD(g_s121_page_state_1to0);
    uint32_t requeue = HZ3_DTOR_STAT_LOAD(g_s121_page_requeue);
    uint32_t push_old_null = HZ3_DTOR_STAT_LOAD(g_s121_push_old_null);
    uint32_t a2_force = HZ3_DTOR_STAT_LOAD(g_s121_a2_force_enq);
    uint32_t a2_scan = HZ3_DTOR_STAT_LOAD(g_s121_a2_scan_pages);

    // Ring send level
    uint32_t ring_send = HZ3_DTOR_STAT_LOAD(g_s121_ring_send_success);
    uint32_t ring_overflow = HZ3_DTOR_STAT_LOAD(g_s121_ring_overflow_detected);
    uint32_t ring_overflow_fail = HZ3_DTOR_STAT_LOAD(g_s121_ring_overflow_failed);

    // Drain level
    uint32_t drain_empty = HZ3_DTOR_STAT_LOAD(g_s121_drain_empty_calls);
    uint32_t drain_success = HZ3_DTOR_STAT_LOAD(g_s121_drain_success_calls);

    double objs_per_page = (pageq_deq > 0) ? (double)drain_total / pageq_deq : 0.0;
    double push_null_ratio = (push_objs > 0) ? (double)push_old_null / push_objs : 0.0;

    // Ring metrics
    double overflow_rate = (ring_send > 0) ? 100.0 * ring_overflow / ring_send : 0.0;
    double overflow_fail_rate = (ring_overflow > 0) ?
        100.0 * ring_overflow_fail / ring_overflow : 0.0;

    // Drain metrics
    double drain_empty_rate = (drain_empty + drain_success > 0) ?
        100.0 * drain_empty / (drain_empty + drain_success) : 0.0;

    fprintf(stderr,
            "[HZ3_S121] push_objs=%u pop_calls=%u pop_objs=%u "
            "pageq_enq=%u pageq_deq=%u drain_total=%u drain_max=%u objs_per_page=%.1f "
            "state_0to1=%u state_1to0=%u requeue=%u push_old_null=%u (%.2f%%) a2_force=%u a2_scan=%u "
            "ring_send=%u ring_overflow=%u (%.1f%%) ring_overflow_fail=%u (%.1f%%) "
            "drain_empty=%u (%.1f%%)\n",
            push_objs, pop_calls, pop_objs,
            pageq_enq, pageq_deq, drain_total, drain_max, objs_per_page,
            state_0to1, state_1to0, requeue, push_old_null, push_null_ratio * 100.0,
            a2_force, a2_scan,
            ring_send, ring_overflow, overflow_rate, ring_overflow_fail, overflow_fail_rate,
            drain_empty, drain_empty_rate);
}

#define S121_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S121_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S121_STAT_MAX(name, val) hz3_s121_stat_update_max(&(name), (uint32_t)(val))
#define S121_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s121, hz3_s121_atexit_dump)
#else
#define S121_STAT_INC(name) ((void)0)
#define S121_STAT_ADD(name, val) ((void)0)
#define S121_STAT_MAX(name, val) ((void)0)
#define S121_STAT_REGISTER() ((void)0)
#endif  // HZ3_S121_STATS

