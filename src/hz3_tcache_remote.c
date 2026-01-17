// hz3_tcache_remote.c - Outbox and Remote Stash operations
// Extracted from hz3_tcache.c for modularization
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_inbox.h"
#include "hz3_stash_guard.h"
#include "hz3_dtor_stats.h"
#include <string.h>

#ifndef HZ3_LIST_FAILFAST
#define HZ3_LIST_FAILFAST 0
#endif

#ifndef HZ3_CENTRAL_DEBUG
#define HZ3_CENTRAL_DEBUG 0
#endif

#ifndef HZ3_XFER_DEBUG
#define HZ3_XFER_DEBUG 0
#endif

#if HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL && (HZ3_LIST_FAILFAST || HZ3_CENTRAL_DEBUG || HZ3_XFER_DEBUG)
#error "HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL requires list debug checks OFF (HZ3_LIST_FAILFAST/HZ3_CENTRAL_DEBUG/HZ3_XFER_DEBUG)"
#endif

static inline void hz3_stat_update_max_u32(_Atomic(uint32_t)* stat, uint32_t val) {
    uint32_t cur = atomic_load_explicit(stat, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                stat, &cur, val, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

// =============================================================================
// S97: RemoteStashFlush SSOT (atexit one-shot)
// =============================================================================
#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
HZ3_DTOR_STATS_BEGIN(S97);
HZ3_DTOR_STAT(g_s97_flush_budget_calls);
HZ3_DTOR_STAT(g_s97_flush_budget_entries_total);
HZ3_DTOR_STAT(g_s97_flush_budget_groups_total);
HZ3_DTOR_STAT(g_s97_flush_budget_distinct_keys_total);
HZ3_DTOR_STAT(g_s97_flush_budget_potential_merge_calls_total);
HZ3_DTOR_STAT(g_s97_flush_budget_saved_calls_total);
HZ3_DTOR_STAT(g_s97_flush_budget_n_max);
HZ3_DTOR_STAT(g_s97_flush_budget_n_gt1_calls_total);
HZ3_DTOR_STAT(g_s97_flush_budget_n_gt1_entries_total);
HZ3_DTOR_STAT(g_s97_flush_budget_small_groups);
HZ3_DTOR_STAT(g_s97_flush_budget_sub4k_groups);
HZ3_DTOR_STAT(g_s97_flush_budget_medium_groups);
HZ3_DTOR_STAT(g_s97_flush_budget_selfdst_groups);
HZ3_DTOR_ATEXIT_FLAG(g_s97);

static inline int hz3_s97_seen_insert(uint16_t* keys, uint16_t key) {
    // Small fixed-size open addressing set: keys[i]==0xFFFF means empty.
    // Returns 1 if inserted new, 0 if already present.
    const uint32_t mask = 512u - 1u;
    uint32_t idx = ((uint32_t)key * 2654435761u) & mask;
    for (uint32_t probe = 0; probe < 512u; probe++) {
        uint16_t cur = keys[idx];
        if (cur == 0xFFFFu) {
            keys[idx] = key;
            return 1;
        }
        if (cur == key) {
            return 0;
        }
        idx = (idx + 1u) & mask;
    }
    return 0;
}

static void hz3_s97_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_calls);
    uint32_t entries = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_entries_total);
    uint32_t groups = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_groups_total);
    uint32_t distinct = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_distinct_keys_total);
    uint32_t merge_calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_potential_merge_calls_total);
    uint32_t saved_calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_saved_calls_total);
    uint32_t nmax = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_n_max);
    uint32_t ngt1_calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_n_gt1_calls_total);
    uint32_t ngt1_entries = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_n_gt1_entries_total);
    uint32_t small_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_small_groups);
    uint32_t sub4k_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_sub4k_groups);
    uint32_t med_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_medium_groups);
    uint32_t selfdst_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_selfdst_groups);

    fprintf(stderr,
            "[HZ3_S97_REMOTE] bucket=%u skip_tail_null=%u shards=%u bin_total=%u max_keys=%u "
            "calls=%u entries=%u groups=%u distinct=%u "
            "potential_merge_calls=%u saved_calls=%u nmax=%u n_gt1_calls=%u n_gt1_entries=%u "
            "small_groups=%u sub4k_groups=%u medium_groups=%u selfdst_groups=%u\n",
            (unsigned)HZ3_S97_REMOTE_STASH_BUCKET,
            (unsigned)HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL,
            (unsigned)HZ3_NUM_SHARDS,
            (unsigned)HZ3_BIN_TOTAL,
            (unsigned)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS,
            calls, entries, groups, distinct, merge_calls,
            saved_calls, nmax, ngt1_calls, ngt1_entries,
            small_g, sub4k_g, med_g, selfdst_g);
}

