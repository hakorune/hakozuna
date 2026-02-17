// hz4_tcache_free.inc - TCache free/gate/public free slice
#ifndef HZ4_TCACHE_FREE_INC
#define HZ4_TCACHE_FREE_INC

// B33 effective gate:
// Relaxed useddec is safe only when decommit/segment-release/central-pageheap
// ordering constraints are inactive.
#if HZ4_ST_FREE_USEDDEC_RELAXED && !HZ4_PAGE_DECOMMIT && !HZ4_SEG_RELEASE_EMPTY && !HZ4_CENTRAL_PAGEHEAP
#define HZ4_ST_FREE_USEDDEC_RELAXED_EFF 1
#else
#define HZ4_ST_FREE_USEDDEC_RELAXED_EFF 0
#endif

// ============================================================================
// Stage5-N6: RemotePageRbufGateBox - evaluate gate based on remote free rate
// ============================================================================
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
__attribute__((noinline, unused))
static void hz4_rbuf_gate_evaluate(hz4_tls_t* tls) {
    uint32_t total = tls->rbuf_gate_total_frees;
    uint32_t remote = tls->rbuf_gate_remote_frees;
    
    // Reset counters
    tls->rbuf_gate_total_frees = 0;
    tls->rbuf_gate_remote_frees = 0;
    
    // Hold period (flap suppression)
    if (tls->rbuf_gate_hold_left > 0) {
        tls->rbuf_gate_hold_left--;
        if (tls->rbuf_gate_on) {
            if (tls->rbuf_gate_on_windows < 255) tls->rbuf_gate_on_windows++;
        } else {
            tls->rbuf_gate_on_windows = 0;
        }
        return;
    }
    
    // Hysteresis logic (cross-multiplication to avoid division)
    // remote% >= HI% means remote*100 >= HI*total
    // remote% <= LO% means remote*100 <= LO*total
    if (remote * 100 >= HZ4_REMOTE_PAGE_RBUF_GATE_HI_PCT * total && !tls->rbuf_gate_on) {
        tls->rbuf_gate_on = 1;
        tls->rbuf_gate_on_windows = 0;
        tls->rbuf_gate_hold_left = HZ4_REMOTE_PAGE_RBUF_GATE_HOLD_WINDOWS;
    } else if (remote * 100 <= HZ4_REMOTE_PAGE_RBUF_GATE_LO_PCT * total && tls->rbuf_gate_on) {
        tls->rbuf_gate_on = 0;
        tls->rbuf_gate_on_windows = 0;
        tls->rbuf_gate_hold_left = HZ4_REMOTE_PAGE_RBUF_GATE_HOLD_WINDOWS;
    }

    if (tls->rbuf_gate_on) {
        if (tls->rbuf_gate_on_windows < 255) tls->rbuf_gate_on_windows++;
    } else {
        tls->rbuf_gate_on_windows = 0;
    }
}
#endif

// page を渡す版（二重計算削減, TLS already provided）
HZ4_ALWAYS_INLINE
static inline void hz4_small_free_with_page_tls(hz4_tls_t* tls, void* ptr, hz4_page_t* page) {
#if HZ4_FAILFAST || HZ4_LOCAL_MAGIC_CHECK
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("hz4_small_free_with_page: invalid page");
        abort();
    }
#endif
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
#endif
#if HZ4_ST_ROUTE_FASTBOX
#if HZ4_PAGE_META_SEPARATE
    uint8_t sc = meta->sc;
#if HZ4_FAILFAST
    if (meta->owner_tid != tls->tid) {
        HZ4_FAIL("hz4_small_free_with_page: owner mismatch (ST route fast)");
        abort();
    }
#endif
#else
    uint8_t sc = page->sc;
#if HZ4_FAILFAST
    if (page->owner_tid != tls->tid) {
        HZ4_FAIL("hz4_small_free_with_page: owner mismatch (ST route fast)");
        abort();
    }
