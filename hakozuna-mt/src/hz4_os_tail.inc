void* hz4_os_seg_acquire(void) {
#if HZ4_OS_STATS
    hz4_os_stats_init_once();
#endif
    int extra = hz4_os_mmap_seg_extra_flags();
    void* mem = hz4_os_mmap_aligned((size_t)HZ4_SEG_SIZE, (size_t)HZ4_SEG_SIZE, extra);
    if (!mem) {
        abort();
    }

#if HZ4_OS_SEG_REGISTRY_ACTIVE
    hz4_seg_registry_insert((uintptr_t)mem);
#endif

#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_segs_acquired, 1, memory_order_relaxed);
#endif
#if HZ4_RSSRETURN
    // PressureGateBox 用常時カウンタ（STATSなしならこっちだけ）
    atomic_fetch_add_explicit(&g_hz4_os_segs_acquired_always, 1, memory_order_relaxed);
#endif

#if HZ4_PAGE_TAG_TABLE
    // Initialize page tag table on first segment acquire
    // Uses this segment's base as arena_base
    if (!g_hz4_arena_base) {
        hz4_pagetag_init(mem);
    }
#endif

    return mem;
}

void hz4_os_seg_release(void* seg_base) {
    if (!seg_base) {
        return;
    }
#if HZ4_OS_SEG_REGISTRY_ACTIVE
    hz4_seg_registry_remove((uintptr_t)seg_base);
#endif
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_segs_released, 1, memory_order_relaxed);
#endif
    hz4_os_munmap_aligned(seg_base, (size_t)HZ4_SEG_SIZE);
}

void* hz4_os_page_acquire(void) {
#if HZ4_OS_STATS
    hz4_os_stats_init_once();
#endif
    void* mem = hz4_os_mmap_aligned((size_t)HZ4_PAGE_SIZE, (size_t)HZ4_PAGE_SIZE, 0);
    if (!mem) {
        abort();
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_acquired, 1, memory_order_relaxed);
#endif
    return mem;
}

void hz4_os_page_release(void* page_base) {
    if (!page_base) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_released, 1, memory_order_relaxed);
#endif
    hz4_os_munmap_aligned(page_base, (size_t)HZ4_PAGE_SIZE);
}

void hz4_os_page_decommit(void* page_base) {
    if (!page_base) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_decommitted, 1, memory_order_relaxed);
#endif
    madvise(page_base, (size_t)HZ4_PAGE_SIZE, MADV_DONTNEED);
}

void hz4_os_page_commit(void* page_base) {
    if (!page_base) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_pages_committed, 1, memory_order_relaxed);
#endif
#ifdef MADV_WILLNEED
    madvise(page_base, (size_t)HZ4_PAGE_SIZE, MADV_WILLNEED);
#endif
}

void* hz4_os_large_acquire(size_t size) {
#if HZ4_OS_STATS
    hz4_os_stats_init_once();
#endif
#if HZ4_S220_LARGE_MMAP_NOALIGN
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        HZ4_FAIL("hz4_os_large_acquire: mmap failed");
        return NULL;
    }
#else
    void* mem = hz4_os_mmap_aligned(size, (size_t)HZ4_PAGE_SIZE, 0);
    if (!mem) {
        HZ4_FAIL("hz4_os_large_acquire: mmap failed");
        return NULL;
    }
#endif
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_large_acquired, (uint64_t)size, memory_order_relaxed);
#endif
    return mem;
}

void hz4_os_large_release(void* base, size_t size) {
    if (!base || size == 0) {
        return;
    }
#if HZ4_OS_STATS
    atomic_fetch_add_explicit(&g_hz4_os_large_released, (uint64_t)size, memory_order_relaxed);
#endif
    hz4_os_munmap_aligned(base, size);
}
