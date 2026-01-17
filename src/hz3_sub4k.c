#include "hz3_sub4k.h"

#include "hz3_arena.h"
#include "hz3_central_bin.h"
#include "hz3_list_util.h"
#include "hz3_owner_excl.h"
#include "hz3_owner_lease.h"
#include "hz3_seg_hdr.h"
#include "hz3_segment.h"
#include "hz3_tcache.h"
#include "hz3_watch_ptr.h"

#include <pthread.h>

#if !HZ3_SUB4K_ENABLE
int hz3_sub4k_sc_from_size(size_t size) {
    (void)size;
    return -1;
}

size_t hz3_sub4k_sc_to_size(int sc) {
    (void)sc;
    return 0;
}

void* hz3_sub4k_alloc(size_t size) {
    (void)size;
    return NULL;
}

void hz3_sub4k_bin_flush(int sc, Hz3Bin* bin) {
    (void)sc;
    (void)bin;
}

void hz3_sub4k_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    (void)owner;
    (void)sc;
    (void)head;
    (void)tail;
    (void)n;
}

void hz3_sub4k_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n) {
    (void)shard;
    (void)sc;
    (void)head;
    (void)tail;
    (void)n;
}

// S62-1b stubs (when SUB4K disabled)
uint32_t hz3_sub4k_central_pop_batch_ext(uint8_t shard, int sc, void** out, uint32_t want) {
    (void)shard;
    (void)sc;
    (void)out;
    (void)want;
    return 0;
}

void hz3_sub4k_central_push_list_ext(uint8_t shard, int sc, void* head, void* tail, uint32_t n) {
    (void)shard;
    (void)sc;
    (void)head;
    (void)tail;
    (void)n;
}

size_t hz3_sub4k_run_capacity(int sc) {
    (void)sc;
    return 0;
}

#else

#define HZ3_SUB4K_REFILL_BATCH 8

static const uint16_t g_hz3_sub4k_sizes[HZ3_SUB4K_NUM_SC] = {
    2304, 2816, 3328, 3840
};
_Static_assert(HZ3_SUB4K_NUM_SC == 4, "update g_hz3_sub4k_sizes for new class count");

int hz3_sub4k_sc_from_size(size_t size) {
    if (size <= HZ3_SMALL_MAX_SIZE || size > HZ3_SUB4K_MAX_SIZE) {
        return -1;
    }
    size_t aligned = (size + (HZ3_SMALL_ALIGN - 1u)) & ~(size_t)(HZ3_SMALL_ALIGN - 1u);
    if (aligned <= g_hz3_sub4k_sizes[0]) return 0;
    if (aligned <= g_hz3_sub4k_sizes[1]) return 1;
    if (aligned <= g_hz3_sub4k_sizes[2]) return 2;
    if (aligned <= g_hz3_sub4k_sizes[3]) return 3;
    return -1;
}

size_t hz3_sub4k_sc_to_size(int sc) {
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) {
        return 0;
    }
    return (size_t)g_hz3_sub4k_sizes[sc];
}

static pthread_once_t g_hz3_sub4k_central_once = PTHREAD_ONCE_INIT;
static Hz3CentralBin g_hz3_sub4k_central[HZ3_NUM_SHARDS][HZ3_SUB4K_NUM_SC];

static void hz3_sub4k_central_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_SUB4K_NUM_SC; sc++) {
            hz3_central_bin_init(&g_hz3_sub4k_central[shard][sc]);
        }
    }
}

static inline void hz3_sub4k_central_init(void) {
    pthread_once(&g_hz3_sub4k_central_once, hz3_sub4k_central_do_init);
}

void hz3_sub4k_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n) {
    hz3_central_bin_push_list(&g_hz3_sub4k_central[shard][sc], head, tail, n);
}

static inline int hz3_sub4k_central_pop_batch(uint8_t shard, int sc, void** out, int want) {
    return hz3_central_bin_pop_batch(&g_hz3_sub4k_central[shard][sc], out, want);
}

static inline int hz3_sub4k_collision_active(void) {
#if HZ3_LANE_SPLIT
    return (hz3_shard_live_count(t_hz3_cache.my_shard) > 1);
#else
    return 0;
#endif
}

static inline Hz3SegHdr* hz3_sub4k_current_hdr_get(int collision_active) {
#if HZ3_LANE_SPLIT
    if (collision_active) {
        Hz3SegHdr* hdr = t_hz3_cache.small_current_seg_base
                           ? hz3_seg_from_ptr(t_hz3_cache.small_current_seg_base)
                           : NULL;
        if (!hdr) {
            return NULL;
        }
        if (hdr->owner != t_hz3_cache.my_shard) {
            return NULL;
        }
        return hdr;
    }
#endif
    (void)collision_active;
    return t_hz3_cache.small_current_seg;
}

