// ============================================================================
// hz3_hot_free_slow.inc - hz3_free_slow implementation (PTAG dispatch)
// This file is included by hz3_hot.c (single TU)
// ============================================================================

__attribute__((noinline))
static void hz3_free_slow(void* ptr) {

#if HZ3_S110_STATS
    // S110-0: Observe segment-local metadata hit rate (no behavior change, PTAG fallback below)
    {
        // Register atexit dump once
        if (!atomic_exchange_explicit(&g_s110_atexit_registered, 1, memory_order_relaxed)) {
            atexit(hz3_s110_atexit_dump);
        }

        Hz3SegHdr* seg = hz3_seg_from_ptr(ptr);
        if (seg) {
            atomic_fetch_add_explicit(&g_s110_seg_hit, 1, memory_order_relaxed);
            // Calculate page_idx using seg_base from hz3_seg_from_ptr (NOT mask)
            size_t page_idx = hz3_ptr_to_page_idx(ptr, (void*)seg);
#if HZ3_S110_META_ENABLE
            uint16_t bin_plus1 = atomic_load_explicit(&seg->page_bin_plus1[page_idx], memory_order_acquire);
            if (bin_plus1 != 0) {
                atomic_fetch_add_explicit(&g_s110_meta_hit, 1, memory_order_relaxed);
            } else {
                atomic_fetch_add_explicit(&g_s110_meta_zero, 1, memory_order_relaxed);
            }
#else
            // page_bin_plus1 not enabled, count as zero
            atomic_fetch_add_explicit(&g_s110_meta_zero, 1, memory_order_relaxed);
#endif
        } else {
            atomic_fetch_add_explicit(&g_s110_seg_miss, 1, memory_order_relaxed);
        }
    }
    // Fall through to PTAG dispatch (Phase 0: observation only)
#endif

#if HZ3_S110_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE
    // S110-1: Optional fast path using segment-local page metadata (PTAG bypass; NO-GO for scale lane).
    {
        Hz3SegHdr* seg = hz3_seg_from_ptr(ptr);  // seg mask 禁止、この関数のみ使う
        if (seg) {
            size_t page_idx = hz3_ptr_to_page_idx(ptr, (void*)seg);

            // page0 は header 自体なので skip (常に fallback)
            if (__builtin_expect(page_idx != 0, 1)) {
                uint16_t bin_plus1 = atomic_load_explicit(&seg->page_bin_plus1[page_idx], memory_order_acquire);

                if (__builtin_expect(bin_plus1 != 0, 1)) {
                    uint32_t bin = (uint32_t)(bin_plus1 - 1);
                    uint8_t dst = seg->owner;

                    // Range validation (opt-in failfast, otherwise silent fallback)
                    if (__builtin_expect(bin < HZ3_BIN_TOTAL && dst < HZ3_NUM_SHARDS, 1)) {
                        hz3_tcache_ensure_init();

#if HZ3_S110_STATS
                        atomic_fetch_add_explicit(&g_s110_fast_taken, 1, memory_order_relaxed);
#endif
                        // Reuse existing local/remote dispatch pattern
#if HZ3_LOCAL_BINS_SPLIT
                        if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
#if HZ3_TCACHE_SOA_LOCAL
                            hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
                            hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
#if HZ3_S110_STATS
                            atomic_fetch_add_explicit(&g_s110_fast_local, 1, memory_order_relaxed);
#endif
                        } else {
#if HZ3_REMOTE_STASH_SPARSE
                            hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
                            hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
                            hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#if HZ3_S110_STATS
                            atomic_fetch_add_explicit(&g_s110_fast_remote, 1, memory_order_relaxed);
#endif
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
                            t_hz3_cache.remote_hint = 1;
#endif
                        }
#elif HZ3_TCACHE_SOA_BANK
                        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
                        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
                        return;  // Fast path success
                    } else {
                        // bin/dst out of range
#if HZ3_S110_FAILFAST
                        fprintf(stderr, "[HZ3_S110_FAILFAST] bin_oob ptr=%p bin=%u dst=%u\n", ptr, bin, (unsigned)dst);
                        abort();
#endif
#if HZ3_S110_STATS
                        atomic_fetch_add_explicit(&g_s110_bin_oob, 1, memory_order_relaxed);
#endif
                    }
                } else {
                    // meta_zero (bin_plus1 == 0)
#if HZ3_S110_STATS
                    atomic_fetch_add_explicit(&g_s110_meta_zero, 1, memory_order_relaxed);
#endif
                }
            } else {
                // page_idx == 0 (segment header page)
#if HZ3_S110_STATS
                atomic_fetch_add_explicit(&g_s110_page0_skip, 1, memory_order_relaxed);
#endif
            }
        }
        // Fall through to PTAG path
#if HZ3_S110_STATS
        atomic_fetch_add_explicit(&g_s110_fallback_ptag, 1, memory_order_relaxed);
#endif
    }