#define S97_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s97, hz3_s97_atexit_dump)
#define S97_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S97_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S97_STAT_MAX(name, val) hz3_stat_update_max_u32(&(name), (uint32_t)(val))
#else
#define S97_STAT_REGISTER() ((void)0)
#define S97_STAT_INC(name) ((void)0)
#define S97_STAT_ADD(name, val) ((void)0)
#define S97_STAT_MAX(name, val) ((void)0)
#endif  // HZ3_S97_REMOTE_STASH_FLUSH_STATS

// =============================================================================
// S128: RemoteStash Defer-MinRun (ARCHIVED / NO-GO)
// =============================================================================
// The full implementation was removed from this file to keep the hot boundary
// maintainable. See:
// - hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md
// - hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/s128_defer_minrun.inc

// ============================================================================
// Day 4: Outbox operations
// ============================================================================

#if !HZ3_REMOTE_STASH_SPARSE
// Flush outbox to owner's inbox (FIFO order)
void hz3_outbox_flush(uint8_t owner, int sc) {
    Hz3OutboxBin* ob = &t_hz3_cache.outbox[owner][sc];
    if (ob->count == 0) return;

    // Build FIFO linked list: slots[0] -> slots[1] -> ... -> slots[count-1]
    void* head = ob->slots[0];
    for (uint8_t i = 0; i < ob->count - 1; i++) {
        hz3_obj_set_next(ob->slots[i], ob->slots[i + 1]);
    }
    hz3_obj_set_next(ob->slots[ob->count - 1], NULL);
    void* tail = ob->slots[ob->count - 1];

    // Push to inbox
    hz3_inbox_push_list(owner, sc, head, tail, ob->count);
    ob->count = 0;
}

// Push to outbox, flush if full
void hz3_outbox_push(uint8_t owner, int sc, void* obj) {
    Hz3OutboxBin* ob = &t_hz3_cache.outbox[owner][sc];
    ob->slots[ob->count++] = obj;

    if (ob->count >= HZ3_OUTBOX_SIZE) {
        hz3_outbox_flush(owner, sc);
    }
}
#endif  // !HZ3_REMOTE_STASH_SPARSE

// ============================================================================
// S41: Sparse RemoteStash implementation (scale lane)
// ============================================================================

#if HZ3_REMOTE_STASH_SPARSE
// Forward declarations for dispatcher
extern void hz3_small_v2_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
extern void hz3_sub4k_push_remote_list(uint8_t dst, int sc, void* head, void* tail, uint32_t n);
extern void hz3_inbox_push_list(uint8_t dst, int sc, void* head, void* tail, uint32_t n);

static inline void hz3_remote_stash_dispatch_list(uint8_t dst, uint32_t bin,
                                                 void* head, void* tail, uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, (int)bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = (int)bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
    } else {
        int sc = (int)bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}

void hz3_remote_stash_push(uint8_t dst, uint32_t bin, void* ptr) {
#if HZ3_S82_STASH_GUARD
    hz3_s82_stash_guard_one("remote_stash_push", ptr, bin);
#endif
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t h = ring->head;
    uint16_t next_h = (h + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);

    // Check for overflow BEFORE writing (prevent overwriting old entry)
    if (__builtin_expect(next_h == ring->tail, 0)) {
        // Ring full → emergency flush
        hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
    }

    // Now safe to write
    ring->ring[h].dst = dst;
    ring->ring[h].bin = (uint8_t)bin;
    ring->ring[h].ptr = ptr;

    ring->head = next_h;
    // Mark remote activity to trigger budget flush on next slow path.
    t_hz3_cache.remote_hint = 1;
}

