static void hz3_arena_do_init(void) {
    g_hz3_arena.size = (size_t)HZ3_ARENA_SIZE;
    g_hz3_arena.slots = (uint32_t)(g_hz3_arena.size / HZ3_SEG_SIZE);
    g_hz3_arena.alloc_cursor = 0;

    // S113-fix: Allocate extra HZ3_SEG_SIZE bytes to ensure SEG_SIZE alignment.
    // S113 computes seg_base via `ptr & ~(SEG_SIZE - 1)`, so arena_base MUST
    // be aligned to HZ3_SEG_SIZE for correct delta computation.
    size_t reserve = g_hz3_arena.size + HZ3_SEG_SIZE;
    void* raw = mmap(NULL, reserve, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) {
        hz3_oom_note("arena_init_base", reserve, 0);
        atomic_store_explicit(&g_hz3_arena_base, NULL, memory_order_release);
        g_hz3_arena.slots = 0;
        return;
    }

    // Align to HZ3_SEG_SIZE boundary
    uintptr_t raw_addr = (uintptr_t)raw;
    uintptr_t aligned_addr = (raw_addr + HZ3_SEG_SIZE - 1) & ~((uintptr_t)HZ3_SEG_SIZE - 1);
    void* base = (void*)aligned_addr;

    // Unmap front/back unused portions
    size_t front = aligned_addr - raw_addr;
    size_t back = reserve - front - g_hz3_arena.size;
#if defined(_WIN32)
    // Windows VirtualFree cannot partially release a reserved region.
    // Keep the full reservation; commit will succeed within the reserved range.
    (void)front;
    (void)back;
#else
    if (front > 0) {
        munmap(raw, front);
    }
    if (back > 0) {
        munmap((char*)base + g_hz3_arena.size, back);
    }
#endif

    size_t used_bytes = (size_t)g_hz3_arena.slots * sizeof(_Atomic uint8_t);
    void* used = mmap(NULL, used_bytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (used == MAP_FAILED) {
        hz3_oom_note("arena_init_used", used_bytes, 0);
        munmap(base, g_hz3_arena.size);
        atomic_store_explicit(&g_hz3_arena_base, NULL, memory_order_release);
        g_hz3_arena.slots = 0;
        return;
    }

    g_hz3_arena.used = (_Atomic uint8_t*)used;
    for (uint32_t i = 0; i < g_hz3_arena.slots; i++) {
        atomic_store_explicit(&g_hz3_arena.used[i], 0, memory_order_relaxed);
    }

#if HZ3_SMALL_V2_PTAG_ENABLE
    void* page_tag = NULL;
    size_t page_tag_bytes = 0;
#endif
#if HZ3_SMALL_V2_PTAG_ENABLE
    // Allocate page tag array: 4GB / 4KB = 1M pages, 2 bytes each = 2MB
    page_tag_bytes = HZ3_ARENA_MAX_PAGES * sizeof(_Atomic(uint16_t));
#if HZ3_S265_PTAG16_LAZY_COMMIT
    page_tag = mmap(NULL, page_tag_bytes, PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page_tag != MAP_FAILED) {
        size_t commit_pages =
            (page_tag_bytes + HZ3_ARENA_PAGE_SIZE - 1) >> HZ3_ARENA_PAGE_SHIFT;
        size_t state_bytes = commit_pages * sizeof(_Atomic(uint8_t));
        void* state = mmap(NULL, state_bytes, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (state != MAP_FAILED) {
            g_hz3_page_tag = (_Atomic(uint16_t)*)page_tag;
            g_hz3_s265_ptag16_commit_state = (_Atomic(uint8_t)*)state;
            g_hz3_s265_ptag16_commit_pages = (uint32_t)commit_pages;
            for (size_t i = 0; i < commit_pages; i++) {
                atomic_store_explicit(&g_hz3_s265_ptag16_commit_state[i],
                                      HZ3_S265_PTAG16_META_UNCOMMITTED,
                                      memory_order_relaxed);
            }
            hz3_s265_ptag16_register_once();
        } else {
            munmap(page_tag, page_tag_bytes);
            page_tag = MAP_FAILED;
        }
    }

    if (page_tag == MAP_FAILED) {
        // Fallback to eager PTAG16 if lazy metadata setup cannot be created.
        page_tag = mmap(NULL, page_tag_bytes, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
#else
    page_tag = mmap(NULL, page_tag_bytes, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if (page_tag == MAP_FAILED) {
        // Non-fatal: PTAG just won't work, fallback to v1 path
        g_hz3_page_tag = NULL;
        page_tag = NULL;
        page_tag_bytes = 0;
    } else {
        g_hz3_page_tag = (_Atomic(uint16_t)*)page_tag;
        // mmap guarantees zero-filled, but be explicit for clarity
        // (no loop needed, mmap returns zeroed memory)
    }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
    // S17: Allocate 32-bit page tag array: 4GB / 4KB = 1M pages, 4 bytes each = 4MB
    size_t page_tag32_bytes = HZ3_ARENA_MAX_PAGES * sizeof(_Atomic(uint32_t));
    void* page_tag32 = mmap(NULL, page_tag32_bytes, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page_tag32 == MAP_FAILED) {
        g_hz3_page_tag32 = NULL;
#if HZ3_PTAG_DSTBIN_FASTLOOKUP
        hz3_oom_note("arena_ptag32", page_tag32_bytes, 0);
#if HZ3_SMALL_V2_PTAG_ENABLE
        if (page_tag && page_tag_bytes > 0) {
            munmap(page_tag, page_tag_bytes);
            g_hz3_page_tag = NULL;
        }
#endif
        munmap(used, used_bytes);
        munmap(base, g_hz3_arena.size);
        g_hz3_arena.used = NULL;
        g_hz3_arena.slots = 0;
        atomic_store_explicit(&g_hz3_arena_base, NULL, memory_order_release);
        return;
#endif
    } else {
        g_hz3_page_tag32 = (_Atomic(uint32_t)*)page_tag32;
#if HZ3_PTAG32_HUGEPAGE
#ifdef MADV_HUGEPAGE
        // S19-1: Hint THP for random PTAG32 lookups (init-only).
        // Best-effort: ignore failures (older kernels / policies).
        (void)madvise(page_tag32, page_tag32_bytes, MADV_HUGEPAGE);
#endif
#endif
        // Touching is not strictly required; keep init cost minimal.
        // (mmap returns zeroed pages; faults occur naturally on first use)
    }
#endif

    // Compute arena end
    void* end = (char*)base + g_hz3_arena.size;

    // Atomic publish order: end FIRST, then base
    // This ensures readers who see base will also see a valid end
    atomic_store_explicit(&g_hz3_arena_end, end, memory_order_release);
    atomic_store_explicit(&g_hz3_arena_base, base, memory_order_release);
    hz3_s256_register_atexit();
    hz3_win_process_stats_register();
}