#endif  // HZ3_S110_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
#if HZ3_PTAG_DISPATCH_ENABLE

#if HZ3_PTAG32_DISPATCH_ENABLE
    // S12-5C: Unified dispatch via PageTagMap (both v1 and v2)
    // Step 1: range check (comparison only, no used[] read)
#if HZ3_PTAG_DSTBIN_ENABLE
#if HZ3_PTAG_DSTBIN_FASTLOOKUP

#if HZ3_PTAG32_NOINRANGE
    // S28-5: hit-only lookup (no in_range store on hit path)
    uint32_t tag32 = 0;
    if (hz3_pagetag32_lookup_hit_fast(ptr, &tag32)) {
        hz3_tcache_ensure_init();
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_PTAG_DSTBIN_FLAT
        uint32_t flat = hz3_pagetag32_flat(tag32);
#if HZ3_PTAG_FAILFAST
        if (flat >= (HZ3_NUM_SHARDS * HZ3_BIN_TOTAL)) {
            abort();
        }
#endif
#if HZ3_S78_PTAG32_GUARD
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            hz3_s78_ptag32_guard("ptag32_free_flat", ptr, bin, dst);
        }
#endif
#if HZ3_S83_BIN_PUSH_GUARD
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            hz3_s83_bin_push_guard("ptag32_free_flat", ptr, bin, dst);
        }
#endif
#if HZ3_S81_MEDIUM_FREE_GUARD
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            if (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
                int sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
                hz3_s81_medium_free_guard("ptag32_free_flat", ptr, sc, dst);
            }
        }
#endif
#if HZ3_WATCH_PTR_BOX
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            int sc = -1;
            if (bin < HZ3_SUB4K_BIN_BASE) {
                sc = (int)bin;
            } else if (bin < HZ3_MEDIUM_BIN_BASE) {
                sc = (int)(bin - HZ3_SUB4K_BIN_BASE);
            } else if (bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            }
            hz3_watch_ptr_on_free("ptag32_free_flat", ptr, sc, (int)dst);
        }
#endif
#if HZ3_S72_MEDIUM_DEBUG
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            if (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                int msc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
                hz3_medium_boundary_check_ptr("medium_free_ptag32_flat", ptr, msc, dst);
            }
        }
#endif
        hz3_bin_push(hz3_tcache_get_bank_bin_flat(flat), ptr);
#else
        uint32_t bin = hz3_pagetag32_bin(tag32);
        uint8_t dst = hz3_pagetag32_dst(tag32);
#if HZ3_PTAG_FAILFAST
        if (bin >= HZ3_BIN_TOTAL || dst >= HZ3_NUM_SHARDS) {
            abort();
        }
#endif
#if HZ3_S78_PTAG32_GUARD
        hz3_s78_ptag32_guard("ptag32_free", ptr, bin, dst);
#endif
#if HZ3_S83_BIN_PUSH_GUARD
        hz3_s83_bin_push_guard("ptag32_free", ptr, bin, dst);
#endif
#if HZ3_S81_MEDIUM_FREE_GUARD
        if (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
            int sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            hz3_s81_medium_free_guard("ptag32_free", ptr, sc, dst);
        }
