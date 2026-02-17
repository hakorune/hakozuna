// hz4_tcache_slow.inc - TCache purge/decommit slow-path slice
#ifndef HZ4_TCACHE_SLOW_INC
#define HZ4_TCACHE_SLOW_INC

#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
// ============================================================================
// Phase 1F: TCache Purge Before Decommit
// ============================================================================
// Purge all objects belonging to target_page from tcache bin and inbox_stash.
// This prevents freelist corruption when the page is decommitted.
//
// Thread safety: owner thread only (single-threaded access to its own tcache)
//
// Critical: used_count==0 means "no live objects", but tcache may still have
// free objects from this page. We must unlink them before decommit.

static inline bool hz4_tcache_bin_has_page(hz4_tcache_bin_t* bin,
                                           hz4_page_t* target_page) {
#if HZ4_TCACHE_OBJ_CACHE_ON
    uint8_t n = bin->st_cache_n;
    while (n) {
        void* obj = bin->st_cache[n - 1];
        if (obj && hz4_page_from_ptr(obj) == target_page) {
            return true;
        }
        n--;
    }
#endif
    void* cur = bin->head;
    while (cur) {
        if (hz4_page_from_ptr(cur) == target_page) {
            return true;
        }
        cur = hz4_obj_get_next(cur);
    }
    return false;
}

static inline uint32_t hz4_tcache_purge_bin_for_page(hz4_tcache_bin_t* bin,
                                                     hz4_page_t* target_page) {
    void* prev = NULL;
    void* cur = bin->head;
    uint32_t purged = 0;

#if HZ4_TCACHE_OBJ_CACHE_ON
    uint8_t n = bin->st_cache_n;
    uint8_t i = 0;
    while (i < n) {
        void* obj = bin->st_cache[i];
        if (obj && hz4_page_from_ptr(obj) == target_page) {
            n--;
            bin->st_cache[i] = bin->st_cache[n];
            bin->st_cache[n] = NULL;
#if HZ4_TCACHE_COUNT
            if (bin->count > 0) {
                bin->count--;
            }
#endif
            purged++;
            continue;  // re-check swapped-in entry
        }
        i++;
    }
    bin->st_cache_n = n;
#endif

    while (cur) {
        void* next = hz4_obj_get_next(cur);
        hz4_page_t* obj_page = hz4_page_from_ptr(cur);

        if (obj_page == target_page) {
            // Unlink cur from freelist
            if (prev) {
                hz4_obj_set_next(prev, next);
            } else {
                bin->head = next;
            }
#if HZ4_TCACHE_COUNT
            if (bin->count > 0) {
                bin->count--;
            }
#endif
            purged++;
            // prev stays the same (cur removed)
        } else {
            prev = cur;
        }
        cur = next;
    }

    return purged;
}

uint32_t hz4_tcache_purge_page_for_sc(hz4_tls_t* tls,
                                       uint8_t sc,
                                       hz4_page_t* target_page) {
    uint32_t total_purged = 0;

#if HZ4_TLS_MERGE
    total_purged += hz4_tcache_purge_bin_for_page(&tls->bins[sc], target_page);
#if HZ4_POPULATE_BATCH
    // Lazy-populate may keep a page alive via bump_page even when the freelist is empty.
    if (tls->bins[sc].bump_page == target_page) {
        tls->bins[sc].bump_page = NULL;
        total_purged++;
    }
#endif
#else
    hz4_alloc_tls_t* atls = hz4_alloc_tls_get(tls);
    total_purged += hz4_tcache_purge_bin_for_page(&atls->bins[sc], target_page);
#if HZ4_POPULATE_BATCH
    if (atls->bins[sc].bump_page == target_page) {
        atls->bins[sc].bump_page = NULL;
        total_purged++;
    }
#endif
#endif

#if HZ4_REMOTE_INBOX
    // Also purge inbox_stash if it contains objects from target_page
    if (tls->inbox_stash[sc]) {
        void* prev = NULL;
        void* cur = tls->inbox_stash[sc];

        while (cur) {
            void* next = hz4_obj_get_next(cur);
            hz4_page_t* obj_page = hz4_page_from_ptr(cur);

            if (obj_page == target_page) {
                // Unlink cur from stash
                if (prev) {
                    hz4_obj_set_next(prev, next);
                } else {
                    tls->inbox_stash[sc] = next;
                }
                total_purged++;
            } else {
                prev = cur;
            }
            cur = next;
        }
    }
#endif

    // Also purge carry slots for this sc.
    // Carry lists are intrusive (obj->next), so they must not reference a decommitted page.
    if (sc < HZ4_SC_MAX) {
        hz4_carry_t* c = &tls->carry[sc];
        if (c->n > 0) {
            hz4_carry_slot_t new_slots[HZ4_CARRY_SLOTS];
            uint8_t new_n = 0;

            for (uint32_t i = 0; i < c->n && i < HZ4_CARRY_SLOTS; i++) {
                hz4_carry_slot_t* slot = &c->slot[i];
                if (slot->page == target_page) {
                    // Drop entire slot (do NOT traverse the list).
                    slot->head = NULL;
                    slot->page = NULL;
                    total_purged++;
                    continue;
                }
                new_slots[new_n++] = *slot;
            }

            // Rewrite slots compactly (LIFO order doesn't matter for correctness).
            for (uint32_t i = 0; i < HZ4_CARRY_SLOTS; i++) {
                if (i < new_n) {
                    c->slot[i] = new_slots[i];
                } else {
                    c->slot[i].head = NULL;
                    c->slot[i].page = NULL;
                }
            }
            c->n = new_n;
        }
    }

#if HZ4_DECOMMIT_OBSERVE
    if (total_purged > 0) {
        fprintf(stderr, "[HZ4_TCACHE_PURGE] sc=%u page=%p purged=%u\n",
                sc, (void*)target_page, total_purged);
    }
#endif

    return total_purged;
}

static inline bool hz4_tcache_has_page_for_sc(hz4_tls_t* tls,
                                              uint8_t sc,
                                              hz4_page_t* target_page) {
#if HZ4_TLS_MERGE
    if (hz4_tcache_bin_has_page(&tls->bins[sc], target_page)) {
        return true;
    }
#if HZ4_POPULATE_BATCH
    if (tls->bins[sc].bump_page == target_page) {
        return true;
    }
#endif
#else
    hz4_alloc_tls_t* atls = hz4_alloc_tls_get(tls);
    if (hz4_tcache_bin_has_page(&atls->bins[sc], target_page)) {
        return true;
    }
#if HZ4_POPULATE_BATCH
    if (atls->bins[sc].bump_page == target_page) {
        return true;
    }
#endif
#endif

#if HZ4_REMOTE_INBOX
    void* cur = tls->inbox_stash[sc];
    while (cur) {
        if (hz4_page_from_ptr(cur) == target_page) {
            return true;
        }
        cur = hz4_obj_get_next(cur);
    }
#endif

    if (sc < HZ4_SC_MAX) {
        hz4_carry_t* c = &tls->carry[sc];
        for (uint32_t i = 0; i < c->n && i < HZ4_CARRY_SLOTS; i++) {
            if (c->slot[i].page == target_page) {
                return true;
            }
        }
    }

    return false;
}
#endif // HZ4_TCACHE_PURGE_BEFORE_DECOMMIT

#endif // HZ4_TCACHE_SLOW_INC
