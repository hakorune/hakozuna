// ============================================================================
// hz3_hot_dispatch.inc - S113 FreeBox + FreeLeafBox dispatch helpers
// This file is included by hz3_hot.c (single TU)
// ============================================================================

// ============================================================================
// S113 FreeBox: SegMath + page_bin_plus1 fast dispatch (PTAG32 fallback)
// ============================================================================
//
// Replaces PTAG32 lookup with direct pointer math:
//   seg_base = ptr & ~(SEG_SIZE-1)  ->  1 AND
//   slot_idx = (seg_base - arena_base) >> SEG_SHIFT  ->  1 SUB + 1 SHIFT
//   Check used[slot_idx]  ->  1 load (cache line shared with allocations)
//   Read page_bin_plus1[page_idx]  ->  1 load from segment header
//
// If any check fails, returns 0 and lets PTAG32 fallback handle it.

#if HZ3_S113_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE
// Returns 1 if handled (seg math hit), 0 otherwise (fallback to PTAG32)
//
// v2 optimizations:
// - Skip used[] check (page_bin_plus1==0 handles unused segments)
// - Use relaxed memory order (arena_base is immutable after init,
//   page_bin_plus1 is written before malloc returns to same thread)
// - Fewer branches, tighter code
static inline int hz3_free_try_s113_segmath(void* ptr) {
    // Step 1: Load arena_base (relaxed: immutable after init)
#if HZ3_PTAG_DSTBIN_TLS
    void* arena_base = t_hz3_cache.arena_base;
    if (__builtin_expect(!arena_base, 0)) {
        arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        t_hz3_cache.arena_base = arena_base;
    }
#else
    void* arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_relaxed);
#endif
    if (__builtin_expect(!arena_base, 0)) {
        return 0;
    }

    // Step 2: Compute seg_base and range check in one
    uintptr_t seg_base = (uintptr_t)ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);
    uintptr_t delta = seg_base - (uintptr_t)arena_base;
    if (__builtin_expect(delta >= HZ3_ARENA_SIZE, 0)) {
        return 0;
    }

    // Step 3: Compute page_idx, skip page 0
    // (page 0 is header, bin_plus1 would be 0 anyway, but skip for clarity)
    size_t page_idx = ((uintptr_t)ptr - seg_base) >> HZ3_PAGE_SHIFT;
    if (__builtin_expect(page_idx == 0, 0)) {
        return 0;
    }

    // Step 4: Read page_bin_plus1 (relaxed: allocation path ensures visibility)
    Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;
    uint16_t bin_plus1 = atomic_load_explicit(&hdr->page_bin_plus1[page_idx], memory_order_relaxed);
    if (__builtin_expect(bin_plus1 == 0, 0)) {
        return 0;  // Unused page or external allocation
    }

    // Step 5: Extract bin and dst
    uint32_t bin = (uint32_t)(bin_plus1 - 1);
    uint8_t dst = hdr->owner;  // Non-atomic: immutable after segment assignment

    // Sanity check
    if (__builtin_expect(bin >= HZ3_BIN_TOTAL || dst >= HZ3_NUM_SHARDS, 0)) {
#if HZ3_S113_FAILFAST
        fprintf(stderr, "[HZ3_S113_BAD] bin=%u dst=%u ptr=%p\n", bin, dst, ptr);
        __builtin_trap();
#endif
        return 0;
    }

    // Step 6: Ensure tcache initialized
    hz3_tcache_ensure_init();

#if HZ3_S69_LIVECOUNT
    // S69: decrement live_count on fast free paths too (avoid overflow)
    hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_S12_V2_STATS
    t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif

    // Step 7: Dispatch to local or remote bin
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
#endif
    return 1;
}
#endif  // HZ3_S113_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE

// ============================================================================
// FreeLeafBox: Minimal PTAG32 hit path for leaf optimization
// ============================================================================

// Allow command-line override: -DHZ3_FREE_LEAF_ENABLE=0 for A/B testing
#ifndef HZ3_FREE_LEAF_ENABLE
#if HZ3_ENABLE && !HZ3_SHIM_FORWARD_ONLY && HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE && HZ3_PTAG32_DISPATCH_ENABLE && HZ3_PTAG_DSTBIN_FASTLOOKUP && HZ3_PTAG32_NOINRANGE
#define HZ3_FREE_LEAF_ENABLE 1
#else
#define HZ3_FREE_LEAF_ENABLE 0
#endif
#endif

#if HZ3_S263_PTAG_DISPATCH_OBS
#define HZ3_S263_PTAG_KIND_V2 1
#define HZ3_S263_PTAG_KIND_V1_MEDIUM 2