#endif
#if HZ3_WATCH_PTR_BOX
        {
            int sc = -1;
            if (bin < HZ3_SUB4K_BIN_BASE) {
                sc = (int)bin;
            } else if (bin < HZ3_MEDIUM_BIN_BASE) {
                sc = (int)(bin - HZ3_SUB4K_BIN_BASE);
            } else if (bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            }
            hz3_watch_ptr_on_free("ptag32_free", ptr, sc, (int)dst);
        }
#endif
#if HZ3_S72_MEDIUM_DEBUG
        if (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
            int msc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            hz3_medium_boundary_check_ptr("medium_free_ptag32", ptr, msc, dst);
        }
#endif
#if HZ3_LOCAL_BINS_SPLIT
        if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
        } else {
#if HZ3_REMOTE_STASH_SPARSE
            hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
            hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
            t_hz3_cache.remote_hint = 1;
#endif
        }
#elif HZ3_TCACHE_SOA_BANK
        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif  // HZ3_PTAG_DSTBIN_FASTLOOKUP
#endif  // HZ3_PTAG_DSTBIN_ENABLE
#endif  // HZ3_PTAG32_DISPATCH_ENABLE
        return;
    }

#if HZ3_S99_PTAG32_MISS_GUARD
    {
        static _Atomic int g_s99_shot_noinrange = 0;
        uint32_t page_idx = 0;
        if (hz3_arena_page_index_fast(ptr, &page_idx)) {
#if HZ3_S98_PTAG32_CLEAR_MAP
            uint64_t seq = 0;
            uint32_t old_tag = 0;
            void* ra = NULL;
            uintptr_t base = 0;
            unsigned long off = 0;
            int hit = hz3_ptag32_clear_map_lookup(page_idx, &seq, &old_tag, &ra, &base, &off);
            uint32_t old_bin = (hit && old_tag) ? hz3_pagetag32_bin(old_tag) : 0;
            uint8_t old_dst = (hit && old_tag) ? hz3_pagetag32_dst(old_tag) : 0;
#endif
            if (!HZ3_S99_PTAG32_MISS_GUARD_SHOT ||
                atomic_exchange_explicit(&g_s99_shot_noinrange, 1, memory_order_acq_rel) == 0) {
                fprintf(stderr,
                        "[HZ3_S99_PTAG32_MISS] where=free_inrange_tag32_zero ptr=%p page_idx=%u "
#if HZ3_S98_PTAG32_CLEAR_MAP
                        "clear_map_hit=%d clear_map_seq=%llu clear_map_old=0x%x clear_map_old_bin=%u clear_map_old_dst=%u clear_map_off=0x%lx "
#endif
                        "sub4k=%d medium_base=%u bin_total=%u\n",
                        ptr, page_idx,
#if HZ3_S98_PTAG32_CLEAR_MAP
                        hit,
                        (unsigned long long)seq,
                        old_tag,
                        old_bin,
                        (unsigned)old_dst,
                        off,
#endif
                        (int)HZ3_SUB4K_ENABLE,
                        (unsigned)HZ3_MEDIUM_BIN_BASE,
                        (unsigned)HZ3_BIN_TOTAL);
            }
            if (HZ3_S99_PTAG32_MISS_GUARD_FAILFAST) {
                abort();
            }
        }
    }
#endif
    // S28-5: miss path - separate range check (miss is rare)
    if (!hz3_arena_page_index_fast(ptr, NULL)) {
        // Arena external -> large/fallback
        if (hz3_large_free(ptr)) {
            return;
        }
        hz3_next_free(ptr);
        return;
    }
    // Arena internal, tag==0: existing failfast/no-op behavior
#if HZ3_PTAG_FAILFAST
    abort();  // Debug mode: fail fast
#else
    return;   // Release mode: silent no-op
#endif