// S41: budget_entries = number of ring entries to drain (not bin count)
void hz3_remote_stash_flush_budget_impl(uint32_t budget_entries) {
    S97_STAT_REGISTER();
    S97_STAT_INC(g_s97_flush_budget_calls);

    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t t = ring->tail;
    uint16_t h = ring->head;
    uint32_t drained = 0;

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    uint16_t seen_keys[512];
    memset(seen_keys, 0xFF, sizeof(seen_keys));
    uint32_t local_groups = 0;  // dispatch calls
    uint32_t local_distinct = 0;
    uint32_t local_n_max = 0;
    uint32_t local_n_gt1_calls = 0;
    uint32_t local_n_gt1_entries = 0;
    uint32_t local_small_groups = 0;
    uint32_t local_sub4k_groups = 0;
    uint32_t local_medium_groups = 0;
    uint32_t local_selfdst_groups = 0;

    #define S97_LOC_GROUPS_INC() do { local_groups++; } while (0)
    #define S97_LOC_NMAX(n_) do { \
        uint32_t s97_n = (uint32_t)(n_); \
        if (s97_n > local_n_max) local_n_max = s97_n; \
    } while (0)
    #define S97_LOC_NGT1(n_) do { \
        uint32_t s97_n = (uint32_t)(n_); \
        if (s97_n > 1) { local_n_gt1_calls++; local_n_gt1_entries += s97_n; } \
    } while (0)
    #define S97_LOC_SEEN(dst_, bin_) do { \
        uint16_t s97_key = (uint16_t)(((uint16_t)(dst_) << 8) | (uint16_t)(bin_)); \
        if (hz3_s97_seen_insert(seen_keys, s97_key)) { local_distinct++; } \
    } while (0)
    #define S97_LOC_CAT(bin_) do { \
        uint32_t s97_bin32 = (uint32_t)(bin_); \
        if (s97_bin32 < HZ3_SMALL_NUM_SC) local_small_groups++; \
        else if (s97_bin32 < HZ3_MEDIUM_BIN_BASE) local_sub4k_groups++; \
        else local_medium_groups++; \
    } while (0)
    #define S97_LOC_SELFDST(dst_) do { if ((uint8_t)(dst_) == (uint8_t)t_hz3_cache.my_shard) local_selfdst_groups++; } while (0)
#endif

#if !HZ3_S97_REMOTE_STASH_FLUSH_STATS
    #define S97_LOC_GROUPS_INC() ((void)0)
    #define S97_LOC_NMAX(n_) ((void)(n_))
    #define S97_LOC_NGT1(n_) ((void)(n_))
    #define S97_LOC_SEEN(dst_, bin_) do { (void)(dst_); (void)(bin_); } while (0)
    #define S97_LOC_CAT(bin_) ((void)(bin_))
    #define S97_LOC_SELFDST(dst_) ((void)(dst_))
#endif

