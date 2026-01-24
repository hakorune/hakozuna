#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include "hz3_config.h"
#include "hz3_small.h"
#include "hz3_tcache.h"
#include "hz3_types.h"
#include "hz3_arena.h"  // S46: for g_hz3_arena_pressure_epoch
#include "hz3_s62_stale_check.h"  // S62-1: stale free_bits fail-fast
#include "hz3_watch_ptr.h"

struct Hz3SegHdr;

// Task 3: page magic for fast small v2 detection (no seg_hdr read)
#define HZ3_PAGE_MAGIC 0x485A5032u  // "HZP2"

void* hz3_small_v2_alloc_slow(int sc);
void  hz3_small_v2_free_remote_slow(void* ptr, int sc, int owner);

// S69: SmallPageLiveCountBox helpers (declared early for inline function use)
#if HZ3_S69_LIVECOUNT
void hz3_s69_live_count_inc(void* ptr);
void hz3_s69_live_count_dec(void* ptr);
uint16_t hz3_s69_live_count_read(void* page_base);
void hz3_s69_retire_blocked_inc(void);
int hz3_small_v2_page_try_get_sc(void* page_base, uint8_t* out_sc);
#endif

#if HZ3_S72_BOUNDARY_DEBUG
// S72: Boundary checks for small_v2 objects/lists (fail-fast / one-shot)
void hz3_small_v2_boundary_check_obj(const char* where, uint8_t owner, int sc, void* obj);
void hz3_small_v2_boundary_check_list(const char* where, uint8_t owner, int sc,
                                      void* head, void* tail, uint32_t n);
#endif

static inline void* hz3_small_v2_alloc(size_t size) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)size;
    return NULL;
#else
    int sc = hz3_small_sc_from_size(size);
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return NULL;
    }

#if !HZ3_TCACHE_INIT_ON_MISS
    hz3_tcache_ensure_init();
#endif
    // S32-1: When HZ3_TCACHE_INIT_ON_MISS=1, skip init here.
    // TLS is zero-initialized, so bin->head == NULL → miss → slow path → init there.

#if HZ3_ARENA_PRESSURE_BOX
    // Pressure check: drop to slow path once per epoch (slow path flushes to central)
    {
        uint32_t cur = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_acquire);
        if (__builtin_expect(cur != 0 && t_hz3_cache.last_pressure_epoch < cur, 0)) {
            return hz3_small_v2_alloc_slow(sc);
        }
    }
#endif

#if HZ3_TCACHE_SOA
    void* obj = hz3_binref_pop(hz3_tcache_get_local_binref(hz3_bin_index_small(sc)));
#else
    Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
    void* obj = hz3_small_bin_pop(bin);
#endif
    if (obj) {
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("small_v2:alloc_local", obj, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_S72_BOUNDARY_DEBUG
        hz3_small_v2_boundary_check_obj("small_v2:alloc_local", t_hz3_cache.my_shard, sc, obj);
#endif
#if HZ3_S62_STALE_FAILFAST
        hz3_s62_stale_check_ptr(obj, "small_alloc_local", sc);
#endif
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_inc(obj);
#endif
        return obj;
    }
    return hz3_small_v2_alloc_slow(sc);
#endif
}
void  hz3_small_v2_free(void* ptr, const struct Hz3SegHdr* hdr);
size_t hz3_small_v2_usable_size(void* ptr);
void  hz3_small_v2_bin_flush(int sc, Hz3Bin* bin);
// S17: remote list push (event-only)
void  hz3_small_v2_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
#if HZ3_LANE_T16_R90_PAGE_REMOTE
extern _Atomic(struct Hz3SegHdr*) g_hz3_lane16_segq[HZ3_NUM_SHARDS];
void  hz3_lane16_push_remote_list_small(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
uint32_t hz3_lane16_pop_batch_small(uint8_t owner, int sc, void** out, uint32_t want);
#endif
#if HZ3_HZ4_BRIDGE
void  hz3_hz4_bridge_push_remote_list_small(uint8_t owner, int sc,
                                            void* head, void* tail, uint32_t n);
uint32_t hz3_hz4_bridge_pop_batch_small(uint8_t owner, int sc,
                                        void** out, uint32_t want);
#endif

// S40: central push list (exposed for SoA destructor)
void  hz3_small_v2_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n);

// S62-1: Pop batch from central bin (exported, wraps hz3_central_bin_pop_batch)
// Returns count actually popped (0 if bin empty)
uint32_t hz3_small_v2_central_pop_batch_ext(uint8_t shard, int sc, void** out, uint32_t want);

// S62-1: Get capacity (objects per page) for size class
// Same logic as hz3_small_v2_fill_bin() for accurate retire判定
size_t hz3_small_v2_page_capacity(int sc);

// S135-1B: Get size class from page header (safe boundary crossing)
// Returns: 1 on success (out_sc set), 0 on failure (invalid magic/bounds)
int hz3_small_v2_page_try_get_sc(void* page_base, uint8_t* out_sc);

// S135-1C: Get tail waste per page for size class (safe boundary crossing)
// Returns tail waste in bytes (0 if SC invalid)
size_t hz3_small_v2_page_tail_waste(int sc);

// Task 3: Fast free using page_hdr only (no seg_hdr read)
// page_hdr is void* to hide internal Hz3SmallV2PageHdr type
void  hz3_small_v2_free_fast(void* ptr, void* page_hdr);

// P4: fast local free using tag-decoded (sc, owner) - no page_hdr read
static inline void hz3_small_v2_free_local_fast(void* ptr, int sc) {
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("small_v2:free_local", ptr, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_S69_LIVECOUNT
    hz3_s69_live_count_dec(ptr);  // Decrement before push to bin
#endif
#if HZ3_S72_BOUNDARY_DEBUG
    hz3_small_v2_boundary_check_obj("small_v2:free_local", t_hz3_cache.my_shard, sc, ptr);
#endif
#if HZ3_S62_STALE_FAILFAST
    hz3_s62_stale_check_ptr(ptr, "small_free_local", sc);
#endif
#if HZ3_TCACHE_SOA
    hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_small(sc)), ptr);
#else
    Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
    hz3_small_bin_push(bin, ptr);
#endif
}

// S12-4C: Fast free using tag-decoded (sc, owner) - no page_hdr read
// Called from hot path when PTAG is enabled
static inline void hz3_small_v2_free_by_tag(void* ptr, int sc, int owner) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)ptr;
    (void)sc;
    (void)owner;
#else
    if (!ptr) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

#if HZ3_S62_STALE_FAILFAST
    // Check before any push (local or remote)
    hz3_s62_stale_check_ptr(ptr, "small_free_by_tag", sc);
#endif
    hz3_tcache_ensure_init();
    if (__builtin_expect((uint8_t)owner == t_hz3_cache.my_shard, 1)) {
        // Note: free_local_fast also has check, but redundant check is acceptable for debug
        hz3_small_v2_free_local_fast(ptr, sc);
        return;
    }
    hz3_small_v2_free_remote_slow(ptr, sc, owner);
#endif
}
