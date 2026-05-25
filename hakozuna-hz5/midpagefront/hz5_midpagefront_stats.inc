#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_STATS
static _Atomic uint64_t g_hz5_midpagefront_stat_refill;
static _Atomic uint64_t g_hz5_midpagefront_stat_refill_partial_hit;
static _Atomic uint64_t g_hz5_midpagefront_stat_refill_remote_hit;
static _Atomic uint64_t g_hz5_midpagefront_stat_refill_new_page;
static _Atomic uint64_t g_hz5_midpagefront_stat_partial_push;
static _Atomic uint64_t g_hz5_midpagefront_stat_remote_drained;
static _Atomic uint64_t g_hz5_midpagefront_stat_ptrcache_hit;
static _Atomic uint64_t g_hz5_midpagefront_stat_ptrcache_push;
static _Atomic uint64_t g_hz5_midpagefront_stat_ptrcache_full;

__attribute__((destructor)) static void hz5_midpagefront_stats_dump(void) {
  fprintf(stderr,
          "[HZ5_MIDPAGEFRONT_NODELESS_STATS]"
          " refill=%llu"
          " refill_partial_hit=%llu"
          " refill_remote_hit=%llu"
          " refill_new_page=%llu"
          " partial_push=%llu"
          " remote_drained=%llu"
          " ptrcache_hit=%llu"
          " ptrcache_push=%llu"
          " ptrcache_full=%llu\n",
          (unsigned long long)atomic_load(&g_hz5_midpagefront_stat_refill),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_refill_partial_hit),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_refill_remote_hit),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_refill_new_page),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_partial_push),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_remote_drained),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_ptrcache_hit),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_ptrcache_push),
          (unsigned long long)atomic_load(
              &g_hz5_midpagefront_stat_ptrcache_full));
}

static void hz5_midpagefront_stat_inc(_Atomic uint64_t* counter) {
  atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
}
#else
#define hz5_midpagefront_stat_inc(counter) ((void)0)
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_STATS
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_alloc_call[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_mag_hit[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_call[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_remote_hit[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_local_pop[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_refill_new_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_cache_slot[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_mag_push[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_midpagefront_m4_stat_mag_full[HZ5_MIDPAGEFRONT_CLASS_COUNT];

static void hz5_midpagefront_m4_stat_inc(_Atomic uint64_t* counter) {
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

__attribute__((destructor)) static void hz5_midpagefront_m4_stats_dump(void) {
  for (uint32_t ci = 0; ci < HZ5_MIDPAGEFRONT_CLASS_COUNT; ++ci) {
    fprintf(stderr,
            "[HZ5_MIDPAGEFRONT_M4_STATS] class=%u bytes=%u"
            " alloc_call=%llu mag_hit=%llu refill_call=%llu"
            " refill_remote_hit=%llu refill_local_pop=%llu"
            " refill_new_page=%llu cache_slot=%llu mag_push=%llu"
            " mag_full=%llu\n",
            ci,
            g_hz5_midpagefront_classes[ci],
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_alloc_call[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_mag_hit[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_call[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_remote_hit[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_local_pop[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_refill_new_page[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_cache_slot[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_mag_push[ci],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_midpagefront_m4_stat_mag_full[ci],
                memory_order_relaxed));
  }
}

#define hz5_midpagefront_m4_stat_inc_class(counter, ci) \
  do {                                                  \
    if ((ci) < HZ5_MIDPAGEFRONT_CLASS_COUNT) {          \
      hz5_midpagefront_m4_stat_inc(&(counter)[(ci)]);   \
    }                                                   \
  } while (0)
#else
#define hz5_midpagefront_m4_stat_inc_class(counter, ci) ((void)0)
#endif
