// hz4_mid_stats.inc - Mid stats box split from hz4_mid.c
#if HZ4_MID_STATS_B1
typedef struct {
    _Atomic(uint64_t) calls;
    _Atomic(uint64_t) obj_cache_hit;
    _Atomic(uint64_t) tls_hit;
    _Atomic(uint64_t) alloc_cache_hit;
    _Atomic(uint64_t) free_batch_hit;
    _Atomic(uint64_t) free_batch_refill;
    _Atomic(uint64_t) free_batch_refill_objs;
    _Atomic(uint64_t) alloc_cache_refill;
    _Atomic(uint64_t) alloc_cache_refill_objs;
    _Atomic(uint64_t) owner_fast;
    _Atomic(uint64_t) owner_drain;
    _Atomic(uint64_t) owner_drain_objs;
    _Atomic(uint64_t) lock_burst_calls;
    _Atomic(uint64_t) lock_burst_objs;
    _Atomic(uint64_t) lock_path;
    _Atomic(uint64_t) st_local_stack_hit;
    _Atomic(uint64_t) owner_local_stack_hit;
    _Atomic(uint64_t) owner_local_stack_stale_remote;
    _Atomic(uint64_t) owner_local_stack_stale_unowned;
    _Atomic(uint64_t) owner_local_stack_sc_skip;
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
    _Atomic(uint64_t) owner_local_stack_pop_gated_remote;
#endif
    _Atomic(uint64_t) bin_hit;
    _Atomic(uint64_t) bin_miss;
    _Atomic(uint64_t) page_create;
    _Atomic(uint64_t) page_create_suppress_retry;
    _Atomic(uint64_t) page_create_suppress_hit;
#if HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
    _Atomic(uint64_t) page_create_outlock_calls;
    _Atomic(uint64_t) page_create_outlock_race_hit;
    _Atomic(uint64_t) page_create_outlock_created;
    _Atomic(uint64_t) page_create_outlock_ns_sum;
#endif
#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
    _Atomic(uint64_t) prefetched_hint_probe;
    _Atomic(uint64_t) prefetched_hint_hit;
    _Atomic(uint64_t) prefetched_hint_miss;
#endif
} hz4_mid_stats_malloc_group_t;

typedef struct {
    _Atomic(uint64_t) calls;
    _Atomic(uint64_t) obj_cache_fast;
    _Atomic(uint64_t) active_owner_hit;
    _Atomic(uint64_t) active_owner_miss;
    _Atomic(uint64_t) tls_fast;
    _Atomic(uint64_t) owner_local;
    _Atomic(uint64_t) owner_remote;
    _Atomic(uint64_t) locked;
    _Atomic(uint64_t) st_local_stack_hit;
    _Atomic(uint64_t) st_local_stack_overflow;
    _Atomic(uint64_t) owner_local_stack_hit;
    _Atomic(uint64_t) owner_local_stack_overflow;
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
    _Atomic(uint64_t) owner_local_stack_push_gated_remote;
#endif
    _Atomic(uint64_t) batch_enq;
    _Atomic(uint64_t) batch_flush;
    _Atomic(uint64_t) batch_flush_objs;
} hz4_mid_stats_free_group_t;

#if HZ4_MID_LOCK_TIME_STATS
typedef struct {
    _Atomic(uint64_t) enter;
    _Atomic(uint64_t) wait_samples;
    _Atomic(uint64_t) wait_ns_sum;
    _Atomic(uint64_t) wait_ns_max;
    _Atomic(uint64_t) contended_samples;
    _Atomic(uint64_t) hold_samples;
    _Atomic(uint64_t) hold_ns_sum;
    _Atomic(uint64_t) hold_ns_max;
    _Atomic(uint64_t) hold_binscan_steps_sum;
    _Atomic(uint64_t) hold_binscan_steps_max;
    _Atomic(uint64_t) hold_page_create_calls;
    _Atomic(uint64_t) hold_page_create_ns_sum;
    _Atomic(uint64_t) prelock_alloc_run_hit;
    _Atomic(uint64_t) prelock_alloc_run_miss;
    _Atomic(uint64_t) prelock_free_batch_refill_calls;
    _Atomic(uint64_t) prelock_free_batch_refill_objs;
    _Atomic(uint64_t) inlock_remote_drain_calls;
    _Atomic(uint64_t) inlock_remote_drain_objs;
    _Atomic(uint64_t) sc[HZ4_MID_SC_COUNT];
} hz4_mid_stats_lock_group_t;

static __thread uint32_t g_hz4_mid_lock_stats_tick;

typedef struct hz4_mid_lock_stats_ctx {
    uint64_t wait_start_ns;
    uint64_t hold_start_ns;
    uint64_t page_create_ns;
    uint32_t binscan_steps;
    uint32_t page_create_calls;
    uint8_t sampled;
} hz4_mid_lock_stats_ctx_t;
#endif

typedef struct {
#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
    _Atomic(uint64_t) resv_hit;
    _Atomic(uint64_t) resv_refill;
    _Atomic(uint64_t) resv_pages;
    _Atomic(uint64_t) resv_new_seg;
#endif
    _Atomic(uint64_t) malloc_sc[HZ4_MID_SC_COUNT];
    _Atomic(uint64_t) free_sc[HZ4_MID_SC_COUNT];
    _Atomic(int) atexit_inited;
} hz4_mid_stats_meta_group_t;