#endif
#endif
#elif HZ4_ST_FREE_ROUTE_BOX
#if HZ4_PAGE_META_SEPARATE
    uint8_t sc = meta->sc;
#else
    uint8_t sc = page->sc;
#endif
#elif HZ4_ST_LOCAL_FREE_FAST
#if HZ4_PAGE_META_SEPARATE
    uint8_t sc = meta->sc;
#if HZ4_FAILFAST
    if (meta->owner_tid != tls->tid) {
        HZ4_FAIL("hz4_small_free_with_page: owner mismatch (ST fast)");
        abort();
    }
#endif
#else
    uint8_t sc = page->sc;
#if HZ4_FAILFAST
    if (page->owner_tid != tls->tid) {
        HZ4_FAIL("hz4_small_free_with_page: owner mismatch (ST fast)");
        abort();
    }
#endif
#endif
#else
#if HZ4_PAGE_META_SEPARATE
#if HZ4_SMALL_FREE_META_DIRECT_BOX
    uint8_t sc = (uint8_t)meta->sc;
    uint16_t owner_tid = meta->owner_tid;
#else
    uint32_t tag = hz4_page_tag_load(page);
    uint8_t sc = hz4_page_tag_sc(tag);
    uint16_t owner_tid = hz4_page_tag_owner(tag);
#endif
#if !HZ4_PAGE_DECOMMIT
    (void)meta;  // suppress unused warning when decommit disabled
#endif
#if HZ4_PAGE_DECOMMIT && HZ4_FAILFAST
    if (meta->decommitted) {
        HZ4_FAIL("hz4_small_free_with_page: decommitted page");
        abort();
    }
#endif
#else
    uint32_t tag = hz4_page_tag_load(page);
    uint8_t sc = hz4_page_tag_sc(tag);
    uint16_t owner_tid = hz4_page_tag_owner(tag);
#endif
#endif  // HZ4_ST_LOCAL_FREE_FAST
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("hz4_small_free_with_page: invalid size class");
        abort();
    }
#endif

#if !HZ4_ST_LOCAL_FREE_FAST && !HZ4_ST_FREE_ROUTE_BOX && !HZ4_ST_ROUTE_FASTBOX
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    // Gate counter: optionally sample local frees to reduce fixed cost under low-remote runs.
#if HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX
    // B40: when gate_off && no remote activity, sample more aggressively
    // Add representative value (step) instead of +1 to keep gate ratio correct.
    if (tls->rbuf_gate_on == 0 && tls->rbuf_gate_remote_frees == 0) {
        uint32_t tick = ++tls->rbuf_gate_local_tick;
        if ((tick & ((1u << HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT) - 1u)) == 0) {
            const uint32_t step = (1u << HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT);
            uint32_t old = tls->rbuf_gate_total_frees;
            uint32_t neu = old + step;
            tls->rbuf_gate_total_frees = neu;
            if (((old & (HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES - 1u)) + step) >=
                HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES) {
                hz4_rbuf_gate_evaluate(tls);
            }
        }
    } else
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT > 0
    {
        uint32_t tick = ++tls->rbuf_gate_local_tick;
        if ((tick & ((1u << HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT) - 1u)) == 0) {
            if (((++tls->rbuf_gate_total_frees) & (HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES - 1)) == 0) {
                hz4_rbuf_gate_evaluate(tls);
            }
        }
    }
#else
    {
        if (((++tls->rbuf_gate_total_frees) & (HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES - 1)) == 0) {
            hz4_rbuf_gate_evaluate(tls);
        }
    }
#endif
#endif
#endif

