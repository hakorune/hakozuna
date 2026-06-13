#if HZ3_S256_WIDE_RSS_OBS
static _Atomic int g_hz3_s256_atexit_registered = 0;

static uint32_t hz3_s256_arena_used_slots_snapshot(void) {
#if HZ3_S47_SEGMENT_QUARANTINE
    return atomic_load_explicit(&g_hz3_arena_used_slots, memory_order_relaxed);
#else
    uint32_t used = 0;
    _Atomic uint8_t* slots = g_hz3_arena.used;
    if (!slots) {
        return 0;
    }
    for (uint32_t i = 0; i < g_hz3_arena.slots; i++) {
        used += (uint32_t)(atomic_load_explicit(&slots[i], memory_order_relaxed) != 0);
    }
    return used;
#endif
}

static void hz3_s256_wide_rss_dump(void) {
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    void* end = atomic_load_explicit(&g_hz3_arena_end, memory_order_relaxed);
    uint32_t total_slots = g_hz3_arena.slots;
    uint32_t used_slots = hz3_s256_arena_used_slots_snapshot();
    if (used_slots > total_slots) {
        used_slots = total_slots;
    }

    uint32_t free_slots = total_slots - used_slots;
    uint64_t arena_size = (uint64_t)g_hz3_arena.size;
#if defined(_WIN32)
    uint64_t arena_raw_reserved = (uint64_t)g_hz3_arena.size + (uint64_t)HZ3_SEG_SIZE;
#else
    uint64_t arena_raw_reserved = (uint64_t)g_hz3_arena.size;
#endif
    uint64_t seg_size = (uint64_t)HZ3_SEG_SIZE;
    uint64_t used_bytes = (uint64_t)used_slots * seg_size;
    uint64_t free_bytes = (uint64_t)free_slots * seg_size;

#if HZ3_SMALL_V2_PTAG_ENABLE
    uint64_t ptag16_bytes = (uint64_t)HZ3_ARENA_MAX_PAGES * (uint64_t)sizeof(_Atomic(uint16_t));
    int ptag16_enabled = 1;
    int ptag16_allocated = g_hz3_page_tag != NULL;
#else
    uint64_t ptag16_bytes = 0;
    int ptag16_enabled = 0;
    int ptag16_allocated = 0;
#endif

#if HZ3_PTAG_DSTBIN_ENABLE
    uint64_t ptag32_bytes = (uint64_t)HZ3_ARENA_MAX_PAGES * (uint64_t)sizeof(_Atomic(uint32_t));
    int ptag32_enabled = 1;
    int ptag32_allocated = g_hz3_page_tag32 != NULL;
#else
    uint64_t ptag32_bytes = 0;
    int ptag32_enabled = 0;
    int ptag32_allocated = 0;
#endif

    fprintf(stderr,
            "[HZ3_S256_ARENA] base=%p end=%p arena_size_bytes=%llu "
            "arena_raw_reserved_bytes=%llu seg_size_bytes=%llu total_slots=%u "
            "used_slots=%u free_slots=%u used_bytes=%llu free_bytes=%llu "
            "alloc_cursor=%u\n",
            base,
            end,
            (unsigned long long)arena_size,
            (unsigned long long)arena_raw_reserved,
            (unsigned long long)seg_size,
            (unsigned)total_slots,
            (unsigned)used_slots,
            (unsigned)free_slots,
            (unsigned long long)used_bytes,
            (unsigned long long)free_bytes,
            (unsigned)g_hz3_arena.alloc_cursor);

    fprintf(stderr,
            "[HZ3_S256_PTAG] arena_max_pages=%llu ptag16_enabled=%d "
            "ptag16_allocated=%d ptag16_bytes=%llu ptag32_enabled=%d "
            "ptag32_allocated=%d ptag32_bytes=%llu\n",
            (unsigned long long)HZ3_ARENA_MAX_PAGES,
            ptag16_enabled,
            ptag16_allocated,
            (unsigned long long)ptag16_bytes,
            ptag32_enabled,
            ptag32_allocated,
            (unsigned long long)ptag32_bytes);

    fprintf(stderr,
            "[HZ3_S256_BUILD] num_shards=%u small_num_sc=%u num_sc=%u "
            "bin_total=%u large_sc_count=%u page_size=%u pages_per_seg=%u\n",
            (unsigned)HZ3_NUM_SHARDS,
            (unsigned)HZ3_SMALL_NUM_SC,
            (unsigned)HZ3_NUM_SC,
            (unsigned)HZ3_BIN_TOTAL,
            (unsigned)HZ3_LARGE_SC_COUNT,
            (unsigned)HZ3_PAGE_SIZE,
            (unsigned)HZ3_PAGES_PER_SEG);
}

static void hz3_s256_register_atexit(void) {
    if (atomic_exchange_explicit(&g_hz3_s256_atexit_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s256_wide_rss_dump);
    }
}
#else
static inline void hz3_s256_register_atexit(void) {}
#endif