static hz4_mid_stats_malloc_group_t g_hz4_mid_stats_malloc;
static hz4_mid_stats_free_group_t g_hz4_mid_stats_free;
#if HZ4_MID_LOCK_TIME_STATS
static hz4_mid_stats_lock_group_t g_hz4_mid_stats_lock;
#endif
static hz4_mid_stats_meta_group_t g_hz4_mid_stats_meta;

// Compatibility aliases (preserve existing call-sites across split .inc files).
#define g_hz4_mid_stats_malloc_calls g_hz4_mid_stats_malloc.calls
#define g_hz4_mid_stats_malloc_obj_cache_hit g_hz4_mid_stats_malloc.obj_cache_hit
#define g_hz4_mid_stats_malloc_tls_hit g_hz4_mid_stats_malloc.tls_hit
#define g_hz4_mid_stats_malloc_alloc_cache_hit g_hz4_mid_stats_malloc.alloc_cache_hit
#define g_hz4_mid_stats_malloc_free_batch_hit g_hz4_mid_stats_malloc.free_batch_hit
#define g_hz4_mid_stats_malloc_free_batch_refill g_hz4_mid_stats_malloc.free_batch_refill
#define g_hz4_mid_stats_malloc_free_batch_refill_objs g_hz4_mid_stats_malloc.free_batch_refill_objs
#define g_hz4_mid_stats_malloc_alloc_cache_refill g_hz4_mid_stats_malloc.alloc_cache_refill
#define g_hz4_mid_stats_malloc_alloc_cache_refill_objs g_hz4_mid_stats_malloc.alloc_cache_refill_objs
#define g_hz4_mid_stats_malloc_owner_fast g_hz4_mid_stats_malloc.owner_fast
#define g_hz4_mid_stats_malloc_owner_drain g_hz4_mid_stats_malloc.owner_drain
#define g_hz4_mid_stats_malloc_owner_drain_objs g_hz4_mid_stats_malloc.owner_drain_objs
#define g_hz4_mid_stats_malloc_lock_burst_calls g_hz4_mid_stats_malloc.lock_burst_calls
#define g_hz4_mid_stats_malloc_lock_burst_objs g_hz4_mid_stats_malloc.lock_burst_objs
#define g_hz4_mid_stats_malloc_lock_path g_hz4_mid_stats_malloc.lock_path
#define g_hz4_mid_stats_malloc_st_local_stack_hit g_hz4_mid_stats_malloc.st_local_stack_hit
#define g_hz4_mid_stats_malloc_owner_local_stack_hit g_hz4_mid_stats_malloc.owner_local_stack_hit
#define g_hz4_mid_stats_malloc_owner_local_stack_stale_remote g_hz4_mid_stats_malloc.owner_local_stack_stale_remote
#define g_hz4_mid_stats_malloc_owner_local_stack_stale_unowned g_hz4_mid_stats_malloc.owner_local_stack_stale_unowned
#define g_hz4_mid_stats_malloc_owner_local_stack_sc_skip g_hz4_mid_stats_malloc.owner_local_stack_sc_skip
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
#define g_hz4_mid_stats_malloc_owner_local_stack_pop_gated_remote g_hz4_mid_stats_malloc.owner_local_stack_pop_gated_remote
#endif
#define g_hz4_mid_stats_malloc_bin_hit g_hz4_mid_stats_malloc.bin_hit
#define g_hz4_mid_stats_malloc_bin_miss g_hz4_mid_stats_malloc.bin_miss
#define g_hz4_mid_stats_malloc_page_create g_hz4_mid_stats_malloc.page_create
#define g_hz4_mid_stats_malloc_page_create_suppress_retry g_hz4_mid_stats_malloc.page_create_suppress_retry
#define g_hz4_mid_stats_malloc_page_create_suppress_hit g_hz4_mid_stats_malloc.page_create_suppress_hit
#if HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
#define g_hz4_mid_stats_malloc_page_create_outlock_calls g_hz4_mid_stats_malloc.page_create_outlock_calls
#define g_hz4_mid_stats_malloc_page_create_outlock_race_hit g_hz4_mid_stats_malloc.page_create_outlock_race_hit
#define g_hz4_mid_stats_malloc_page_create_outlock_created g_hz4_mid_stats_malloc.page_create_outlock_created
#define g_hz4_mid_stats_malloc_page_create_outlock_ns_sum g_hz4_mid_stats_malloc.page_create_outlock_ns_sum
#endif
#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
#define g_hz4_mid_stats_malloc_prefetched_hint_probe g_hz4_mid_stats_malloc.prefetched_hint_probe
#define g_hz4_mid_stats_malloc_prefetched_hint_hit g_hz4_mid_stats_malloc.prefetched_hint_hit
#define g_hz4_mid_stats_malloc_prefetched_hint_miss g_hz4_mid_stats_malloc.prefetched_hint_miss
#endif