#if HZ3_S97_REMOTE_STASH_BUCKET
#if HZ3_S97_REMOTE_STASH_BUCKET == 2
    // ==========================================================================
    // S97-2: Direct-map + stamp (probe-less bucketize)
    // ==========================================================================
    // Key insight: flat = dst * HZ3_BIN_TOTAL + bin has bounded range, so we can
    // use direct indexing instead of hash + open addressing. Reset via epoch++.

    typedef struct {
        uint8_t dst;
        uint8_t bin;
        uint8_t tail_null_set;
        uint8_t _pad;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S97Bucket;

    enum { S97_FLAT_TOTAL = HZ3_NUM_SHARDS * HZ3_BIN_TOTAL };
    _Static_assert(S97_FLAT_TOTAL <= 65535, "S97_FLAT_TOTAL must fit in uint16_t");

    // TLS direct-map tables (stamp + bucket index)
    static __thread uint32_t s97_stamp[S97_FLAT_TOTAL];
    static __thread uint16_t s97_bucket_idx[S97_FLAT_TOTAL];
    static __thread uint32_t s97_epoch = 0;

    // Local bucket array (stack allocated, reused per flush)
    Hz3S97Bucket buckets[HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS];
    uint32_t nb = 0;

    // First call initialization: set epoch to 1
    if (s97_epoch == 0) {
        s97_epoch = 1;
    }

    // Dispatch macro (same as S97-1)
    #define S97_BUCKET_FLUSH_ROUND() do { \
        for (uint32_t _i = 0; _i < nb; _i++) { \
            Hz3S97Bucket* _b = &buckets[_i]; \
            void* _head = _b->head; \
            void* _tail = _b->tail; \
            uint32_t _n = _b->n; \
            if (_n == 0) continue; \
            if (_n > 1) { \
                if (!_b->tail_null_set) { \
                    hz3_obj_set_next(_tail, NULL); \
                    _b->tail_null_set = 1; \
                } \
            } else { \
                if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) { \
                    hz3_obj_set_next(_tail, NULL); \
                } \
            } \
            S97_LOC_GROUPS_INC(); \
            S97_LOC_NMAX(_n); \
            S97_LOC_NGT1(_n); \
            S97_LOC_SEEN(_b->dst, _b->bin); \
            S97_LOC_CAT(_b->bin); \
            S97_LOC_SELFDST(_b->dst); \
            hz3_remote_stash_dispatch_list(_b->dst, (uint32_t)_b->bin, _head, _tail, _n); \
        } \
        nb = 0; \
    } while (0)

    while (t != h && drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint8_t bin = entry0->bin;
        void* ptr = entry0->ptr;

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

        // Direct-map lookup: flat index = dst * BIN_TOTAL + bin
        uint32_t flat = (uint32_t)dst * (uint32_t)HZ3_BIN_TOTAL + (uint32_t)bin;

        if (s97_stamp[flat] == s97_epoch) {
            // Hit: append to existing bucket
            uint16_t idx = s97_bucket_idx[flat];
            Hz3S97Bucket* b = &buckets[idx];
            // First time we see a 2nd entry, null-terminate original tail
            if (!b->tail_null_set) {
                hz3_obj_set_next(b->tail, NULL);
                b->tail_null_set = 1;
            }
            hz3_obj_set_next(ptr, b->head);
            b->head = ptr;
            b->n++;
        } else {
            // Miss: allocate new bucket
            if (nb >= (uint32_t)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS) {
                // Too many distinct keys: flush and reset epoch
                S97_BUCKET_FLUSH_ROUND();
                s97_epoch++;
                if (s97_epoch == 0) {
                    // Wrap around: clear stamp array
                    memset(s97_stamp, 0, sizeof(s97_stamp));
                    s97_epoch = 1;
                }
            }
            s97_stamp[flat] = s97_epoch;
            s97_bucket_idx[flat] = (uint16_t)nb;
            Hz3S97Bucket* b = &buckets[nb++];
            b->dst = dst;
            b->bin = bin;
            b->tail_null_set = 0;
            b->head = ptr;
            b->tail = ptr;
            b->n = 1;
        }
    }

    S97_BUCKET_FLUSH_ROUND();
    #undef S97_BUCKET_FLUSH_ROUND

    // End of flush: increment epoch for next round (O(1) reset)
    s97_epoch++;
    if (s97_epoch == 0) {
        memset(s97_stamp, 0, sizeof(s97_stamp));
        s97_epoch = 1;
    }

    ring->tail = t;
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
    return;