#else // !HZ3_PTAG32_NOINRANGE
    // Original path with in_range out-param
    uint32_t tag32 = 0;
    int in_range = 0;
    if (hz3_pagetag32_lookup_fast(ptr, &tag32, &in_range)) {
        hz3_tcache_ensure_init();
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_PTAG_DSTBIN_FLAT
        uint32_t flat = hz3_pagetag32_flat(tag32);
#if HZ3_PTAG_FAILFAST
        if (flat >= (HZ3_NUM_SHARDS * HZ3_BIN_TOTAL)) {
            abort();
        }
#endif
#if HZ3_S78_PTAG32_GUARD
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            hz3_s78_ptag32_guard("ptag32_free_flat", ptr, bin, dst);
        }
#endif
#if HZ3_S83_BIN_PUSH_GUARD
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            hz3_s83_bin_push_guard("ptag32_free_flat", ptr, bin, dst);
        }
#endif
#if HZ3_S81_MEDIUM_FREE_GUARD
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            if (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
                int sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
                hz3_s81_medium_free_guard("ptag32_free_flat", ptr, sc, dst);
            }
        }
#endif
#if HZ3_WATCH_PTR_BOX
        {
            uint32_t bin = flat % HZ3_BIN_TOTAL;
            uint8_t dst = (uint8_t)(flat / HZ3_BIN_TOTAL);
            int sc = -1;
            if (bin < HZ3_SUB4K_BIN_BASE) {
                sc = (int)bin;
            } else if (bin < HZ3_MEDIUM_BIN_BASE) {
                sc = (int)(bin - HZ3_SUB4K_BIN_BASE);
            } else if (bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            }
            hz3_watch_ptr_on_free("ptag32_free_flat", ptr, sc, (int)dst);
        }
#endif
        hz3_bin_push(hz3_tcache_get_bank_bin_flat(flat), ptr);
#else
        uint32_t bin = hz3_pagetag32_bin(tag32);
        uint8_t dst = hz3_pagetag32_dst(tag32);
#if HZ3_PTAG_FAILFAST
        if (bin >= HZ3_BIN_TOTAL || dst >= HZ3_NUM_SHARDS) {
            abort();
        }
#endif
#if HZ3_S78_PTAG32_GUARD
        hz3_s78_ptag32_guard("ptag32_free", ptr, bin, dst);
#endif
#if HZ3_S83_BIN_PUSH_GUARD
        hz3_s83_bin_push_guard("ptag32_free", ptr, bin, dst);
#endif
#if HZ3_S81_MEDIUM_FREE_GUARD
        if (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
            int sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            hz3_s81_medium_free_guard("ptag32_free", ptr, sc, dst);
        }
#endif
#if HZ3_WATCH_PTR_BOX
        {
            int sc = -1;
            if (bin < HZ3_SUB4K_BIN_BASE) {
                sc = (int)bin;
            } else if (bin < HZ3_MEDIUM_BIN_BASE) {
                sc = (int)(bin - HZ3_SUB4K_BIN_BASE);
            } else if (bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                sc = (int)(bin - HZ3_MEDIUM_BIN_BASE);
            }
            hz3_watch_ptr_on_free("ptag32_free", ptr, sc, (int)dst);
        }
#endif
#if HZ3_LOCAL_BINS_SPLIT
        if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
            // S28-2C: Local: push to local bins (shallow TLS offset)
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
        } else {
            // Remote: push to bank (owner transfer later)
#if HZ3_REMOTE_STASH_SPARSE
	            hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
	            hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
	            hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
            t_hz3_cache.remote_hint = 1;
#endif
        }
#elif HZ3_TCACHE_SOA_BANK
        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#endif
        return;
    }