#define g_hz4_mid_stats_free_calls g_hz4_mid_stats_free.calls
#define g_hz4_mid_stats_free_obj_cache_fast g_hz4_mid_stats_free.obj_cache_fast
#define g_hz4_mid_stats_free_active_owner_hit g_hz4_mid_stats_free.active_owner_hit
#define g_hz4_mid_stats_free_active_owner_miss g_hz4_mid_stats_free.active_owner_miss
#define g_hz4_mid_stats_free_tls_fast g_hz4_mid_stats_free.tls_fast
#define g_hz4_mid_stats_free_owner_local g_hz4_mid_stats_free.owner_local
#define g_hz4_mid_stats_free_owner_remote g_hz4_mid_stats_free.owner_remote
#define g_hz4_mid_stats_free_locked g_hz4_mid_stats_free.locked
#define g_hz4_mid_stats_free_st_local_stack_hit g_hz4_mid_stats_free.st_local_stack_hit
#define g_hz4_mid_stats_free_st_local_stack_overflow g_hz4_mid_stats_free.st_local_stack_overflow
#define g_hz4_mid_stats_free_owner_local_stack_hit g_hz4_mid_stats_free.owner_local_stack_hit
#define g_hz4_mid_stats_free_owner_local_stack_overflow g_hz4_mid_stats_free.owner_local_stack_overflow
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
#define g_hz4_mid_stats_free_owner_local_stack_push_gated_remote g_hz4_mid_stats_free.owner_local_stack_push_gated_remote
#endif
#define g_hz4_mid_stats_free_batch_enq g_hz4_mid_stats_free.batch_enq
#define g_hz4_mid_stats_free_batch_flush g_hz4_mid_stats_free.batch_flush
#define g_hz4_mid_stats_free_batch_flush_objs g_hz4_mid_stats_free.batch_flush_objs

#if HZ4_MID_LOCK_TIME_STATS
#define g_hz4_mid_stats_lock_enter g_hz4_mid_stats_lock.enter
#define g_hz4_mid_stats_lock_wait_samples g_hz4_mid_stats_lock.wait_samples
#define g_hz4_mid_stats_lock_wait_ns_sum g_hz4_mid_stats_lock.wait_ns_sum
#define g_hz4_mid_stats_lock_wait_ns_max g_hz4_mid_stats_lock.wait_ns_max
#define g_hz4_mid_stats_lock_contended_samples g_hz4_mid_stats_lock.contended_samples
#define g_hz4_mid_stats_lock_hold_samples g_hz4_mid_stats_lock.hold_samples
#define g_hz4_mid_stats_lock_hold_ns_sum g_hz4_mid_stats_lock.hold_ns_sum
#define g_hz4_mid_stats_lock_hold_ns_max g_hz4_mid_stats_lock.hold_ns_max
#define g_hz4_mid_stats_lock_hold_binscan_steps_sum g_hz4_mid_stats_lock.hold_binscan_steps_sum
#define g_hz4_mid_stats_lock_hold_binscan_steps_max g_hz4_mid_stats_lock.hold_binscan_steps_max
#define g_hz4_mid_stats_lock_hold_page_create_calls g_hz4_mid_stats_lock.hold_page_create_calls
#define g_hz4_mid_stats_lock_hold_page_create_ns_sum g_hz4_mid_stats_lock.hold_page_create_ns_sum
#define g_hz4_mid_stats_lock_prelock_alloc_run_hit g_hz4_mid_stats_lock.prelock_alloc_run_hit
#define g_hz4_mid_stats_lock_prelock_alloc_run_miss g_hz4_mid_stats_lock.prelock_alloc_run_miss
#define g_hz4_mid_stats_lock_prelock_free_batch_refill_calls g_hz4_mid_stats_lock.prelock_free_batch_refill_calls
#define g_hz4_mid_stats_lock_prelock_free_batch_refill_objs g_hz4_mid_stats_lock.prelock_free_batch_refill_objs
#define g_hz4_mid_stats_lock_inlock_remote_drain_calls g_hz4_mid_stats_lock.inlock_remote_drain_calls
#define g_hz4_mid_stats_lock_inlock_remote_drain_objs g_hz4_mid_stats_lock.inlock_remote_drain_objs
#define g_hz4_mid_stats_lock_sc g_hz4_mid_stats_lock.sc
#endif

#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
#define g_hz4_mid_stats_supply_resv_hit g_hz4_mid_stats_meta.resv_hit
#define g_hz4_mid_stats_supply_resv_refill g_hz4_mid_stats_meta.resv_refill
#define g_hz4_mid_stats_supply_resv_pages g_hz4_mid_stats_meta.resv_pages
#define g_hz4_mid_stats_supply_resv_new_seg g_hz4_mid_stats_meta.resv_new_seg
#endif
#define g_hz4_mid_stats_malloc_sc g_hz4_mid_stats_meta.malloc_sc
#define g_hz4_mid_stats_free_sc g_hz4_mid_stats_meta.free_sc
#define g_hz4_mid_stats_atexit_inited g_hz4_mid_stats_meta.atexit_inited

