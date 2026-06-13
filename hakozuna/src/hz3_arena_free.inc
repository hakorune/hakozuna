// S12-5A: Free a 2MB slot inside arena
// Uses madvise(MADV_DONTNEED) to release physical memory while keeping virtual address
void hz3_arena_free(uint32_t idx) {
    if (idx >= g_hz3_arena.slots) {
        return;
    }

    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        return;
    }

    hz3_lock_acquire(&g_hz3_arena_lock);
    if (atomic_load_explicit(&g_hz3_arena.used[idx], memory_order_relaxed)) {
        void* addr = (char*)base_ptr + (size_t)idx * HZ3_SEG_SIZE;
        // madvise(MADV_DONTNEED): release physical memory, keep virtual address
        madvise(addr, HZ3_SEG_SIZE, MADV_DONTNEED);
        atomic_store_explicit(&g_hz3_arena.used[idx], 0, memory_order_release);
        g_hz3_arena.alloc_cursor = idx;
#if HZ3_S47_SEGMENT_QUARANTINE
        atomic_fetch_sub_explicit(&g_hz3_arena_used_slots, 1, memory_order_relaxed);
        // S47-PolicyBox: Increment free_gen for bounded wait coordination
        atomic_fetch_add_explicit(&g_hz3_arena_free_gen, 1, memory_order_release);
#endif
    }
    hz3_lock_release(&g_hz3_arena_lock);
}

#if HZ3_S47_SEGMENT_QUARANTINE
// S47: Accessor for used_slots (read outside lock is OK, relaxed)
uint32_t hz3_arena_used_slots(void) {
    return atomic_load_explicit(&g_hz3_arena_used_slots, memory_order_relaxed);
}

// S47: Accessor for total slots
uint32_t hz3_arena_total_slots(void) {
    return g_hz3_arena.slots;
}

// S47-PolicyBox: Accessor for free_gen (for bounded wait coordination)
uint32_t hz3_arena_free_gen(void) {
    return atomic_load_explicit(&g_hz3_arena_free_gen, memory_order_acquire);
}
#endif