#if HZ4_ST_LOCAL_FREE_FAST || HZ4_ST_FREE_ROUTE_BOX || HZ4_ST_ROUTE_FASTBOX
    hz4_st_stats_b1_free_fast_inc();
    hz4_small_stats_b19_small_free_local_inc();
    if (1) {
#else
    hz4_st_stats_b1_free_fallback_inc();
    if (owner_tid == tls->tid) {
        hz4_small_stats_b19_small_free_local_inc();
#endif
#if HZ4_PAGE_META_SEPARATE
#if HZ4_LOCAL_FREE_FASTDEC
#if HZ4_ST_FREE_USEDDEC_RELAXED_EFF
        uint16_t used = hz4_page_used_dec_meta_relaxed(meta, 1);
#else
        uint16_t used = hz4_page_used_dec_meta(meta, 1);
#endif
#else
        hz4_page_used_dec(page, 1);
        uint16_t used = hz4_page_used_count(page);
#endif
#if HZ4_PAGE_DECOMMIT
#if !HZ4_ST_FREE_ROUTE_BOX && !HZ4_ST_ROUTE_FASTBOX
        if (used == 0) {
            hz4_os_stats_used_zero();
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
            atomic_store_explicit(&meta->cph_state, HZ4_CPH_SEALING, memory_order_relaxed);
            meta->seal_epoch = (uint32_t)tls->collect_count;
            hz4_os_stats_cph_seal_enter();
#endif
  #if HZ4_LOCAL_FREE_OBSERVE
            fprintf(stderr, "[HZ4_LOCAL_FREE_EMPTY] page=%p used_count=0\n", (void*)page);
  #endif
#if HZ4_DECOMMIT_DELAY_QUEUE
            // Phase 2: Enqueue to delay queue（即時decommitはしない）
            hz4_decommit_queue_enqueue(tls, page, meta);
#else
            // Phase 1F: 即時decommit（delay queue OFF時）
#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
            // Phase 1F: Purge tcache before decommit to prevent freelist corruption
            hz4_tcache_purge_page_for_sc(tls, sc, page);
#endif
            hz4_pageq_notify(page);
#if HZ4_LOCAL_FREE_DECOMMIT
            hz4_page_try_decommit(page, meta);
#endif
#endif
        }
  #if HZ4_LOCAL_FREE_OBSERVE
        else if ((used % 10) == 0) {
            fprintf(stderr, "[HZ4_LOCAL_FREE] page=%p used_count=%u\n", (void*)page, used);
        }
  #endif
#else
        (void)used;
#endif
#else
        (void)used;
#endif
#endif
#if HZ4_TLS_MERGE
        hz4_tcache_bin_t* bin = &tls->bins[sc];
#else
        hz4_alloc_tls_t* atls = hz4_alloc_tls_get(tls);
        hz4_tcache_bin_t* bin = &atls->bins[sc];
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST && HZ4_TCACHE_OBJ_CACHE_ON
        uint8_t allow_tcache_first = 1;
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE == 1
        allow_tcache_first = tls->rbuf_gate_on ? 1 : 0;
#elif HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE == 2
        allow_tcache_first = tls->rbuf_gate_on ? 0 : 1;
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST_PAGESEL
        if (used < (uint16_t)HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED) {
            allow_tcache_first = 0;
        }
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED > 0
        if (used > (uint16_t)HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED) {
            allow_tcache_first = 0;
        }
#endif
#if HZ4_PAGE_DECOMMIT
        if (meta->decommitted) {
            allow_tcache_first = 0;
        }
#endif
#if HZ4_DECOMMIT_DELAY_QUEUE
        if (meta->queued_decommit) {
            allow_tcache_first = 0;
        }
#endif
#endif
        if (allow_tcache_first && bin->st_cache_n < HZ4_TCACHE_OBJ_CACHE_SLOTS_ON) {
            hz4_small_stats_b19_small_tcache_first_inc();
            hz4_tcache_push(bin, ptr);
            return;
        }
#endif
#if HZ4_BUMP_FREE_META && HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT && HZ4_PAGE_META_SEPARATE
        if (hz4_bump_free_meta_try_push(tls, bin, page, meta, sc, ptr)) {
            hz4_small_stats_b19_small_bump_meta_inc();
            return;
        }
#endif
        hz4_small_stats_b19_small_tcache_push_inc();
        hz4_tcache_push(bin, ptr);
    } else {
#if !HZ4_ST_LOCAL_FREE_FAST && !HZ4_ST_FREE_ROUTE_BOX && !HZ4_ST_ROUTE_FASTBOX
        hz4_small_stats_b19_small_free_remote_inc();
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    // Gate counter: count remote frees
    tls->rbuf_gate_remote_frees++;
#if HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT > 0
    if (((++tls->rbuf_gate_total_frees) & (HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES - 1)) == 0) {
        hz4_rbuf_gate_evaluate(tls);
    }
#endif
#endif
#if HZ4_REMOTE_INBOX && HZ4_REMOTE_FREE_KEYED && HZ4_PAGE_META_SEPARATE
        uint8_t owner = (uint8_t)hz4_owner_shard(owner_tid);
        hz4_small_stats_b19_small_remote_keyed_inc();
        hz4_remote_free_keyed(tls, ptr, page, meta, owner, sc);
#else
        hz4_small_stats_b19_small_remote_plain_inc();
        hz4_remote_free(tls, ptr);
#endif
#endif
    }
}