static void hz4_mid_stats_dump_atexit(void) {
    uint64_t malloc_calls = atomic_load_explicit(&g_hz4_mid_stats_malloc_calls, memory_order_relaxed);
    uint64_t malloc_obj_cache_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_obj_cache_hit, memory_order_relaxed);
    uint64_t malloc_tls_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_tls_hit, memory_order_relaxed);
    uint64_t malloc_alloc_cache_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_alloc_cache_hit, memory_order_relaxed);
    uint64_t malloc_free_batch_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_free_batch_hit, memory_order_relaxed);
    uint64_t malloc_free_batch_refill =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_free_batch_refill, memory_order_relaxed);
    uint64_t malloc_free_batch_refill_objs =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_free_batch_refill_objs, memory_order_relaxed);
    uint64_t malloc_alloc_cache_refill =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_alloc_cache_refill, memory_order_relaxed);
    uint64_t malloc_alloc_cache_refill_objs =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_alloc_cache_refill_objs, memory_order_relaxed);
    uint64_t malloc_owner_fast =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_fast, memory_order_relaxed);
    uint64_t malloc_owner_drain =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_drain, memory_order_relaxed);
    uint64_t malloc_owner_drain_objs =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_drain_objs, memory_order_relaxed);
    uint64_t malloc_lock_burst_calls =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_lock_burst_calls, memory_order_relaxed);
    uint64_t malloc_lock_burst_objs =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_lock_burst_objs, memory_order_relaxed);
    uint64_t malloc_lock_path =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_lock_path, memory_order_relaxed);
    uint64_t malloc_st_local_stack_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_st_local_stack_hit, memory_order_relaxed);
    uint64_t malloc_owner_local_stack_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_local_stack_hit, memory_order_relaxed);
    uint64_t malloc_owner_local_stack_stale_remote =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_local_stack_stale_remote,
                             memory_order_relaxed);
    uint64_t malloc_owner_local_stack_stale_unowned =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_local_stack_stale_unowned,
                             memory_order_relaxed);
    uint64_t malloc_owner_local_stack_sc_skip =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_local_stack_sc_skip,
                             memory_order_relaxed);
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
    uint64_t malloc_owner_local_stack_pop_gated_remote =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_owner_local_stack_pop_gated_remote,
                             memory_order_relaxed);
#endif
    uint64_t malloc_bin_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_bin_hit, memory_order_relaxed);
    uint64_t malloc_bin_miss =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_bin_miss, memory_order_relaxed);
    uint64_t malloc_page_create =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create, memory_order_relaxed);
    uint64_t malloc_page_create_suppress_retry =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create_suppress_retry, memory_order_relaxed);
    uint64_t malloc_page_create_suppress_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create_suppress_hit, memory_order_relaxed);
#if HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
    uint64_t malloc_page_create_outlock_calls =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create_outlock_calls, memory_order_relaxed);
    uint64_t malloc_page_create_outlock_race_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create_outlock_race_hit, memory_order_relaxed);
    uint64_t malloc_page_create_outlock_created =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create_outlock_created, memory_order_relaxed);
    uint64_t malloc_page_create_outlock_ns_sum =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_page_create_outlock_ns_sum, memory_order_relaxed);
#endif
#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
    uint64_t malloc_prefetched_hint_probe =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_prefetched_hint_probe, memory_order_relaxed);
    uint64_t malloc_prefetched_hint_hit =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_prefetched_hint_hit, memory_order_relaxed);
    uint64_t malloc_prefetched_hint_miss =
        atomic_load_explicit(&g_hz4_mid_stats_malloc_prefetched_hint_miss, memory_order_relaxed);
#endif
#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
    uint64_t supply_resv_hit =
        atomic_load_explicit(&g_hz4_mid_stats_supply_resv_hit, memory_order_relaxed);
    uint64_t supply_resv_refill =
        atomic_load_explicit(&g_hz4_mid_stats_supply_resv_refill, memory_order_relaxed);
    uint64_t supply_resv_pages =
        atomic_load_explicit(&g_hz4_mid_stats_supply_resv_pages, memory_order_relaxed);
    uint64_t supply_resv_new_seg =
        atomic_load_explicit(&g_hz4_mid_stats_supply_resv_new_seg, memory_order_relaxed);