#if HZ3_S99_PTAG32_MISS_GUARD
    {
        static _Atomic int g_s99_shot = 0;
        uint32_t tag32_full = 0;
        int in_range = 0;
        (void)hz3_pagetag32_lookup_fast(ptr, &tag32_full, &in_range);
        if (in_range && tag32_full == 0) {
            if (!HZ3_S99_PTAG32_MISS_GUARD_SHOT ||
                atomic_exchange_explicit(&g_s99_shot, 1, memory_order_acq_rel) == 0) {
                uint32_t page_idx = 0;
                (void)hz3_arena_page_index_fast(ptr, &page_idx);
#if HZ3_S98_PTAG32_CLEAR_MAP
                uint64_t seq = 0;
                uint32_t old_tag = 0;
                void* ra = NULL;
                uintptr_t base = 0;
                unsigned long off = 0;
                int hit = hz3_ptag32_clear_map_lookup(page_idx, &seq, &old_tag, &ra, &base, &off);
                uint32_t old_bin = (hit && old_tag) ? hz3_pagetag32_bin(old_tag) : 0;
                uint8_t old_dst = (hit && old_tag) ? hz3_pagetag32_dst(old_tag) : 0;
#endif
                fprintf(stderr,
                        "[HZ3_S99_PTAG32_MISS] where=free_inrange_tag32_zero ptr=%p page_idx=%u "
#if HZ3_S98_PTAG32_CLEAR_MAP
                        "clear_map_hit=%d clear_map_seq=%llu clear_map_old=0x%x clear_map_old_bin=%u clear_map_old_dst=%u clear_map_off=0x%lx "
#endif
                        "sub4k=%d medium_base=%u bin_total=%u\n",
                        ptr, page_idx,
#if HZ3_S98_PTAG32_CLEAR_MAP
                        hit,
                        (unsigned long long)seq,
                        old_tag,
                        old_bin,
                        (unsigned)old_dst,
                        off,
#endif
                        (int)HZ3_SUB4K_ENABLE,
                        (unsigned)HZ3_MEDIUM_BIN_BASE,
                        (unsigned)HZ3_BIN_TOTAL);
            }
            if (HZ3_S99_PTAG32_MISS_GUARD_FAILFAST) {
                abort();
            }
        }
    }
#endif
    if (!in_range) {
        // Arena external -> large/fallback
        if (hz3_large_free(ptr)) {
            return;
        }
        hz3_next_free(ptr);
        return;
    }
#if HZ3_PTAG_FAILFAST
    abort();  // Debug mode: fail fast
#else
    return;   // Release mode: silent no-op
#endif
#endif // HZ3_PTAG32_NOINRANGE
#elif HZ3_PTAG_DSTBIN_TLS
    void* arena_base = t_hz3_cache.arena_base;
    _Atomic(uint32_t)* tag32_base = t_hz3_cache.page_tag32;
    if (__builtin_expect(!arena_base, 0)) {
        arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        t_hz3_cache.arena_base = arena_base;
    }
    if (__builtin_expect(!arena_base, 0)) {
        // Arena external -> large/fallback
        if (hz3_large_free(ptr)) {
            return;
        }
        hz3_next_free(ptr);
        return;
    }
    if (__builtin_expect(!tag32_base, 0)) {
        tag32_base = g_hz3_page_tag32;
        t_hz3_cache.page_tag32 = tag32_base;
    }
    if (tag32_base) {
        uintptr_t delta = (uintptr_t)ptr - (uintptr_t)arena_base;
#if HZ3_ARENA_SIZE == (1ULL << 32)
        if (__builtin_expect((delta >> 32) != 0, 0)) {
#else
        if (__builtin_expect(delta >= (uintptr_t)HZ3_ARENA_SIZE, 0)) {
#endif
            // Arena external -> large/fallback
            if (hz3_large_free(ptr)) {
                return;
            }
            hz3_next_free(ptr);
            return;
        }
        uint32_t page_idx = (uint32_t)(delta >> HZ3_ARENA_PAGE_SHIFT);
        uint32_t tag32_tls = atomic_load_explicit(&tag32_base[page_idx], memory_order_relaxed);
        if (tag32_tls == 0) {
#if HZ3_PTAG_FAILFAST
            abort();  // Debug mode: fail fast
#else
            return;   // Release mode: silent no-op
#endif
        }

        hz3_tcache_ensure_init();
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_PTAG_DSTBIN_FLAT
        uint32_t flat = hz3_pagetag32_flat(tag32_tls);
#if HZ3_PTAG_FAILFAST
        if (flat >= (HZ3_NUM_SHARDS * HZ3_BIN_TOTAL)) {
            abort();
        }
#endif
        hz3_bin_push(hz3_tcache_get_bank_bin_flat(flat), ptr);
#else
        uint32_t bin = hz3_pagetag32_bin(tag32_tls);
        uint8_t dst = hz3_pagetag32_dst(tag32_tls);
#if HZ3_PTAG_FAILFAST
        if (bin >= HZ3_BIN_TOTAL || dst >= HZ3_NUM_SHARDS) {
            abort();
        }
#endif
#if HZ3_LOCAL_BINS_SPLIT
        if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
            // S28-2C: Local: push to local bins (shallow TLS offset)
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
        } else {
            // Remote: push to bank (owner transfer later)
#if HZ3_REMOTE_STASH_SPARSE
            hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
            hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
        }
#elif HZ3_TCACHE_SOA_BANK
        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#endif
        return;
    }