#if HZ4_FREE_ROUTE_ORDER_GATEBOX
static __thread uint32_t g_hz4_free_b27_window_total = 0;
static __thread uint32_t g_hz4_free_b27_window_large = 0;
static __thread uint8_t g_hz4_free_b27_mode_large_first =
    (uint8_t)(HZ4_FREE_ROUTE_ORDER_START_LARGE_FIRST != 0);
static __thread uint8_t g_hz4_free_b27_hold_left = 0;
static __thread uint8_t g_hz4_free_b27_frozen = 0;

HZ4_ALWAYS_INLINE
static inline void hz4_free_b27_note_route(uint8_t is_large) {
    if (__builtin_expect(g_hz4_free_b27_frozen != 0, 0)) {
        return;
    }
    uint32_t total = g_hz4_free_b27_window_total + 1u;
    uint32_t large = g_hz4_free_b27_window_large + (uint32_t)(is_large != 0);
    g_hz4_free_b27_window_total = total;
    g_hz4_free_b27_window_large = large;
    if (__builtin_expect(total < (uint32_t)HZ4_FREE_ROUTE_ORDER_WINDOW, 1)) {
        return;
    }

    if (g_hz4_free_b27_mode_large_first) {
        hz4_free_stats_b27_window_large_first_inc();
    } else {
        hz4_free_stats_b27_window_small_first_inc();
    }

    if (g_hz4_free_b27_hold_left > 0) {
        g_hz4_free_b27_hold_left--;
    } else {
        uint32_t ratio100 = large * 100u;
        uint32_t hi = (uint32_t)HZ4_FREE_ROUTE_ORDER_HI_PCT * total;
        uint32_t lo = (uint32_t)HZ4_FREE_ROUTE_ORDER_LO_PCT * total;
        if (g_hz4_free_b27_mode_large_first) {
            if (ratio100 <= lo) {
                g_hz4_free_b27_mode_large_first = 0;
                g_hz4_free_b27_hold_left = (uint8_t)HZ4_FREE_ROUTE_ORDER_HOLD_WINDOWS;
                hz4_free_stats_b27_switch_to_small_first_inc();
                g_hz4_free_b27_frozen = 1;
                hz4_free_stats_b27_freeze_small_first_inc();
            } else if (ratio100 >= hi) {
                g_hz4_free_b27_frozen = 1;
                hz4_free_stats_b27_freeze_large_first_inc();
            }
        } else {
            if (ratio100 >= hi) {
                g_hz4_free_b27_mode_large_first = 1;
                g_hz4_free_b27_hold_left = (uint8_t)HZ4_FREE_ROUTE_ORDER_HOLD_WINDOWS;
                hz4_free_stats_b27_switch_to_large_first_inc();
                g_hz4_free_b27_frozen = 1;
                hz4_free_stats_b27_freeze_large_first_inc();
            } else if (ratio100 <= lo) {
                g_hz4_free_b27_frozen = 1;
                hz4_free_stats_b27_freeze_small_first_inc();
            }
        }
    }

    g_hz4_free_b27_window_total = 0;
    g_hz4_free_b27_window_large = 0;
}

HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_b27_mode_large_first(void) {
    return g_hz4_free_b27_mode_large_first;
}
#else
HZ4_ALWAYS_INLINE
static inline void hz4_free_b27_note_route(uint8_t is_large) {
    (void)is_large;
}
HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_b27_mode_large_first(void) {
    return 1;
}
#endif

#if HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX
HZ4_ALWAYS_INLINE
static inline void hz4_free_b51_note_small(hz4_tls_t* tls, uintptr_t base);
#endif

HZ4_ALWAYS_INLINE
static inline void hz4_free_route_small(hz4_tls_t* tls, void* ptr, hz4_page_t* page) {
    hz4_free_b27_note_route(0);
    hz4_small_stats_b19_free_route_small_inc();
    hz4_free_stats_b26_small_page_valid_hit_inc();
#if HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX
    hz4_free_b51_note_small(tls, (uintptr_t)page);
#endif
    hz4_small_free_with_page_tls(tls, ptr, page);
}

HZ4_ALWAYS_INLINE
static inline void hz4_free_route_mid(void* ptr) {
    hz4_free_b27_note_route(0);
    hz4_small_stats_b19_free_route_mid_inc();
    hz4_free_stats_b26_mid_magic_hit_inc();
    hz4_mid_free(ptr);
}

HZ4_ALWAYS_INLINE
static inline void hz4_free_route_large_magic(void* ptr) {
    hz4_free_b27_note_route(1);
    hz4_small_stats_b19_free_route_large_inc();
    hz4_free_stats_b26_large_magic_hit_inc();
    hz4_large_free(ptr);
}

#if HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX
#define HZ4_FREE_B51_KIND_NONE 0u
#define HZ4_FREE_B51_KIND_SMALL 1u
static __thread uintptr_t g_hz4_free_b51_base[HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS];
static __thread uint8_t g_hz4_free_b51_kind[HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS];

HZ4_ALWAYS_INLINE
static inline uint32_t hz4_free_b51_idx(uintptr_t base) {
    return (uint32_t)((base >> HZ4_PAGE_SHIFT) & (HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS - 1u));
}

HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_b51_gate_on(hz4_tls_t* tls) {
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    return (uint8_t)((tls->rbuf_gate_on != 0) || (tls->rbuf_gate_remote_frees != 0));
#else
    (void)tls;
    return 1;
#endif
}

HZ4_ALWAYS_INLINE
static inline void hz4_free_b51_note_small(hz4_tls_t* tls, uintptr_t base) {
    if (!hz4_free_b51_gate_on(tls)) {
        return;
    }
    uint32_t idx = hz4_free_b51_idx(base);
    g_hz4_free_b51_base[idx] = base;
    g_hz4_free_b51_kind[idx] = HZ4_FREE_B51_KIND_SMALL;
}

HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_b51_try_route(hz4_tls_t* tls, void* ptr) {
    if (!hz4_free_b51_gate_on(tls)) {
        return 0;
    }
    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1u);
    uint32_t idx = hz4_free_b51_idx(base);
    if (__builtin_expect(g_hz4_free_b51_base[idx] != base, 1)) {
        return 0;
    }
    uint8_t kind = g_hz4_free_b51_kind[idx];
    if (kind == HZ4_FREE_B51_KIND_SMALL) {
        hz4_page_t* page = (hz4_page_t*)base;
        if (__builtin_expect(hz4_page_valid(page), 1)) {
            hz4_free_route_small(tls, ptr, page);
            return 1;
        }
    } else {
        return 0;
    }
    g_hz4_free_b51_base[idx] = 0;
    g_hz4_free_b51_kind[idx] = HZ4_FREE_B51_KIND_NONE;
    return 0;
}
#else
HZ4_ALWAYS_INLINE
static inline void hz4_free_b51_note_small(hz4_tls_t* tls, uintptr_t base) {
    (void)tls;
    (void)base;
}
HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_b51_try_route(hz4_tls_t* tls, void* ptr) {
    (void)tls;
    (void)ptr;
    return 0;
}
#endif

HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_is_large_candidate(void* ptr) {
#if HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX
    uintptr_t addr = (uintptr_t)ptr;
    if (__builtin_expect((addr & ((uintptr_t)HZ4_PAGE_SIZE - 1u)) <
                             (uintptr_t)HZ4_LARGE_HEADER_BYTES,
                         0)) {
        return 0;
    }
    uintptr_t haddr = addr - (uintptr_t)HZ4_LARGE_HEADER_BYTES;
    return (uint8_t)((haddr & (uintptr_t)HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK) == 0);
#else
    (void)ptr;
    return 1;
#endif
}

HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_try_large_validate_route(void* ptr) {
    hz4_free_stats_b26_large_validate_call_inc();
#if HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX
    if (!hz4_large_try_free(ptr)) {
        return 0;
    }
#else
    if (!hz4_large_header_valid(ptr)) {
        return 0;
    }
#endif
    hz4_free_b27_note_route(1);
    hz4_small_stats_b19_free_route_large_inc();
    hz4_free_stats_b26_large_validate_hit_inc();
#if !HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX
    hz4_large_free(ptr);
#endif
    return 1;
}

HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_try_large_route(void* ptr) {
    if (!hz4_free_is_large_candidate(ptr)) {
        return 0;
    }
    return hz4_free_try_large_validate_route(ptr);
}

#if HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX
HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_try_superfast_local_small(hz4_tls_t* tls, void* ptr) {
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    if (__builtin_expect((tls->rbuf_gate_on != 0) || (tls->rbuf_gate_remote_frees != 0), 0)) {
        return 0;
    }
#else
    (void)tls;
#endif
    if (__builtin_expect(!hz4_os_is_seg_ptr(ptr), 0)) {
        return 0;
    }
    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
    if (__builtin_expect(*(uint32_t*)base != HZ4_PAGE_MAGIC, 0)) {
        return 0;
    }
    hz4_page_t* page = (hz4_page_t*)base;
    if (__builtin_expect(!hz4_page_valid(page), 0)) {
        return 0;
    }
    hz4_free_route_small(tls, ptr, page);
    return 1;
}
#else
HZ4_ALWAYS_INLINE
static inline uint8_t hz4_free_try_superfast_local_small(hz4_tls_t* tls, void* ptr) {
    (void)tls;
    (void)ptr;
    return 0;
}
#endif