#endif

    uint64_t free_calls = atomic_load_explicit(&g_hz4_mid_stats_free_calls, memory_order_relaxed);
    uint64_t free_obj_cache_fast =
        atomic_load_explicit(&g_hz4_mid_stats_free_obj_cache_fast, memory_order_relaxed);
    uint64_t free_active_owner_hit =
        atomic_load_explicit(&g_hz4_mid_stats_free_active_owner_hit, memory_order_relaxed);
    uint64_t free_active_owner_miss =
        atomic_load_explicit(&g_hz4_mid_stats_free_active_owner_miss, memory_order_relaxed);
    uint64_t free_tls_fast =
        atomic_load_explicit(&g_hz4_mid_stats_free_tls_fast, memory_order_relaxed);
    uint64_t free_owner_local =
        atomic_load_explicit(&g_hz4_mid_stats_free_owner_local, memory_order_relaxed);
    uint64_t free_owner_remote =
        atomic_load_explicit(&g_hz4_mid_stats_free_owner_remote, memory_order_relaxed);
    uint64_t free_locked = atomic_load_explicit(&g_hz4_mid_stats_free_locked, memory_order_relaxed);
    uint64_t free_st_local_stack_hit =
        atomic_load_explicit(&g_hz4_mid_stats_free_st_local_stack_hit, memory_order_relaxed);
    uint64_t free_st_local_stack_overflow =
        atomic_load_explicit(&g_hz4_mid_stats_free_st_local_stack_overflow, memory_order_relaxed);
    uint64_t free_owner_local_stack_hit =
        atomic_load_explicit(&g_hz4_mid_stats_free_owner_local_stack_hit, memory_order_relaxed);
    uint64_t free_owner_local_stack_overflow =
        atomic_load_explicit(&g_hz4_mid_stats_free_owner_local_stack_overflow, memory_order_relaxed);
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
    uint64_t free_owner_local_stack_push_gated_remote =
        atomic_load_explicit(&g_hz4_mid_stats_free_owner_local_stack_push_gated_remote,
                             memory_order_relaxed);
#endif
    uint64_t free_batch_enq =
        atomic_load_explicit(&g_hz4_mid_stats_free_batch_enq, memory_order_relaxed);
    uint64_t free_batch_flush =
        atomic_load_explicit(&g_hz4_mid_stats_free_batch_flush, memory_order_relaxed);
    uint64_t free_batch_flush_objs =
        atomic_load_explicit(&g_hz4_mid_stats_free_batch_flush_objs, memory_order_relaxed);
#if HZ4_MID_LOCK_TIME_STATS
    uint64_t lock_enter =
        atomic_load_explicit(&g_hz4_mid_stats_lock_enter, memory_order_relaxed);
    uint64_t lock_wait_samples =
        atomic_load_explicit(&g_hz4_mid_stats_lock_wait_samples, memory_order_relaxed);
    uint64_t lock_wait_ns_sum =
        atomic_load_explicit(&g_hz4_mid_stats_lock_wait_ns_sum, memory_order_relaxed);
    uint64_t lock_wait_ns_max =
        atomic_load_explicit(&g_hz4_mid_stats_lock_wait_ns_max, memory_order_relaxed);
    uint64_t lock_contended_samples =
        atomic_load_explicit(&g_hz4_mid_stats_lock_contended_samples, memory_order_relaxed);
    uint64_t lock_hold_samples =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_samples, memory_order_relaxed);
    uint64_t lock_hold_ns_sum =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_ns_sum, memory_order_relaxed);
    uint64_t lock_hold_ns_max =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_ns_max, memory_order_relaxed);
    uint64_t lock_hold_binscan_steps_sum =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_binscan_steps_sum, memory_order_relaxed);
    uint64_t lock_hold_binscan_steps_max =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_binscan_steps_max, memory_order_relaxed);
    uint64_t lock_hold_page_create_calls =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_page_create_calls, memory_order_relaxed);
    uint64_t lock_hold_page_create_ns_sum =
        atomic_load_explicit(&g_hz4_mid_stats_lock_hold_page_create_ns_sum, memory_order_relaxed);
    uint64_t lock_prelock_alloc_run_hit =
        atomic_load_explicit(&g_hz4_mid_stats_lock_prelock_alloc_run_hit, memory_order_relaxed);
    uint64_t lock_prelock_alloc_run_miss =
        atomic_load_explicit(&g_hz4_mid_stats_lock_prelock_alloc_run_miss, memory_order_relaxed);
    uint64_t lock_prelock_free_batch_refill_calls =
        atomic_load_explicit(&g_hz4_mid_stats_lock_prelock_free_batch_refill_calls, memory_order_relaxed);
    uint64_t lock_prelock_free_batch_refill_objs =
        atomic_load_explicit(&g_hz4_mid_stats_lock_prelock_free_batch_refill_objs, memory_order_relaxed);
    uint64_t lock_inlock_remote_drain_calls =
        atomic_load_explicit(&g_hz4_mid_stats_lock_inlock_remote_drain_calls, memory_order_relaxed);
    uint64_t lock_inlock_remote_drain_objs =
        atomic_load_explicit(&g_hz4_mid_stats_lock_inlock_remote_drain_objs, memory_order_relaxed);
