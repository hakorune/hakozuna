#if HZ3_S275_LARGE_ACTIVATION_OBS
typedef struct Hz3S275ActivationTimer {
    _Atomic size_t calls;
    _Atomic size_t ns_total;
    _Atomic size_t ns_max;
} Hz3S275ActivationTimer;

static Hz3S275ActivationTimer g_s275_t_userptr_front;
static Hz3S275ActivationTimer g_s275_t_userptr_inbox;
static Hz3S275ActivationTimer g_s275_t_userptr_global;
static Hz3S275ActivationTimer g_s275_t_userptr_mmap;
static Hz3S275ActivationTimer g_s275_t_meta_front;
static Hz3S275ActivationTimer g_s275_t_meta_inbox;
static Hz3S275ActivationTimer g_s275_t_meta_global;
static Hz3S275ActivationTimer g_s275_t_meta_mmap;
static Hz3S275ActivationTimer g_s275_t_insert_direct_mapless;
static Hz3S275ActivationTimer g_s275_t_insert_active_map;
static Hz3S275ActivationTimer g_s275_t_insert_direct_shadow;
static Hz3S275ActivationTimer g_s275_t_direct_take;

static _Atomic size_t g_s275_insert_direct_mapless_hit = 0;
static _Atomic size_t g_s275_insert_direct_mapless_miss = 0;
static _Atomic size_t g_s275_direct_take_hit = 0;
static _Atomic size_t g_s275_direct_take_miss = 0;

static inline size_t hz3_s275_activation_timing_start(void) {
    return hz3_large_aligned_obs_timing_start();
}

static inline void hz3_s275_activation_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline size_t hz3_s275_activation_load(const _Atomic size_t* p) {
    return atomic_load_explicit(p, memory_order_relaxed);
}

static inline void hz3_s275_activation_note_timer(Hz3S275ActivationTimer* timer,
                                                  size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &timer->calls, &timer->ns_total, &timer->ns_max, start_ns);
}

static inline size_t hz3_s275_activation_timer_calls(Hz3S275ActivationTimer* timer) {
    return hz3_s275_activation_load(&timer->calls);
}

static inline size_t hz3_s275_activation_timer_total(Hz3S275ActivationTimer* timer) {
    return hz3_s275_activation_load(&timer->ns_total);
}

static inline size_t hz3_s275_activation_timer_max(Hz3S275ActivationTimer* timer) {
    return hz3_s275_activation_load(&timer->ns_max);
}

static inline size_t hz3_s275_activation_timer_avg(Hz3S275ActivationTimer* timer) {
    size_t calls = hz3_s275_activation_timer_calls(timer);
    size_t total = hz3_s275_activation_timer_total(timer);
    return calls ? (total / calls) : 0;
}

static inline void hz3_s275_on_userptr_front_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_front, start_ns);
}

static inline void hz3_s275_on_userptr_inbox_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_inbox, start_ns);
}

static inline void hz3_s275_on_userptr_global_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_global, start_ns);
}

static inline void hz3_s275_on_userptr_mmap_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_mmap, start_ns);
}

static inline void hz3_s275_on_meta_front_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_front, start_ns);
}

static inline void hz3_s275_on_meta_inbox_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_inbox, start_ns);
}

static inline void hz3_s275_on_meta_global_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_global, start_ns);
}

static inline void hz3_s275_on_meta_mmap_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_mmap, start_ns);
}

static inline void hz3_s275_on_insert_direct_mapless_elapsed(size_t start_ns,
                                                             int hit) {
    hz3_s275_activation_inc(hit ? &g_s275_insert_direct_mapless_hit :
                                  &g_s275_insert_direct_mapless_miss);
    hz3_s275_activation_note_timer(&g_s275_t_insert_direct_mapless, start_ns);
}

static inline void hz3_s275_on_insert_active_map_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_insert_active_map, start_ns);
}

static inline void hz3_s275_on_insert_direct_shadow_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_insert_direct_shadow, start_ns);
}

static inline void hz3_s275_on_direct_take_elapsed(size_t start_ns, int hit) {
    hz3_s275_activation_inc(hit ? &g_s275_direct_take_hit : &g_s275_direct_take_miss);
    hz3_s275_activation_note_timer(&g_s275_t_direct_take, start_ns);
}