static _Atomic(uint64_t) g_s263_free_entry = 0;
static _Atomic(uint64_t) g_s263_leaf_call = 0;
static _Atomic(uint64_t) g_s263_leaf_hit = 0;
static _Atomic(uint64_t) g_s263_leaf_miss = 0;
static _Atomic(uint64_t) g_s263_leaf_local = 0;
static _Atomic(uint64_t) g_s263_leaf_remote = 0;
static _Atomic(uint64_t) g_s263_leaf_small = 0;
static _Atomic(uint64_t) g_s263_leaf_sub4k = 0;
static _Atomic(uint64_t) g_s263_leaf_medium = 0;
static _Atomic(uint64_t) g_s263_slow_call = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_hit = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_miss = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_external = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_internal_zero = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_local = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_remote = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_small = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_sub4k = 0;
static _Atomic(uint64_t) g_s263_slow_ptag32_medium = 0;
static _Atomic(uint64_t) g_s263_slow_ptag16_hit = 0;
static _Atomic(uint64_t) g_s263_slow_ptag16_zero = 0;
static _Atomic(uint64_t) g_s263_slow_ptag16_small = 0;
static _Atomic(uint64_t) g_s263_slow_ptag16_medium = 0;
static _Atomic(uint64_t) g_s263_slow_segmap_hit = 0;
static _Atomic(uint64_t) g_s263_slow_segmap_miss = 0;
static _Atomic(uint64_t) g_s263_slow_segmap_tag_zero = 0;
static _Atomic(uint64_t) g_s263_slow_segmap_local = 0;
static _Atomic(uint64_t) g_s263_slow_segmap_remote = 0;
static _Atomic(int) g_s263_atexit_registered = 0;

static void hz3_s263_atexit_dump(void) {
    fprintf(stderr,
            "[HZ3_S263_PTAG_DISPATCH] "
            "free_entry=%llu "
            "leaf_call=%llu leaf_hit=%llu leaf_miss=%llu "
            "leaf_local=%llu leaf_remote=%llu "
            "leaf_small=%llu leaf_sub4k=%llu leaf_medium=%llu "
            "slow_call=%llu "
            "slow_ptag32_hit=%llu slow_ptag32_miss=%llu "
            "slow_ptag32_external=%llu slow_ptag32_internal_zero=%llu "
            "slow_ptag32_local=%llu slow_ptag32_remote=%llu "
            "slow_ptag32_small=%llu slow_ptag32_sub4k=%llu slow_ptag32_medium=%llu "
            "slow_ptag16_hit=%llu slow_ptag16_zero=%llu "
            "slow_ptag16_small=%llu slow_ptag16_medium=%llu "
            "slow_segmap_hit=%llu slow_segmap_miss=%llu slow_segmap_tag_zero=%llu "
            "slow_segmap_local=%llu slow_segmap_remote=%llu\n",
            (unsigned long long)atomic_load_explicit(&g_s263_free_entry, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_call, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_hit, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_miss, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_local, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_remote, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_small, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_sub4k, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_leaf_medium, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_call, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_hit, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_miss, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_external, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_internal_zero, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_local, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_remote, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_small, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_sub4k, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag32_medium, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag16_hit, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag16_zero, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag16_small, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_ptag16_medium, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_segmap_hit, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_segmap_miss, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_segmap_tag_zero, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_segmap_local, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s263_slow_segmap_remote, memory_order_relaxed));
}

static inline void hz3_s263_register_once(void) {
    if (atomic_exchange_explicit(&g_s263_atexit_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s263_atexit_dump);
    }
}

static inline void hz3_s263_inc(_Atomic(uint64_t)* counter) {
    atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
}

static inline void hz3_s263_note_bin_band(uint32_t bin,
                                          _Atomic(uint64_t)* small,
                                          _Atomic(uint64_t)* sub4k,
                                          _Atomic(uint64_t)* medium) {
    if (bin < HZ3_SUB4K_BIN_BASE) {
        hz3_s263_inc(small);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        hz3_s263_inc(sub4k);
    } else if (bin < HZ3_BIN_TOTAL) {
        hz3_s263_inc(medium);
    }
}

static inline void hz3_s263_note_free_entry(void) {
    hz3_s263_register_once();
    hz3_s263_inc(&g_s263_free_entry);
}

static inline void hz3_s263_note_leaf_call(void) {
    hz3_s263_inc(&g_s263_leaf_call);
}

static inline void hz3_s263_note_leaf_miss(void) {
    hz3_s263_inc(&g_s263_leaf_miss);
}

static inline void hz3_s263_note_leaf_hit(uint32_t bin, uint8_t dst) {
    hz3_s263_inc(&g_s263_leaf_hit);
    hz3_s263_note_bin_band(bin,
                           &g_s263_leaf_small,
                           &g_s263_leaf_sub4k,
                           &g_s263_leaf_medium);
    if (dst == t_hz3_cache.my_shard) {
        hz3_s263_inc(&g_s263_leaf_local);
    } else {
        hz3_s263_inc(&g_s263_leaf_remote);
    }
}