#endif

    fprintf(stderr,
            "[HZ4_MID_STATS_B1] malloc_calls=%llu malloc_obj_cache_hit=%llu malloc_tls_hit=%llu malloc_alloc_cache_hit=%llu malloc_free_batch_hit=%llu malloc_free_batch_refill=%llu malloc_free_batch_refill_objs=%llu malloc_alloc_cache_refill=%llu malloc_alloc_cache_refill_objs=%llu malloc_owner_fast=%llu malloc_owner_drain=%llu malloc_owner_drain_objs=%llu malloc_lock_burst_calls=%llu malloc_lock_burst_objs=%llu malloc_lock_path=%llu malloc_st_local_stack_hit=%llu malloc_owner_local_stack_hit=%llu malloc_owner_local_stack_stale_remote=%llu malloc_owner_local_stack_stale_unowned=%llu malloc_owner_local_stack_sc_skip=%llu malloc_bin_hit=%llu malloc_bin_miss=%llu malloc_page_create=%llu malloc_page_create_suppress_retry=%llu malloc_page_create_suppress_hit=%llu free_calls=%llu free_obj_cache_fast=%llu free_active_owner_hit=%llu free_active_owner_miss=%llu free_tls_fast=%llu free_owner_local=%llu free_owner_remote=%llu free_locked=%llu free_st_local_stack_hit=%llu free_st_local_stack_overflow=%llu free_owner_local_stack_hit=%llu free_owner_local_stack_overflow=%llu free_batch_enq=%llu free_batch_flush=%llu free_batch_flush_objs=%llu\n",
            (unsigned long long)malloc_calls,
            (unsigned long long)malloc_obj_cache_hit,
            (unsigned long long)malloc_tls_hit,
            (unsigned long long)malloc_alloc_cache_hit,
            (unsigned long long)malloc_free_batch_hit,
            (unsigned long long)malloc_free_batch_refill,
            (unsigned long long)malloc_free_batch_refill_objs,
            (unsigned long long)malloc_alloc_cache_refill,
            (unsigned long long)malloc_alloc_cache_refill_objs,
            (unsigned long long)malloc_owner_fast,
            (unsigned long long)malloc_owner_drain,
            (unsigned long long)malloc_owner_drain_objs,
            (unsigned long long)malloc_lock_burst_calls,
            (unsigned long long)malloc_lock_burst_objs,
            (unsigned long long)malloc_lock_path,
            (unsigned long long)malloc_st_local_stack_hit,
            (unsigned long long)malloc_owner_local_stack_hit,
            (unsigned long long)malloc_owner_local_stack_stale_remote,
            (unsigned long long)malloc_owner_local_stack_stale_unowned,
            (unsigned long long)malloc_owner_local_stack_sc_skip,
            (unsigned long long)malloc_bin_hit,
            (unsigned long long)malloc_bin_miss,
            (unsigned long long)malloc_page_create,
            (unsigned long long)malloc_page_create_suppress_retry,
            (unsigned long long)malloc_page_create_suppress_hit,
            (unsigned long long)free_calls,
            (unsigned long long)free_obj_cache_fast,
            (unsigned long long)free_active_owner_hit,
            (unsigned long long)free_active_owner_miss,
            (unsigned long long)free_tls_fast,
            (unsigned long long)free_owner_local,
            (unsigned long long)free_owner_remote,
            (unsigned long long)free_locked,
            (unsigned long long)free_st_local_stack_hit,
            (unsigned long long)free_st_local_stack_overflow,
            (unsigned long long)free_owner_local_stack_hit,
            (unsigned long long)free_owner_local_stack_overflow,
            (unsigned long long)free_batch_enq,
            (unsigned long long)free_batch_flush,
            (unsigned long long)free_batch_flush_objs);

#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
    fprintf(stderr,
            "[HZ4_MID_STATS_B1] malloc_owner_local_stack_pop_gated_remote=%llu free_owner_local_stack_push_gated_remote=%llu\n",
            (unsigned long long)malloc_owner_local_stack_pop_gated_remote,
            (unsigned long long)free_owner_local_stack_push_gated_remote);
#endif

#if HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
    fprintf(stderr,
            "[HZ4_MID_STATS_B1] malloc_page_create_outlock_calls=%llu malloc_page_create_outlock_race_hit=%llu malloc_page_create_outlock_created=%llu malloc_page_create_outlock_ns_sum=%llu\n",
            (unsigned long long)malloc_page_create_outlock_calls,
            (unsigned long long)malloc_page_create_outlock_race_hit,
            (unsigned long long)malloc_page_create_outlock_created,
            (unsigned long long)malloc_page_create_outlock_ns_sum);
#endif

#if HZ4_MID_LOCK_TIME_STATS
    fprintf(stderr,
            "[HZ4_MID_LOCK_STATS] lock_enter=%llu lock_wait_samples=%llu lock_wait_ns_sum=%llu lock_wait_ns_max=%llu lock_contended_samples=%llu lock_hold_samples=%llu lock_hold_ns_sum=%llu lock_hold_ns_max=%llu hold_binscan_steps_sum=%llu hold_binscan_steps_max=%llu hold_page_create_calls=%llu hold_page_create_ns_sum=%llu prelock_alloc_run_hit=%llu prelock_alloc_run_miss=%llu prelock_free_batch_refill_calls=%llu prelock_free_batch_refill_objs=%llu inlock_remote_drain_calls=%llu inlock_remote_drain_objs=%llu\n",
            (unsigned long long)lock_enter,
            (unsigned long long)lock_wait_samples,
            (unsigned long long)lock_wait_ns_sum,
            (unsigned long long)lock_wait_ns_max,
            (unsigned long long)lock_contended_samples,
            (unsigned long long)lock_hold_samples,
            (unsigned long long)lock_hold_ns_sum,
            (unsigned long long)lock_hold_ns_max,
            (unsigned long long)lock_hold_binscan_steps_sum,
            (unsigned long long)lock_hold_binscan_steps_max,
            (unsigned long long)lock_hold_page_create_calls,
            (unsigned long long)lock_hold_page_create_ns_sum,
            (unsigned long long)lock_prelock_alloc_run_hit,
            (unsigned long long)lock_prelock_alloc_run_miss,
            (unsigned long long)lock_prelock_free_batch_refill_calls,
            (unsigned long long)lock_prelock_free_batch_refill_objs,
            (unsigned long long)lock_inlock_remote_drain_calls,
            (unsigned long long)lock_inlock_remote_drain_objs);