#elif HZ3_S97_REMOTE_STASH_BUCKET == 6
    // ==========================================================================
    // S97-8: Table-less radix sort + group (stack-only, no TLS tables)
    // ==========================================================================
    // Motivation:
    // - Avoid TLS table pressure (direct-map variants) and avoid hash probe branch-miss.
    // - budget_entries is typically small (e.g., 32), so stack-local sort+group may win.
    //
    // Approach:
    // 1) Drain up to `budget_entries` entries into local arrays (key, ptr).
    // 2) Radix sort (2 passes, 8-bit digits) by 16-bit key = (dst<<8)|bin.
    // 3) Group consecutive identical keys and build a list by prepending.

    enum { S97_SORT_MAX = 256 };

    // Local buffers (stack): use small fixed max and chunk if budget is larger.
    uint16_t keys0[S97_SORT_MAX];
    void* ptrs0[S97_SORT_MAX];
    uint16_t keys1[S97_SORT_MAX];
    void* ptrs1[S97_SORT_MAX];

    // Radix counts: bin is guaranteed < HZ3_BIN_TOTAL and dst < HZ3_NUM_SHARDS.
    // Keep loops bounded to these ranges (avoid fixed 256 cost on every flush).
    _Static_assert(HZ3_BIN_TOTAL <= 255, "S97-8 requires HZ3_BIN_TOTAL <= 255");
    _Static_assert(HZ3_NUM_SHARDS <= 255, "S97-8 requires HZ3_NUM_SHARDS <= 255");

    uint16_t cnt_lo[HZ3_BIN_TOTAL];
    uint16_t pos_lo[HZ3_BIN_TOTAL];
    uint16_t cnt_hi[HZ3_NUM_SHARDS];
    uint16_t pos_hi[HZ3_NUM_SHARDS];

    while (t != h && drained < budget_entries) {
        uint32_t m = 0;
        while (t != h && drained < budget_entries && m < (uint32_t)S97_SORT_MAX) {
            Hz3RemoteStashEntry* entry0 = &ring->ring[t];
            uint8_t dst = entry0->dst;
            uint8_t bin = entry0->bin;
            void* ptr = entry0->ptr;

            t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
            drained++;

            keys0[m] = (uint16_t)(((uint16_t)dst << 8) | (uint16_t)bin);
            ptrs0[m] = ptr;
            m++;
        }

        if (m == 0) {
            break;
        }

        // Pass 0: low byte
        memset(cnt_lo, 0, sizeof(cnt_lo));
        for (uint32_t i = 0; i < m; i++) {
            uint8_t bin = (uint8_t)(keys0[i] & 0xFFu);
            cnt_lo[bin]++;
        }
        uint16_t sum = 0;
        for (uint32_t b = 0; b < (uint32_t)HZ3_BIN_TOTAL; b++) {
            uint16_t c = cnt_lo[b];
            pos_lo[b] = sum;
            sum = (uint16_t)(sum + c);
        }
        for (uint32_t i = 0; i < m; i++) {
            uint8_t bin = (uint8_t)(keys0[i] & 0xFFu);
            uint16_t p = pos_lo[bin]++;
            keys1[p] = keys0[i];
            ptrs1[p] = ptrs0[i];
        }

        // Pass 1: high byte
        memset(cnt_hi, 0, sizeof(cnt_hi));
        for (uint32_t i = 0; i < m; i++) {
            uint8_t dst = (uint8_t)(keys1[i] >> 8);
            cnt_hi[dst]++;
        }
        sum = 0;
        for (uint32_t b = 0; b < (uint32_t)HZ3_NUM_SHARDS; b++) {
            uint16_t c = cnt_hi[b];
            pos_hi[b] = sum;
            sum = (uint16_t)(sum + c);
        }
        for (uint32_t i = 0; i < m; i++) {
            uint8_t dst = (uint8_t)(keys1[i] >> 8);
            uint16_t p = pos_hi[dst]++;
            keys0[p] = keys1[i];
            ptrs0[p] = ptrs1[i];
        }

        // Group + dispatch
        uint32_t i = 0;
        while (i < m) {
            uint16_t key = keys0[i];
            uint8_t dst = (uint8_t)(key >> 8);
            uint8_t bin = (uint8_t)(key & 0xFFu);

            void* head = ptrs0[i];
            void* tail = head;
            uint32_t n = 1;
            i++;

            while (i < m && keys0[i] == key) {
                void* obj = ptrs0[i];
                hz3_obj_set_next(obj, head);
                head = obj;
                n++;
                i++;
            }

            if (n > 1) {
                hz3_obj_set_next(tail, NULL);
            } else {
#if !HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
                hz3_obj_set_next(tail, NULL);
#endif
            }

            S97_LOC_GROUPS_INC();
            S97_LOC_NMAX(n);
            S97_LOC_NGT1(n);
            S97_LOC_SEEN(dst, bin);
            S97_LOC_CAT(bin);
            S97_LOC_SELFDST(dst);
            hz3_remote_stash_dispatch_list(dst, (uint32_t)bin, head, tail, n);
        }
    }

    ring->tail = t;
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
    return;