static inline void hz3_sub4k_current_hdr_set(Hz3SegHdr* hdr, int collision_active) {
#if HZ3_LANE_SPLIT
    t_hz3_cache.small_current_seg_base = hdr ? (void*)hdr : NULL;
    if (collision_active) {
        t_hz3_cache.small_current_seg = NULL;
        return;
    }
#endif
    (void)collision_active;
    t_hz3_cache.small_current_seg = hdr;
}

static void* hz3_sub4k_alloc_run(int sc) {
    (void)sc;
    // CollisionGuardBox: small segment header (free_bits/free_pages) is "my_shard-only".
    // When shard collision is tolerated, multiple writers can race here.
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(t_hz3_cache.my_shard);
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(t_hz3_cache.my_shard);
    int collision_active = hz3_sub4k_collision_active();

    Hz3SegHdr* hdr = hz3_sub4k_current_hdr_get(collision_active);
    if (!hdr || hdr->free_pages < 2) {
        hdr = hz3_seg_alloc_small(t_hz3_cache.my_shard);
        if (!hdr) {
            hz3_owner_excl_release(excl_token);
            hz3_owner_lease_release(lease_token);
            return NULL;
        }
        hz3_sub4k_current_hdr_set(hdr, collision_active);
    }

    int start_page = hz3_bitmap_find_free(hdr->free_bits, 2);
    if (start_page < 0) {
        hdr = hz3_seg_alloc_small(t_hz3_cache.my_shard);
        if (!hdr) {
            hz3_owner_excl_release(excl_token);
            hz3_owner_lease_release(lease_token);
            return NULL;
        }
        hz3_sub4k_current_hdr_set(hdr, collision_active);
        start_page = hz3_bitmap_find_free(hdr->free_bits, 2);
        if (start_page < 0) {
            hz3_owner_excl_release(excl_token);
            hz3_owner_lease_release(lease_token);
            return NULL;
        }
    }

    hz3_bitmap_mark_used(hdr->free_bits, (size_t)start_page, 2);
    if (hdr->free_pages >= 2) {
        hdr->free_pages -= 2;
    } else {
        hdr->free_pages = 0;
    }

    // S62-1b: Mark run_start in sub4k_run_start_bits
    {
        size_t word = (size_t)start_page / 64;
        size_t bit = (size_t)start_page % 64;
        hdr->sub4k_run_start_bits[word] |= (1ULL << bit);
    }

    char* run_base = (char*)hdr + ((size_t)start_page << HZ3_PAGE_SHIFT);

    uint32_t page_idx;
    if (g_hz3_page_tag32 && hz3_arena_page_index_fast(run_base, &page_idx)) {
        int bin = hz3_bin_index_sub4k(sc);
        uint32_t tag32 = hz3_pagetag32_encode(bin, (int)hdr->owner);
        hz3_pagetag32_store(page_idx, tag32);
        hz3_pagetag32_store(page_idx + 1u, tag32);
    }

#if HZ3_S110_META_ENABLE
    // S110: store bin+1 in segment header per-page metadata (2 pages for sub4k run)
    {
        int bin = hz3_bin_index_sub4k(sc);
        uint16_t tag = (uint16_t)(bin + 1);
        atomic_store_explicit(&hdr->page_bin_plus1[start_page], tag, memory_order_release);
        atomic_store_explicit(&hdr->page_bin_plus1[start_page + 1], tag, memory_order_release);
    }
#endif

    hz3_owner_excl_release(excl_token);
    hz3_owner_lease_release(lease_token);
    return run_base;
}

// SoA: fill bin using binref
#if HZ3_TCACHE_SOA
static void hz3_sub4k_fill_binref(Hz3BinRef binref, int sc, void* run_base) {
    size_t obj_size = hz3_sub4k_sc_to_size(sc);
    size_t run_bytes = HZ3_PAGE_SIZE * 2u;
    size_t count = run_bytes / obj_size;
    if (count == 0) {
        return;
    }

    char* cur = (char*)run_base;
    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    hz3_binref_prepend_list(binref, run_base, cur, (uint32_t)count);
}
#else
static void hz3_sub4k_fill_bin(Hz3Bin* bin, int sc, void* run_base) {
    size_t obj_size = hz3_sub4k_sc_to_size(sc);
    size_t run_bytes = HZ3_PAGE_SIZE * 2u;
    size_t count = run_bytes / obj_size;
    if (count == 0) {
        return;
    }

    char* cur = (char*)run_base;
    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    hz3_bin_prepend_list(bin, run_base, cur, (uint32_t)count);
}
#endif