#endif

#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
    fprintf(stderr,
            "[HZ4_MID_STATS_B1] prefetched_hint_probe=%llu prefetched_hint_hit=%llu prefetched_hint_miss=%llu\n",
            (unsigned long long)malloc_prefetched_hint_probe,
            (unsigned long long)malloc_prefetched_hint_hit,
            (unsigned long long)malloc_prefetched_hint_miss);
#endif

#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
    fprintf(stderr,
            "[HZ4_MID_STATS_B1] supply_resv_hit=%llu supply_resv_refill=%llu supply_resv_pages=%llu supply_resv_new_seg=%llu\n",
            (unsigned long long)supply_resv_hit,
            (unsigned long long)supply_resv_refill,
            (unsigned long long)supply_resv_pages,
            (unsigned long long)supply_resv_new_seg);
#endif

#if HZ4_MID_STATS_B1_SC_HIST
    uint32_t malloc_top_sc = 0;
    uint32_t free_top_sc = 0;
    uint64_t malloc_top_calls = 0;
    uint64_t free_top_calls = 0;

    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint64_t mc = atomic_load_explicit(&g_hz4_mid_stats_malloc_sc[i], memory_order_relaxed);
        uint64_t fc = atomic_load_explicit(&g_hz4_mid_stats_free_sc[i], memory_order_relaxed);
        if (mc > malloc_top_calls) {
            malloc_top_calls = mc;
            malloc_top_sc = i;
        }
        if (fc > free_top_calls) {
            free_top_calls = fc;
            free_top_sc = i;
        }
    }

    fprintf(stderr,
            "[HZ4_MID_STATS_B1_SC] malloc_top_sc=%u malloc_top_size=%zu malloc_top_calls=%llu free_top_sc=%u free_top_size=%zu free_top_calls=%llu\n",
            malloc_top_sc,
            (size_t)(malloc_top_sc + 1u) * HZ4_MID_ALIGN,
            (unsigned long long)malloc_top_calls,
            free_top_sc,
            (size_t)(free_top_sc + 1u) * HZ4_MID_ALIGN,
            (unsigned long long)free_top_calls);

    fprintf(stderr, "[HZ4_MID_STATS_B1_HIST_M]");
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint64_t mc = atomic_load_explicit(&g_hz4_mid_stats_malloc_sc[i], memory_order_relaxed);
        if (mc != 0) {
            fprintf(stderr, " sc%u=%llu", i, (unsigned long long)mc);
        }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "[HZ4_MID_STATS_B1_HIST_F]");
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint64_t fc = atomic_load_explicit(&g_hz4_mid_stats_free_sc[i], memory_order_relaxed);
        if (fc != 0) {
            fprintf(stderr, " sc%u=%llu", i, (unsigned long long)fc);
        }
    }
    fprintf(stderr, "\n");
#endif

#if HZ4_MID_LOCK_TIME_STATS && HZ4_MID_LOCK_TIME_STATS_SC_HIST
    uint32_t lock_top_sc = 0;
    uint64_t lock_top_calls = 0;
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint64_t lc = atomic_load_explicit(&g_hz4_mid_stats_lock_sc[i], memory_order_relaxed);
        if (lc > lock_top_calls) {
            lock_top_calls = lc;
            lock_top_sc = i;
        }
    }
    fprintf(stderr,
            "[HZ4_MID_LOCK_STATS_SC] top_sc=%u top_size=%zu top_calls=%llu\n",
            lock_top_sc,
            (size_t)(lock_top_sc + 1u) * HZ4_MID_ALIGN,
            (unsigned long long)lock_top_calls);
    fprintf(stderr, "[HZ4_MID_LOCK_STATS_HIST]");
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint64_t lc = atomic_load_explicit(&g_hz4_mid_stats_lock_sc[i], memory_order_relaxed);
        if (lc != 0) {
            fprintf(stderr, " sc%u=%llu", i, (unsigned long long)lc);
        }
    }
    fprintf(stderr, "\n");
#endif

    fflush(stderr);
}

static inline void hz4_mid_stats_init_once(void) {
    if (atomic_load_explicit(&g_hz4_mid_stats_atexit_inited, memory_order_relaxed) != 0) {
        return;
    }
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&g_hz4_mid_stats_atexit_inited, &expected, 1,
                                                memory_order_relaxed, memory_order_relaxed)) {
        atexit(hz4_mid_stats_dump_atexit);
    }
}

static inline void hz4_mid_stats_inc(_Atomic(uint64_t)* counter) {
    hz4_mid_stats_init_once();
    atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
}

static inline void hz4_mid_stats_add(_Atomic(uint64_t)* counter, uint64_t n) {
    hz4_mid_stats_init_once();
    atomic_fetch_add_explicit(counter, n, memory_order_relaxed);
}