#else  // HZ3_S97_REMOTE_STASH_BUCKET == 1
    // ==========================================================================
    // S97-1: Hash + open addressing bucketize
    // ==========================================================================
    // Bucketize by (dst,bin) within this flush_budget window.
    // Goal: reduce dispatch calls (push_list) when n=1 dominates but keys repeat.
    typedef struct {
        uint16_t key;  // (dst<<8)|bin
        uint8_t dst;
        uint8_t bin;
        uint8_t tail_null_set;
        uint8_t _pad;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S97Bucket;

    enum { S97_HASH_SIZE = 256 };  // must be power-of-two
    enum { S97_HASH_MASK = S97_HASH_SIZE - 1 };

#if HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS <= 0 || HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS > 256
#error "HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS must be in [1..256]"
#endif

    uint16_t map[S97_HASH_SIZE];
    Hz3S97Bucket buckets[S97_HASH_SIZE];
    uint32_t nb = 0;
    memset(map, 0xFF, sizeof(map));

    // Dispatch current buckets and reset map.
    // Note: does not touch ring indices; only uses accumulated buckets.
    #define S97_BUCKET_FLUSH_ROUND() do { \
        for (uint32_t _i = 0; _i < nb; _i++) { \
            Hz3S97Bucket* _b = &buckets[_i]; \
            void* _head = _b->head; \
            void* _tail = _b->tail; \
            uint32_t _n = _b->n; \
            if (_n == 0) continue; \
            if (_n > 1) { \
                if (!_b->tail_null_set) { \
                    hz3_obj_set_next(_tail, NULL); \
                    _b->tail_null_set = 1; \
                } \
            } else { \
                /* n==1: only null-terminate if requested */ \
                if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) { \
                    hz3_obj_set_next(_tail, NULL); \
                } \
            } \
            S97_LOC_GROUPS_INC(); \
            S97_LOC_NMAX(_n); \
            S97_LOC_NGT1(_n); \
            S97_LOC_SEEN(_b->dst, _b->bin); \
            S97_LOC_CAT(_b->bin); \
            S97_LOC_SELFDST(_b->dst); \
            hz3_remote_stash_dispatch_list(_b->dst, (uint32_t)_b->bin, _head, _tail, _n); \
        } \
        nb = 0; \
        memset(map, 0xFF, sizeof(map)); \
    } while (0)

    while (t != h && drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint8_t bin = entry0->bin;
        void* ptr = entry0->ptr;

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

        uint16_t key = (uint16_t)(((uint16_t)dst << 8) | (uint16_t)bin);
        uint32_t idx = ((uint32_t)key * 2654435761u) & (uint32_t)S97_HASH_MASK;

        for (uint32_t probe = 0; probe < (uint32_t)S97_HASH_SIZE; probe++) {
            uint16_t mi = map[idx];
            if (mi == 0xFFFFu) {
                if (nb >= (uint32_t)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS) {
                    S97_BUCKET_FLUSH_ROUND();
                }
                map[idx] = (uint16_t)nb;
                Hz3S97Bucket* b = &buckets[nb++];
                b->key = key;
                b->dst = dst;
                b->bin = bin;
                b->tail_null_set = 0;
                b->head = ptr;
                b->tail = ptr;
                b->n = 1;
                break;
            }

            Hz3S97Bucket* b = &buckets[mi];
            if (b->key == key) {
                // First time we see a 2nd entry for this key, we must null-terminate
                // the current tail (which is the first element) before linking.
                if (!b->tail_null_set) {
                    hz3_obj_set_next(b->tail, NULL);
                    b->tail_null_set = 1;
                }
                hz3_obj_set_next(ptr, b->head);
                b->head = ptr;
                b->n++;
                break;
            }

            idx = (idx + 1u) & (uint32_t)S97_HASH_MASK;
        }
    }

    S97_BUCKET_FLUSH_ROUND();
    #undef S97_BUCKET_FLUSH_ROUND

    ring->tail = t;
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
    return;
