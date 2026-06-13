void hz3_arena_init_slow(void) {
    hz3_once(&g_hz3_arena_once, hz3_arena_do_init);
}

// ----------------------------------------------------------------------------
// S45: Arena accessor API (for mem_budget segment scan)
// ----------------------------------------------------------------------------
void* hz3_arena_get_base(void) {
    return atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
}

uint32_t hz3_arena_slots(void) {
    return g_hz3_arena.slots;
}

int hz3_arena_slot_used(uint32_t idx) {
    if (idx >= g_hz3_arena.slots) {
        return 0;
    }
    return atomic_load_explicit(&g_hz3_arena.used[idx], memory_order_relaxed);
}

// Fast path: reads base atomically, returns 0 if not initialized.
// Does NOT call hz3_once (safe for free hot path).
// NOTE: This function reads used[] - for PageTagMap hot path, use
// hz3_arena_page_index_fast() instead (no used[] read).
int hz3_arena_contains_fast(const void* ptr, uint32_t* idx_out, void** base_out) {
    // Acquire load to synchronize with release store in do_init
    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        return 0;  // Not initialized yet, false negative is OK
    }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)base_ptr;
    if (addr < base || addr >= base + g_hz3_arena.size) {
        return 0;
    }

    uint32_t idx = (uint32_t)((addr - base) / HZ3_SEG_SIZE);
    if (idx >= g_hz3_arena.slots) {
        return 0;
    }

    if (!atomic_load_explicit(&g_hz3_arena.used[idx], memory_order_acquire)) {
        return 0;
    }

    if (idx_out) {
        *idx_out = idx;
    }
    if (base_out) {
        *base_out = (void*)(base + (uintptr_t)idx * HZ3_SEG_SIZE);
    }
    return 1;
}

// Legacy contains: calls init_slow first, then fast path.
int hz3_arena_contains(const void* ptr, uint32_t* idx_out, void** base_out) {
    hz3_arena_init_slow();
    return hz3_arena_contains_fast(ptr, idx_out, base_out);
}

#if HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
// S12-4B: Clear page tags for a segment (all pages in 2MB segment)
// Called when segment is freed/recycled to prevent false positives
void hz3_arena_clear_segment_tags(void* seg_base) {
#if HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_DSTBIN_ENABLE
    if (!g_hz3_page_tag && !g_hz3_page_tag32) {
        return;  // PTAG not initialized
    }
#elif HZ3_SMALL_V2_PTAG_ENABLE
    if (!g_hz3_page_tag) {
        return;  // PTAG not initialized
    }
#else
    if (!g_hz3_page_tag32) {
        return;  // PTAG32 not initialized
    }
#endif
    uint32_t page_idx;
    if (!hz3_arena_page_index_fast(seg_base, &page_idx)) {
        return;  // Not in arena
    }
    // Clear all pages in segment (2MB / 4KB = 512 pages)
    uint32_t pages_per_seg = HZ3_SEG_SIZE >> HZ3_ARENA_PAGE_SHIFT;
#if HZ3_SMALL_V2_PTAG_ENABLE
    for (uint32_t i = 0; i < pages_per_seg; i++) {
        if (g_hz3_page_tag) {
            hz3_pagetag_clear(page_idx + i);
        }
    }
#endif

#if HZ3_PTAG_DSTBIN_ENABLE
    if (g_hz3_page_tag32) {
        for (uint32_t i = 0; i < pages_per_seg; i++) {
            hz3_pagetag32_clear(page_idx + i);
        }
    }
#endif
}
#endif