static inline void hz4_mid_stats_inc_sc(_Atomic(uint64_t)* counters, uint16_t sc) {
    hz4_mid_stats_init_once();
    if (sc < HZ4_MID_SC_COUNT) {
        atomic_fetch_add_explicit(&counters[sc], 1, memory_order_relaxed);
    }
}

#if HZ4_MID_LOCK_TIME_STATS
static inline uint64_t hz4_mid_lock_stats_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static inline void hz4_mid_stats_update_max(_Atomic(uint64_t)* counter, uint64_t v) {
    uint64_t cur = atomic_load_explicit(counter, memory_order_relaxed);
    while (v > cur) {
        if (atomic_compare_exchange_weak_explicit(counter, &cur, v, memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
    }
}

static inline int hz4_mid_lock_stats_should_sample(void) {
#if HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT > 0
    uint32_t tick = ++g_hz4_mid_lock_stats_tick;
    return (tick & ((1u << HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT) - 1u)) == 0u;
#else
    ++g_hz4_mid_lock_stats_tick;
    return 1;
#endif
}

static inline void hz4_mid_lock_stats_enter(hz4_mid_lock_stats_ctx_t* ctx, uint16_t sc) {
    ctx->sampled = (uint8_t)hz4_mid_lock_stats_should_sample();
    ctx->wait_start_ns = 0;
    ctx->hold_start_ns = 0;
    ctx->page_create_ns = 0;
    ctx->binscan_steps = 0;
    ctx->page_create_calls = 0;

    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_enter);
    hz4_mid_stats_inc_sc(g_hz4_mid_stats_lock_sc, sc);

    if (ctx->sampled) {
        hz4_mid_stats_inc(&g_hz4_mid_stats_lock_wait_samples);
        ctx->wait_start_ns = hz4_mid_lock_stats_now_ns();
    }
}

static inline void hz4_mid_lock_stats_on_reacquire_wait_begin(hz4_mid_lock_stats_ctx_t* ctx) {
    if (!ctx->sampled) {
        return;
    }
    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_wait_samples);
    ctx->wait_start_ns = hz4_mid_lock_stats_now_ns();
}

static inline void hz4_mid_lock_stats_on_acquired(hz4_mid_lock_stats_ctx_t* ctx) {
    if (!ctx->sampled) {
        return;
    }
    uint64_t now_ns = hz4_mid_lock_stats_now_ns();
    uint64_t wait_ns = now_ns - ctx->wait_start_ns;
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_wait_ns_sum, wait_ns);
    hz4_mid_stats_update_max(&g_hz4_mid_stats_lock_wait_ns_max, wait_ns);
    if (wait_ns >= (uint64_t)HZ4_MID_LOCK_TIME_STATS_CONTENDED_NS) {
        hz4_mid_stats_inc(&g_hz4_mid_stats_lock_contended_samples);
    }
    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_hold_samples);
    ctx->hold_start_ns = now_ns;
}

static inline void hz4_mid_lock_stats_note_binscan(hz4_mid_lock_stats_ctx_t* ctx, uint32_t steps) {
    if (ctx->sampled) {
        ctx->binscan_steps += steps;
    }
}

static inline void hz4_mid_lock_stats_note_page_create(hz4_mid_lock_stats_ctx_t* ctx,
                                                       uint64_t page_create_ns) {
    if (ctx->sampled) {
        ctx->page_create_calls++;
        ctx->page_create_ns += page_create_ns;
    }
}

static inline void hz4_mid_lock_stats_on_release(hz4_mid_lock_stats_ctx_t* ctx) {
    if (!ctx->sampled) {
        return;
    }
    uint64_t hold_ns = hz4_mid_lock_stats_now_ns() - ctx->hold_start_ns;
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_hold_ns_sum, hold_ns);
    hz4_mid_stats_update_max(&g_hz4_mid_stats_lock_hold_ns_max, hold_ns);
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_hold_binscan_steps_sum, ctx->binscan_steps);
    hz4_mid_stats_update_max(&g_hz4_mid_stats_lock_hold_binscan_steps_max, ctx->binscan_steps);
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_hold_page_create_calls, ctx->page_create_calls);
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_hold_page_create_ns_sum, ctx->page_create_ns);
}

static inline void hz4_mid_lock_stats_note_prelock_alloc_run_hit(void) {
    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_prelock_alloc_run_hit);
}

static inline void hz4_mid_lock_stats_note_prelock_alloc_run_miss(void) {
    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_prelock_alloc_run_miss);
}

static inline void hz4_mid_lock_stats_note_prelock_free_batch_refill(uint64_t objs) {
    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_prelock_free_batch_refill_calls);
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_prelock_free_batch_refill_objs, objs);
}

static inline void hz4_mid_lock_stats_note_inlock_remote_drain(uint64_t objs) {
    hz4_mid_stats_inc(&g_hz4_mid_stats_lock_inlock_remote_drain_calls);
    hz4_mid_stats_add(&g_hz4_mid_stats_lock_inlock_remote_drain_objs, objs);
}
#endif
#endif