void* hz3_sub4k_alloc(size_t size) {
    int sc = hz3_sub4k_sc_from_size(size);
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) {
        return NULL;
    }

    hz3_tcache_ensure_init();

#if HZ3_TCACHE_SOA
    Hz3BinRef binref = hz3_tcache_get_local_binref(hz3_bin_index_sub4k(sc));
    void* obj = hz3_binref_pop(binref);
    if (obj) {
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("sub4k:alloc_local", obj, sc, t_hz3_cache.my_shard);
#endif
        return obj;
    }

    hz3_sub4k_central_init();
    void* batch[HZ3_SUB4K_REFILL_BATCH];
    int got = hz3_sub4k_central_pop_batch(t_hz3_cache.my_shard, sc, batch, HZ3_SUB4K_REFILL_BATCH);
    if (got > 0) {
        for (int i = 1; i < got; i++) {
            hz3_binref_push(binref, batch[i]);
        }
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("sub4k:alloc_central", batch[0], sc, t_hz3_cache.my_shard);
#endif
        return batch[0];
    }

    void* run = hz3_sub4k_alloc_run(sc);
    if (!run) {
        return NULL;
    }

    hz3_sub4k_fill_binref(binref, sc, run);
    obj = hz3_binref_pop(binref);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("sub4k:alloc_run", obj, sc, t_hz3_cache.my_shard);
#endif
    return obj;
#else
    Hz3Bin* bin = hz3_tcache_get_sub4k_bin(sc);
    void* obj = hz3_bin_pop(bin);
    if (obj) {
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("sub4k:alloc_local", obj, sc, t_hz3_cache.my_shard);
#endif
        return obj;
    }

    hz3_sub4k_central_init();
    void* batch[HZ3_SUB4K_REFILL_BATCH];
    int got = hz3_sub4k_central_pop_batch(t_hz3_cache.my_shard, sc, batch, HZ3_SUB4K_REFILL_BATCH);
    if (got > 0) {
        for (int i = 1; i < got; i++) {
            hz3_bin_push(bin, batch[i]);
        }
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("sub4k:alloc_central", batch[0], sc, t_hz3_cache.my_shard);
#endif
        return batch[0];
    }

    void* run = hz3_sub4k_alloc_run(sc);
    if (!run) {
        return NULL;
    }

    hz3_sub4k_fill_bin(bin, sc, run);
    obj = hz3_bin_pop(bin);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("sub4k:alloc_run", obj, sc, t_hz3_cache.my_shard);
#endif
    return obj;
#endif
}

void hz3_sub4k_bin_flush(int sc, Hz3Bin* bin) {
    if (!bin || hz3_bin_is_empty(bin)) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) {
        return;
    }

    hz3_sub4k_central_init();

    void* head = hz3_bin_take_all(bin);
    void* tail;
    uint32_t n;
    hz3_list_find_tail(head, &tail, &n);

    hz3_sub4k_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
}

void hz3_sub4k_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) {
        return;
    }
    hz3_sub4k_central_init();
    hz3_sub4k_central_push_list(owner, sc, head, tail, n);
}

// S62-1b: External APIs for sub4k retire (with validation guards)
uint32_t hz3_sub4k_central_pop_batch_ext(uint8_t shard, int sc, void** out, uint32_t want) {
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) return 0;
    if (shard >= HZ3_NUM_SHARDS) return 0;
    hz3_sub4k_central_init();
    uint32_t got = (uint32_t)hz3_sub4k_central_pop_batch(shard, sc, out, (int)want);
    return got;
}

void hz3_sub4k_central_push_list_ext(uint8_t shard, int sc, void* head, void* tail, uint32_t n) {
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) return;
    if (shard >= HZ3_NUM_SHARDS) return;
    if (!head || !tail || n == 0) return;
    hz3_sub4k_central_init();
    hz3_sub4k_central_push_list(shard, sc, head, tail, n);
}

size_t hz3_sub4k_run_capacity(int sc) {
    if (sc < 0 || sc >= HZ3_SUB4K_NUM_SC) return 0;
    size_t obj_size = hz3_sub4k_sc_to_size(sc);
    size_t run_bytes = HZ3_PAGE_SIZE * 2u;  // 8KB
    return run_bytes / obj_size;
}

#endif