HZ4_ALWAYS_INLINE_EXTERN
void hz4_free(void* ptr) {
    if (!ptr) {
        return;
    }
    hz4_small_stats_b19_free_calls_inc();
    hz4_free_stats_b26_calls_inc();
    hz4_tls_t* tls = hz4_tls_get();

#if HZ4_PAGE_TAG_TABLE
    // Fast path: single lookup via page tag table
    uint32_t tag;
    if (hz4_pagetag_lookup(ptr, &tag)) {
        hz4_small_stats_b19_free_pagetag_hit_inc();
        uint8_t kind = hz4_pagetag_kind(tag);

        if (kind == HZ4_TAG_KIND_SMALL) {
            uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
            hz4_page_t* page = (hz4_page_t*)base;
            hz4_free_route_small(tls, ptr, page);
            return;
        }
        if (kind == HZ4_TAG_KIND_MID) {
            hz4_free_route_mid(ptr);
            return;
        }
        // kind=0 (unknown) → fallback to legacy path
    } else {
        hz4_small_stats_b19_free_pagetag_miss_inc();
    }
#endif

    // Legacy path (PAGE_TAG_TABLE=0 / lookup miss / Large)
    if (hz4_free_b51_try_route(tls, ptr)) {
        return;
    }
#if HZ4_FREE_ROUTE_SMALL_FIRST_BOX
    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
    uint32_t head_magic = *(uint32_t*)base;
    if (head_magic == HZ4_PAGE_MAGIC) {
        hz4_page_t* page = (hz4_page_t*)base;
        hz4_free_route_small(tls, ptr, page);
        return;
    }
    if (head_magic == HZ4_MID_MAGIC) {
        hz4_free_route_mid(ptr);
        return;
    }
    if (head_magic == HZ4_LARGE_MAGIC) {
        hz4_free_route_large_magic(ptr);
        return;
    }
    if (hz4_free_try_large_route(ptr)) {
        return;
    }
#else
#if HZ4_FREE_ROUTE_ORDER_GATEBOX
    if (hz4_free_b27_mode_large_first()) {
        if (hz4_free_try_superfast_local_small(tls, ptr)) {
            return;
        }
#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
        hz4_free_stats_b26_seg_check_inc();
        if (!hz4_os_is_seg_ptr(ptr)) {
            hz4_free_stats_b26_seg_miss_inc();
            if (hz4_free_try_large_route(ptr)) {
                return;
            }
        } else {
            hz4_free_stats_b26_seg_hit_inc();
        }
#else
        if (hz4_free_try_large_route(ptr)) {
            return;
        }
#endif
        uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
        uint32_t head_magic = *(uint32_t*)base;
        if (head_magic == HZ4_MID_MAGIC) {
            hz4_free_route_mid(ptr);
            return;
        }
        if (head_magic == HZ4_LARGE_MAGIC) {
            hz4_free_route_large_magic(ptr);
            return;
        }

        hz4_page_t* page = (hz4_page_t*)base;
        if (hz4_page_valid(page)) {
            hz4_free_route_small(tls, ptr, page);
            return;
        }
    } else {
        uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
        uint32_t head_magic = *(uint32_t*)base;
        if (head_magic == HZ4_MID_MAGIC) {
            hz4_free_route_mid(ptr);
            return;
        }
        if (head_magic == HZ4_LARGE_MAGIC) {
            hz4_free_route_large_magic(ptr);
            return;
        }
        hz4_page_t* page = (hz4_page_t*)base;
        if (hz4_page_valid(page)) {
            hz4_free_route_small(tls, ptr, page);
            return;
        }
        if (hz4_free_try_large_route(ptr)) {
            return;
        }
    }
#else
    if (hz4_free_try_superfast_local_small(tls, ptr)) {
        return;
    }
#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
    hz4_free_stats_b26_seg_check_inc();
    if (!hz4_os_is_seg_ptr(ptr)) {
        hz4_free_stats_b26_seg_miss_inc();
        if (hz4_free_try_large_route(ptr)) {
            return;
        }
    } else {
        hz4_free_stats_b26_seg_hit_inc();
    }
#else
    if (hz4_free_try_large_route(ptr)) {
        return;
    }
#endif
    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
    uint32_t head_magic = *(uint32_t*)base;
    if (head_magic == HZ4_MID_MAGIC) {
        hz4_free_route_mid(ptr);
        return;
    }
    if (head_magic == HZ4_LARGE_MAGIC) {
        hz4_free_route_large_magic(ptr);
        return;
    }

    // base はすでに計算済みなので page として再利用（二重計算削減）
    hz4_page_t* page = (hz4_page_t*)base;
    if (hz4_page_valid(page)) {
        hz4_free_route_small(tls, ptr, page);
        return;
    }
#endif

#endif
    hz4_free_b27_note_route(0);
    hz4_small_stats_b19_free_route_unknown_inc();
    hz4_free_stats_b26_unknown_hit_inc();
    HZ4_FAIL("hz4_free: unknown pointer");
    abort();
}


#endif // HZ4_TCACHE_FREE_INC