#endif  // HZ3_S97_REMOTE_STASH_BUCKET == 1/2/6
#endif  // HZ3_S97_REMOTE_STASH_BUCKET

    while (t != h && drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint32_t bin = entry0->bin;
        void* tail = entry0->ptr;
        void* head = tail;
#if !HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
        hz3_obj_set_next(tail, NULL);
#endif
        uint32_t n = 1;

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

#if HZ3_S84_REMOTE_STASH_BATCH
        while (t != h && drained < budget_entries && n < (uint32_t)HZ3_S84_REMOTE_STASH_BATCH_MAX) {
            Hz3RemoteStashEntry* entry = &ring->ring[t];
            if (entry->dst != dst || entry->bin != bin) {
                break;
            }
            void* ptr = entry->ptr;
            hz3_obj_set_next(ptr, head);  // LIFO build
            head = ptr;
            n++;
            t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
            drained++;
        }
#endif

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
        S97_LOC_GROUPS_INC();
        S97_LOC_NMAX(n);
        S97_LOC_NGT1(n);
        S97_LOC_SEEN(dst, bin);
        S97_LOC_CAT(bin);
        S97_LOC_SELFDST(dst);
#endif

        hz3_remote_stash_dispatch_list(dst, bin, head, tail, n);
    }

    ring->tail = t;
    // It is safe to clear remote_hint iff the ring is now empty.
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
}

#undef S97_LOC_GROUPS_INC
#undef S97_LOC_NMAX
#undef S97_LOC_NGT1
#undef S97_LOC_SEEN
#undef S97_LOC_CAT
#undef S97_LOC_SELFDST

void hz3_remote_stash_flush_all_impl(void) {
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t t = ring->tail;
    uint16_t h = ring->head;

    while (t != h) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint32_t bin = entry0->bin;
        void* tail = entry0->ptr;
        void* head = tail;
#if !HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
        hz3_obj_set_next(tail, NULL);
#endif
        uint32_t n = 1;

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);

#if HZ3_S84_REMOTE_STASH_BATCH
        while (t != h && n < (uint32_t)HZ3_S84_REMOTE_STASH_BATCH_MAX) {
            Hz3RemoteStashEntry* entry = &ring->ring[t];
            if (entry->dst != dst || entry->bin != bin) {
                break;
            }
            void* ptr = entry->ptr;
            hz3_obj_set_next(ptr, head);  // LIFO build
            head = ptr;
            n++;
            t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        }
#endif

        hz3_remote_stash_dispatch_list(dst, bin, head, tail, n);
    }

    ring->tail = t;
    t_hz3_cache.remote_hint = 0;  // safe, guaranteed empty
}

// Outbox stubs for sparse mode (not used but may be referenced)
void hz3_outbox_flush(uint8_t owner, int sc) {
    (void)owner;
    (void)sc;
}

void hz3_outbox_push(uint8_t owner, int sc, void* obj) {
    (void)owner;
    (void)sc;
    (void)obj;
}
#endif  // HZ3_REMOTE_STASH_SPARSE

// ============================================================================
// S17: dst/bin direct remote bank flush (event-only, dense bank only)
// ============================================================================

#if HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE

#if HZ3_TCACHE_SOA_BANK
// S40: SoA version - detach from bank_head/bank_count
static void hz3_dstbin_detach_soa(uint8_t dst, int bin_idx, void** head_out, void** tail_out, uint32_t* n_out) {
    void* head = t_hz3_cache.bank_head[dst][bin_idx];
    if (!head) {
        *head_out = NULL;
        *tail_out = NULL;
        *n_out = 0;
        return;
    }
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    t_hz3_cache.bank_head[dst][bin_idx] = NULL;
#if !HZ3_BIN_LAZY_COUNT
    t_hz3_cache.bank_count[dst][bin_idx] = 0;
#endif
    *head_out = head;
    *tail_out = tail;
    *n_out = n;
}

// S40: SoA version - flush one bin
void hz3_dstbin_flush_one(uint8_t dst, int bin) {
    if (!t_hz3_cache.bank_head[dst][bin]) return;

    void* head;
    void* tail;
    uint32_t n;
    hz3_dstbin_detach_soa(dst, bin, &head, &tail, &n);
    if (!head) return;

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
    } else {
        int sc = bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}

