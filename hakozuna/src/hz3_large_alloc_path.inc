void* hz3_large_alloc(size_t size) {
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_alloc_call();

    if (size == 0) {
        size = 1;
    }
    hz3_s292_on_alloc_req(size);
    hz3_large_aligned_obs_on_normal_alloc(size);

    size_t offset = hz3_large_user_offset();
    if (size > (SIZE_MAX - offset)) {
        return NULL;
    }

#if HZ3_LARGE_CANARY_ENABLE
    if (size > (SIZE_MAX - offset - HZ3_LARGE_CANARY_SIZE)) {
        return NULL;
    }
    size_t need = offset + size + HZ3_LARGE_CANARY_SIZE;
#else
    size_t need = offset + size;
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
    // S50: Round up to class size, then recompute sc from final size
    // This ensures alloc and free use the same class for the same block.
    int sc = hz3_large_sc(need);
    size_t class_size = hz3_large_sc_size(sc);
    if (class_size == 0) {
        // >64MB: キャッシュ対象外、実サイズ使用
        class_size = hz3_page_align(need);
    } else {
        class_size = hz3_page_align(class_size);
        // Recompute sc from page-aligned size to match what free() will compute
        sc = hz3_large_sc(class_size);
    }

    // Cache lookup: O(1) pop (sc < HZ3_LARGE_SC_COUNT-1 のみ)
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT - 1) {
        int try_sc = sc;
        Hz3LargeHdr* hdr = NULL;

#if HZ3_S240_LARGE_FRONT_CACHE
        hdr = hz3_large_s240_front_pop(sc, class_size, size);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, HZ3_LARGE_ALIGN, sc, 0)) {
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_alloc_front_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_front", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#if HZ3_S240_LARGE_OWNER_INBOX
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
        hdr = hz3_large_s246_inbox_drain_current_take_first_to_front(
            sc, (int)HZ3_S240_LARGE_INBOX_DRAIN_BATCH, size, NULL);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, HZ3_LARGE_ALIGN, sc, 1)) {
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_alloc_inbox_take_first_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_inbox_take_first", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#else
        hz3_large_s240_inbox_drain_current_to_front(
            sc, (int)HZ3_S240_LARGE_INBOX_DRAIN_BATCH);
        hdr = hz3_large_s240_front_pop(sc, class_size, size);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, HZ3_LARGE_ALIGN, sc, 1)) {
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_alloc_inbox_front_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_inbox_front", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#endif
#endif
#endif

        size_t obs_cache_pop_start_ns = hz3_large_aligned_obs_timing_start();

#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → sc+2)
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hz3_large_sc_lock_acquire(try_sc);
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            hz3_large_sc_lock_release(try_sc);
            if (hdr) {
                break;
            }
        }
#else
        hz3_large_sc_lock_acquire(sc);
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
        hz3_large_sc_lock_release(sc);
#endif
#else
        hz3_large_cache_lock_acquire();
#if HZ3_S52_LARGE_BESTFIT
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            if (hdr) {
                break;
            }
        }
#else
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
#endif
        hz3_large_cache_lock_release();
#endif
        hz3_large_aligned_obs_on_alloc_cache_pop_elapsed(obs_cache_pop_start_ns);

        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
            hz3_large_s251_on_global_pop(hdr);
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, try_sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_alloc:
#endif
    hz3_large_stats_on_alloc_cache_miss();
    hz3_large_reuse_note_alloc_miss(sc);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    hz3_large_unmap_defer_drain_on_alloc_miss();
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#if HZ3_S193_DEMAND_SCAVENGE
    hz3_large_scavenge_on_mmap_miss(sc);
#endif
#else
    // Legacy: Cache から探す（first-fit）
    hz3_large_cache_lock_acquire();
    Hz3LargeHdr* prev = NULL;
    Hz3LargeHdr* cur = g_hz3_large_free_head;
    while (cur) {
        if (cur->map_size >= need) {
            // 見つかった: cache から外す
            if (prev) {
                prev->next_free = cur->next_free;
            } else {
                g_hz3_large_free_head = cur->next_free;
            }
            g_hz3_large_cached_bytes -= cur->map_size;
            g_hz3_large_cached_nodes--;
            hz3_large_s251_on_global_pop(cur);
            cur->in_use = 1;
            cur->req_size = size;
            cur->next_free = NULL;
            hz3_large_s240_on_activate(cur, hz3_large_sc(cur->map_size), 1);
            hz3_large_cache_lock_release();

            // bucket に再挿入（usable_size 等で参照される）
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(cur);
#endif
            hz3_large_insert(cur);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", cur->user_ptr, -1, -1);
#endif
            return cur->user_ptr;
        }
        prev = cur;
        cur = cur->next_free;
    }
    hz3_large_cache_lock_release();
#endif
#endif

    Hz3LargeHdr* hdr = NULL;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    hdr = hz3_large_s218_c1_batch_mmap_alloc(size, need, offset, sc, 0, 0);
#endif
    if (!hdr) {
        void* base = hz3_large_os_mmap(need);
        if (base == MAP_FAILED) {
            hz3_oom_note("large_mmap", need, 0);
            return NULL;
        }
        hdr = (Hz3LargeHdr*)base;
        hdr->magic = HZ3_LARGE_MAGIC;
        hdr->req_size = size;
        hdr->map_base = base;
        hdr->map_size = need;
        hdr->user_ptr = (char*)base + offset;
        hdr->next = NULL;
#if HZ3_LARGE_CACHE_ENABLE
        hdr->in_use = 1;
        hdr->next_free = NULL;
#endif
        hz3_large_s240_on_activate(hdr, sc, 0);
    }

#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif
    hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("large_alloc_mmap", hdr->user_ptr, -1, -1);
#endif
    return hdr->user_ptr;
}