static void hz3_s275_activation_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S275_ACTIVATION] "
            "userptr_front_calls=%zu userptr_front_ns_avg=%zu userptr_front_ns_max=%zu "
            "userptr_inbox_calls=%zu userptr_inbox_ns_avg=%zu userptr_inbox_ns_max=%zu "
            "userptr_global_calls=%zu userptr_global_ns_avg=%zu userptr_global_ns_max=%zu "
            "userptr_mmap_calls=%zu userptr_mmap_ns_avg=%zu userptr_mmap_ns_max=%zu "
            "meta_front_calls=%zu meta_front_ns_avg=%zu meta_front_ns_max=%zu "
            "meta_inbox_calls=%zu meta_inbox_ns_avg=%zu meta_inbox_ns_max=%zu "
            "meta_global_calls=%zu meta_global_ns_avg=%zu meta_global_ns_max=%zu "
            "meta_mmap_calls=%zu meta_mmap_ns_avg=%zu meta_mmap_ns_max=%zu\n",
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_front),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_front),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_front),
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_inbox),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_inbox),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_inbox),
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_global),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_global),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_global),
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_mmap),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_mmap),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_mmap),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_front),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_front),
            hz3_s275_activation_timer_max(&g_s275_t_meta_front),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_inbox),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_inbox),
            hz3_s275_activation_timer_max(&g_s275_t_meta_inbox),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_global),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_global),
            hz3_s275_activation_timer_max(&g_s275_t_meta_global),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_mmap),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_mmap),
            hz3_s275_activation_timer_max(&g_s275_t_meta_mmap));
    fprintf(stderr,
            "[HZ3_S275_DIRECT_MAP] "
            "insert_direct_mapless_calls=%zu insert_direct_mapless_hit=%zu "
            "insert_direct_mapless_miss=%zu insert_direct_mapless_ns_avg=%zu "
            "insert_direct_mapless_ns_max=%zu insert_active_map_calls=%zu "
            "insert_active_map_ns_avg=%zu insert_active_map_ns_max=%zu "
            "insert_direct_shadow_calls=%zu insert_direct_shadow_ns_avg=%zu "
            "insert_direct_shadow_ns_max=%zu direct_take_calls=%zu "
            "direct_take_hit=%zu direct_take_miss=%zu direct_take_ns_avg=%zu "
            "direct_take_ns_max=%zu\n",
            hz3_s275_activation_timer_calls(&g_s275_t_insert_direct_mapless),
            hz3_s275_activation_load(&g_s275_insert_direct_mapless_hit),
            hz3_s275_activation_load(&g_s275_insert_direct_mapless_miss),
            hz3_s275_activation_timer_avg(&g_s275_t_insert_direct_mapless),
            hz3_s275_activation_timer_max(&g_s275_t_insert_direct_mapless),
            hz3_s275_activation_timer_calls(&g_s275_t_insert_active_map),
            hz3_s275_activation_timer_avg(&g_s275_t_insert_active_map),
            hz3_s275_activation_timer_max(&g_s275_t_insert_active_map),
            hz3_s275_activation_timer_calls(&g_s275_t_insert_direct_shadow),
            hz3_s275_activation_timer_avg(&g_s275_t_insert_direct_shadow),
            hz3_s275_activation_timer_max(&g_s275_t_insert_direct_shadow),
            hz3_s275_activation_timer_calls(&g_s275_t_direct_take),
            hz3_s275_activation_load(&g_s275_direct_take_hit),
            hz3_s275_activation_load(&g_s275_direct_take_miss),
            hz3_s275_activation_timer_avg(&g_s275_t_direct_take),
            hz3_s275_activation_timer_max(&g_s275_t_direct_take));
}
#else
static inline size_t hz3_s275_activation_timing_start(void) { return 0; }
static inline void hz3_s275_on_userptr_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_mapless_elapsed(size_t start_ns,
                                                             int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s275_on_insert_active_map_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_shadow_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_direct_take_elapsed(size_t start_ns, int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s275_activation_dump_final(void) {}
#endif