static inline void hz3_s263_note_slow_call(void) {
    hz3_s263_inc(&g_s263_slow_call);
}

static inline void hz3_s263_note_slow_ptag32_miss(void) {
    hz3_s263_inc(&g_s263_slow_ptag32_miss);
}

static inline void hz3_s263_note_slow_ptag32_external(void) {
    hz3_s263_inc(&g_s263_slow_ptag32_external);
}

static inline void hz3_s263_note_slow_ptag32_internal_zero(void) {
    hz3_s263_inc(&g_s263_slow_ptag32_internal_zero);
}

static inline void hz3_s263_note_slow_ptag32_hit(uint32_t bin, uint8_t dst) {
    hz3_s263_inc(&g_s263_slow_ptag32_hit);
    hz3_s263_note_bin_band(bin,
                           &g_s263_slow_ptag32_small,
                           &g_s263_slow_ptag32_sub4k,
                           &g_s263_slow_ptag32_medium);
    if (dst == t_hz3_cache.my_shard) {
        hz3_s263_inc(&g_s263_slow_ptag32_local);
    } else {
        hz3_s263_inc(&g_s263_slow_ptag32_remote);
    }
}

static inline void hz3_s263_note_slow_ptag16_zero(void) {
    hz3_s263_inc(&g_s263_slow_ptag16_zero);
}

static inline void hz3_s263_note_slow_ptag16_hit(int kind) {
    hz3_s263_inc(&g_s263_slow_ptag16_hit);
    if (kind == HZ3_S263_PTAG_KIND_V2) {
        hz3_s263_inc(&g_s263_slow_ptag16_small);
    } else if (kind == HZ3_S263_PTAG_KIND_V1_MEDIUM) {
        hz3_s263_inc(&g_s263_slow_ptag16_medium);
    }
}

static inline void hz3_s263_note_slow_segmap_hit(void) {
    hz3_s263_inc(&g_s263_slow_segmap_hit);
}

static inline void hz3_s263_note_slow_segmap_miss(void) {
    hz3_s263_inc(&g_s263_slow_segmap_miss);
}

static inline void hz3_s263_note_slow_segmap_tag_zero(void) {
    hz3_s263_inc(&g_s263_slow_segmap_tag_zero);
}

static inline void hz3_s263_note_slow_segmap_dispatch(uint8_t owner) {
    if (owner == t_hz3_cache.my_shard) {
        hz3_s263_inc(&g_s263_slow_segmap_local);
    } else {
        hz3_s263_inc(&g_s263_slow_segmap_remote);
    }
}
#else
#define hz3_s263_note_free_entry() do { } while (0)
#define hz3_s263_note_leaf_call() do { } while (0)
#define hz3_s263_note_leaf_miss() do { } while (0)
#define hz3_s263_note_leaf_hit(bin, dst) do { (void)(bin); (void)(dst); } while (0)
#define hz3_s263_note_slow_call() do { } while (0)
#define hz3_s263_note_slow_ptag32_miss() do { } while (0)
#define hz3_s263_note_slow_ptag32_external() do { } while (0)
#define hz3_s263_note_slow_ptag32_internal_zero() do { } while (0)
#define hz3_s263_note_slow_ptag32_hit(bin, dst) do { (void)(bin); (void)(dst); } while (0)
#define hz3_s263_note_slow_ptag16_zero() do { } while (0)
#define hz3_s263_note_slow_ptag16_hit(kind) do { (void)(kind); } while (0)
#define hz3_s263_note_slow_segmap_hit() do { } while (0)
#define hz3_s263_note_slow_segmap_miss() do { } while (0)
#define hz3_s263_note_slow_segmap_tag_zero() do { } while (0)
#define hz3_s263_note_slow_segmap_dispatch(owner) do { (void)(owner); } while (0)
#endif

#if HZ3_FREE_LEAF_ENABLE
// Returns 1 if handled (PTAG32 hit), 0 otherwise
static inline int hz3_free_try_ptag32_leaf(void* ptr) {
    hz3_s263_note_leaf_call();
    uint32_t tag32 = 0;
    if (!hz3_pagetag32_lookup_hit_fast(ptr, &tag32)) {
        hz3_s263_note_leaf_miss();
        return 0;  // Miss - let slow path handle
    }
    hz3_tcache_ensure_init();

#if HZ3_S69_LIVECOUNT
    // S69: decrement live_count on PTAG32 leaf fast path
    hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_S12_V2_STATS
    t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif

    uint32_t bin = hz3_pagetag32_bin(tag32);
    uint8_t dst = hz3_pagetag32_dst(tag32);
    hz3_s263_note_leaf_hit(bin, dst);

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
#endif
    return 1;  // Handled
}
#endif  // HZ3_FREE_LEAF_ENABLE