#else
    if (g_hz3_page_tag32) {
        uint32_t page_idx;
        if (!hz3_arena_page_index_fast(ptr, &page_idx)) {
            // Arena external -> large/fallback
            if (hz3_large_free(ptr)) {
                return;
            }
            hz3_next_free(ptr);
            return;
        }
        uint32_t tag32 = hz3_pagetag32_load(page_idx);
        if (tag32 == 0) {
#if HZ3_PTAG_FAILFAST
            abort();  // Debug mode: fail fast
#else
            return;   // Release mode: silent no-op
#endif
        }

        hz3_tcache_ensure_init();
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_PTAG_DSTBIN_FLAT
        uint32_t flat = hz3_pagetag32_flat(tag32);
#if HZ3_PTAG_FAILFAST
        if (flat >= (HZ3_NUM_SHARDS * HZ3_BIN_TOTAL)) {
            abort();
        }
#endif
        hz3_bin_push(hz3_tcache_get_bank_bin_flat(flat), ptr);
#else
        uint32_t bin = hz3_pagetag32_bin(tag32);
        uint8_t dst = hz3_pagetag32_dst(tag32);
#if HZ3_PTAG_FAILFAST
        if (bin >= HZ3_BIN_TOTAL || dst >= HZ3_NUM_SHARDS) {
            abort();
        }
#endif
#if HZ3_LOCAL_BINS_SPLIT
        if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
            // S28-2C: Local: push to local bins (shallow TLS offset)
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
        } else {
            // Remote: push to bank (owner transfer later)
#if HZ3_REMOTE_STASH_SPARSE
            hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
            hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
            hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
        }
#elif HZ3_TCACHE_SOA_BANK
        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#endif
        return;
    }
#endif
#endif

#if HZ3_PTAG16_DISPATCH_ENABLE
    uint32_t page_idx;
    if (!hz3_arena_page_index_fast(ptr, &page_idx)) {
        // Arena external -> large/fallback
        if (hz3_large_free(ptr)) {
            return;
        }
        hz3_next_free(ptr);
        return;
    }

    // Step 2: tag load (1 load)
    uint16_t tag = hz3_pagetag_load(page_idx);
    if (tag == 0) {
        // S12-5C: Arena-internal but tag==0 means unallocated/freed
        // Don't pass to hz3_next_free() - that hides mixed allocation bugs
#if HZ3_PTAG_FAILFAST
        abort();  // Debug mode: fail fast
#else
        return;   // Release mode: silent no-op (double-free equivalent)
#endif
    }

    // Step 3: dispatch by kind
#if HZ3_FREE_FASTPATH_SPLIT
    int kind = tag >> 14;
    if (kind == PTAG_KIND_V2) {
        hz3_free_v2_impl(ptr, tag);
        return;
    }
    if (kind == PTAG_KIND_V1_MEDIUM) {
        hz3_free_medium_impl(ptr, tag);
        return;
    }
#else
    int sc, owner, kind;
    hz3_pagetag_decode_with_kind(tag, &sc, &owner, &kind);

    switch (kind) {
        case PTAG_KIND_V2:
            hz3_small_v2_free_by_tag(ptr, sc, owner);
#if HZ3_S12_V2_STATS
            hz3_tcache_ensure_init();
            t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif
            return;
        case PTAG_KIND_V1_MEDIUM:
            hz3_medium_free_by_tag(ptr, sc, owner);
            return;
        default:
            break;
    }