#else  // !HZ3_TCACHE_SOA_BANK

static void hz3_dstbin_detach(Hz3Bin* bin, void** head_out, void** tail_out, uint32_t* n_out) {
    void* head = hz3_bin_head(bin);
    if (!head) {
        *head_out = NULL;
        *tail_out = NULL;
        *n_out = 0;
        return;
    }
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    hz3_bin_clear(bin);
    *head_out = head;
    *tail_out = tail;
    *n_out = n;
}

// S24: Helper to flush one bin (shared by budget/all)
void hz3_dstbin_flush_one(uint8_t dst, int bin) {
    Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
    if (hz3_bin_is_empty(b)) return;

    void* head;
    void* tail;
    uint32_t n;
    hz3_dstbin_detach(b, &head, &tail, &n);
    if (!head) return;

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
    } else {
        int sc = bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}
#endif  // HZ3_TCACHE_SOA_BANK

#endif  // HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE

// ============================================================================
// S24-1/S41: Remote flush entry points (work for both sparse and dense)
// ============================================================================

#if HZ3_PTAG_DSTBIN_ENABLE
// S24-1: Budgeted flush with round-robin cursor
void hz3_dstbin_flush_remote_budget(uint32_t budget_bins) {
#if HZ3_REMOTE_STASH_SPARSE
    // S41: For sparse ring, budget_bins means "number of ring entries"
    //      (not "number of bins to scan" as in dense bank)
    hz3_remote_stash_flush_budget_impl(budget_bins);
#else
    // Dense bank implementation
    uint8_t my_shard = t_hz3_cache.my_shard;
    uint8_t dst = t_hz3_cache.remote_dst_cursor;
    uint16_t bin = t_hz3_cache.remote_bin_cursor;
    uint32_t scanned = 0;
    uint8_t start_dst = dst;
    int wrapped = 0;

    while (scanned < budget_bins && !wrapped) {
        // Skip my_shard
        if (dst == my_shard) {
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            bin = 0;
            if (dst == start_dst) break;
            continue;
        }

#if HZ3_TCACHE_SOA_BANK
        void* head = t_hz3_cache.bank_head[dst][bin];
#else
        Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
        void* head = b->head;
#endif
        scanned++;
        if (head) {
            hz3_dstbin_flush_one(dst, bin);
            // Found remote -> set hint (budgeted can only raise, never lower)
            t_hz3_cache.remote_hint = 1;
        }

        // Advance cursor
        bin++;
        if (bin >= HZ3_BIN_TOTAL) {
            bin = 0;
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            // Check for wrap-around
            if (dst == start_dst) wrapped = 1;
        }
    }

    // Save cursor for next call
    t_hz3_cache.remote_dst_cursor = dst;
    t_hz3_cache.remote_bin_cursor = bin;
#endif
}

// S24-1: Full flush (all bins, for epoch/destructor)
void hz3_dstbin_flush_remote_all(void) {
#if HZ3_REMOTE_STASH_SPARSE
    hz3_remote_stash_flush_all_impl();
#else
    // Dense bank implementation
    uint8_t my_shard = t_hz3_cache.my_shard;
    int found_any = 0;

    for (uint8_t dst = 0; dst < HZ3_NUM_SHARDS; dst++) {
        if (dst == my_shard) continue;
        for (int bin = 0; bin < HZ3_BIN_TOTAL; bin++) {
#if HZ3_TCACHE_SOA_BANK
            if (t_hz3_cache.bank_head[dst][bin]) {
                hz3_dstbin_flush_one(dst, bin);
                found_any = 1;
            }
#else
            Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
            if (b->head) {
                hz3_dstbin_flush_one(dst, bin);
                found_any = 1;
            }
#endif
        }
    }

    // Full flush confirms hint: 0 if nothing found, 1 if any found
    t_hz3_cache.remote_hint = found_any ? 1 : 0;
    // Reset cursor after full flush
    t_hz3_cache.remote_dst_cursor = 0;
    t_hz3_cache.remote_bin_cursor = 0;
#endif
}
#endif  // HZ3_PTAG_DSTBIN_ENABLE
