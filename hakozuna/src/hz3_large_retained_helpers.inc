#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline int hz3_s276_try_activate_retained_from_cache(Hz3LargeHdr* hdr,
                                                            size_t size,
                                                            size_t alignment,
                                                            int sc,
                                                            int from_inbox) {
    if (!hz3_large_s287_can_activate_retained_exact_fast(hdr, size, alignment, sc) &&
        !hz3_large_s276_can_activate_retained(hdr, size, alignment, sc)) {
        return 0;
    }
    hz3_s285_on_activate_success(from_inbox);
    hdr->in_use = 1;
    hdr->req_size = size;
    hdr->next_free = NULL;
    hz3_large_s276_on_activate_retained(hdr, sc, 1);
    if (from_inbox) {
        hz3_large_s276_note_inbox_activate_fast();
    } else {
        hz3_large_s276_note_front_activate_fast();
    }
    return 1;
}

static inline void hz3_s276_clear_retained_for_rekey(Hz3LargeHdr* hdr) {
    if (hdr && (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
        hz3_large_s276_note_front_activate_rekey();
        hz3_s276_direct_clear_retained(hdr);
    }
}
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
static inline void hz3_s270_dispose_detached_victim(Hz3LargeHdr* victim,
                                                    const char* where) {
    hz3_s270_validate_victim_before_dispose(victim, where);
    if (!victim) {
        return;
    }
#if HZ3_LARGE_DEBUG
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(victim);
    hz3_large_cache_lock_release();
#endif
#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS
    atomic_fetch_add_explicit(&g_s270_quarantine_victim_calls,
                              1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_quarantine_victim_bytes,
                              victim->map_size,
                              memory_order_relaxed);
    hz3_large_s240_on_munmap_store(victim);
    victim->next = NULL;
    victim->next_free = NULL;
    victim->magic = 0;
#else
    hz3_large_dispose_victim_unlocked(victim);
#endif
}
#endif