#endif
    // Unknown kind -> no-op (don't crash in production)
#if HZ3_PTAG_FAILFAST
    abort();
#else
    return;
#endif

#elif HZ3_SMALL_V2_PTAG_ENABLE
    // S12-4C: PageTagMap hot path (v2 only, v1 falls through to segmap)
    // Step 1: range check (comparison only, no used[] read)
    uint32_t page_idx;
    if (hz3_arena_page_index_fast(ptr, &page_idx)) {
        // Step 2: tag load (1 load - miss side ends here!)
        uint16_t tag = hz3_pagetag_load(page_idx);
        if (tag != 0) {
            // Step 3: decode & process (no page_hdr read)
            int sc, owner;
            hz3_pagetag_decode(tag, &sc, &owner);
            hz3_small_v2_free_by_tag(ptr, sc, owner);
#if HZ3_S12_V2_STATS
            hz3_tcache_ensure_init();
            t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif
            return;
        }
        // tag == 0: not a small v2 page, fall through to v1 path
    }
#endif  // HZ3_PTAG16_DISPATCH_ENABLE || HZ3_SMALL_V2_PTAG_ENABLE

#else  // !HZ3_PTAG_DISPATCH_ENABLE
    // Task 3: page_hdr based detection (no seg_hdr cold reads)
    // Step 1: arena range check (must reject first - reading page_hdr for ptr outside arena is dangerous)
    uint32_t arena_idx;
    void* arena_base;
    if (hz3_arena_contains_fast(ptr, &arena_idx, &arena_base)) {
        // Step 2: page_hdr->magic for small v2 detection
        void* page_base = (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
        uint32_t magic = *(uint32_t*)page_base;
        if (magic == HZ3_PAGE_MAGIC) {
            hz3_small_v2_free_fast(ptr, page_base);
#if HZ3_S12_V2_STATS
            // TLS initialized by hz3_small_v2_free_fast()
            t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif
            return;
        }
        // Not a small v2 page - fall through to v1 path
    }
#endif // HZ3_PTAG_DISPATCH_ENABLE
#endif // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

#if !HZ3_PTAG16_DISPATCH_ENABLE
    // v1 fallback path: only needed when PTAG unified dispatch is not enabled
    // When HZ3_PTAG_V1_ENABLE=1, all v1/v2 handled via PageTagMap above

    // Look up segment metadata (lock-free)
    Hz3SegMeta* meta = hz3_segmap_get(ptr);
    if (!meta) {
        if (hz3_large_free(ptr)) {
            return;
        }
        // Not our allocation, fallback
        hz3_next_free(ptr);
        return;
    }

    // Get page index within segment (v1 path)
    void* seg_base = hz3_ptr_to_seg_base(ptr);
    size_t seg_page_idx = hz3_ptr_to_page_idx(ptr, seg_base);

    // Read tag (hot path: read-only)
    uint16_t tag = meta->sc_tag[seg_page_idx];
    if (tag == 0) {
        // Not cached / unknown, fallback
        hz3_next_free(ptr);
        return;
    }

    uint8_t kind = hz3_tag_kind(tag);
    int sc = hz3_tag_sc(tag);
    if (kind == HZ3_TAG_KIND_SMALL) {
        hz3_small_free(ptr, sc);
        return;
    }
    if (kind != HZ3_TAG_KIND_LARGE) {
        hz3_next_free(ptr);
        return;
    }

    // Ensure TLS cache is initialized
    hz3_tcache_ensure_init();

    // Day 4: Local vs Remote free
    uint8_t owner = meta->owner;
    if (owner == t_hz3_cache.my_shard) {
        // Local: push to bin (fast path)
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_medium(sc)), ptr);
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        hz3_bin_push(bin, ptr);
#endif
    } else {
        // Remote: push to outbox (batches to inbox)
        hz3_outbox_push(owner, sc, ptr);
    }
#endif // !HZ3_PTAG16_DISPATCH_ENABLE
}
