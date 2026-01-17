#include "hz3_arena.h"

#if HZ3_S79_PTAG32_STORE_TRACE || HZ3_S88_PTAG32_CLEAR_TRACE
#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HZ3_S79_PTAG32_STORE_TRACE
static _Atomic int g_s79_ptag32_trace_fired = 0;
#endif
#if HZ3_S92_PTAG32_STORE_LAST
static _Atomic(uint32_t) g_s92_last_store_page_idx = 0;
static _Atomic(uint32_t) g_s92_last_store_old_tag = 0;
static _Atomic(uint32_t) g_s92_last_store_new_tag = 0;
static _Atomic(uintptr_t) g_s92_last_store_ra = 0;
static _Atomic(uint64_t) g_s92_last_store_seq = 0;
#endif
#if HZ3_S88_PTAG32_CLEAR_TRACE
static _Atomic int g_s88_ptag32_clear_fired = 0;
static _Atomic(uint32_t) g_s88_last_clear_page_idx = 0;
static _Atomic(uint32_t) g_s88_last_clear_old_tag = 0;
static _Atomic(uintptr_t) g_s88_last_clear_ra = 0;
static _Atomic(uint64_t) g_s88_last_clear_seq = 0;
#endif

static uintptr_t hz3_s79_find_so_base(void) {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libhakozuna_hz3") == NULL) {
            continue;
        }
        char* dash = strchr(line, '-');
        if (!dash) {
            continue;
        }
        *dash = '\0';
        uintptr_t base = (uintptr_t)strtoull(line, NULL, 16);
        fclose(f);
        return base;
    }
    fclose(f);
    return 0;
}

#if HZ3_S92_PTAG32_STORE_LAST
static pthread_once_t g_s92_so_base_once = PTHREAD_ONCE_INIT;
static uintptr_t g_s92_so_base = 0;

static void hz3_s92_so_base_init_once(void) {
    g_s92_so_base = hz3_s79_find_so_base();
}

static inline uintptr_t hz3_s92_get_so_base(void) {
    pthread_once(&g_s92_so_base_once, hz3_s92_so_base_init_once);
    return g_s92_so_base;
}

void hz3_ptag32_store_last_capture(uint32_t page_idx, uint32_t old_tag, uint32_t new_tag, void* ra) {
    atomic_store_explicit(&g_s92_last_store_page_idx, page_idx, memory_order_relaxed);
    atomic_store_explicit(&g_s92_last_store_old_tag, old_tag, memory_order_relaxed);
    atomic_store_explicit(&g_s92_last_store_new_tag, new_tag, memory_order_relaxed);
    atomic_store_explicit(&g_s92_last_store_ra, (uintptr_t)ra, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s92_last_store_seq, 1, memory_order_release);
}

int hz3_ptag32_store_last_snapshot(uint64_t* seq_out,
                                  uint32_t* page_idx_out,
                                  uint32_t* old_tag_out,
                                  uint32_t* new_tag_out,
                                  void** ra_out,
                                  uintptr_t* so_base_out,
                                  unsigned long* off_out)
{
    if (seq_out) *seq_out = 0;
    if (page_idx_out) *page_idx_out = 0;
    if (old_tag_out) *old_tag_out = 0;
    if (new_tag_out) *new_tag_out = 0;
    if (ra_out) *ra_out = NULL;
    if (so_base_out) *so_base_out = 0;
    if (off_out) *off_out = 0;

    uint64_t seq1 = atomic_load_explicit(&g_s92_last_store_seq, memory_order_acquire);
    if (seq1 == 0) {
        return 0;
    }

    uint32_t page_idx = atomic_load_explicit(&g_s92_last_store_page_idx, memory_order_relaxed);
    uint32_t old_tag = atomic_load_explicit(&g_s92_last_store_old_tag, memory_order_relaxed);
    uint32_t new_tag = atomic_load_explicit(&g_s92_last_store_new_tag, memory_order_relaxed);
    uintptr_t ra_u = atomic_load_explicit(&g_s92_last_store_ra, memory_order_relaxed);

    uint64_t seq2 = atomic_load_explicit(&g_s92_last_store_seq, memory_order_acquire);
    if (seq2 != seq1) {
        seq1 = seq2;
        page_idx = atomic_load_explicit(&g_s92_last_store_page_idx, memory_order_relaxed);
        old_tag = atomic_load_explicit(&g_s92_last_store_old_tag, memory_order_relaxed);
        new_tag = atomic_load_explicit(&g_s92_last_store_new_tag, memory_order_relaxed);
        ra_u = atomic_load_explicit(&g_s92_last_store_ra, memory_order_relaxed);
        seq2 = atomic_load_explicit(&g_s92_last_store_seq, memory_order_acquire);
        seq1 = seq2;
    }

    void* ra = (void*)ra_u;
    uintptr_t so_base = hz3_s92_get_so_base();
    unsigned long off = 0;
    if (so_base && ra) {
        off = (unsigned long)((uintptr_t)ra - so_base);
    }

    if (seq_out) *seq_out = seq1;
    if (page_idx_out) *page_idx_out = page_idx;
    if (old_tag_out) *old_tag_out = old_tag;
    if (new_tag_out) *new_tag_out = new_tag;
    if (ra_out) *ra_out = ra;
    if (so_base_out) *so_base_out = so_base;
    if (off_out) *off_out = off;
    return 1;
}
#endif

#if HZ3_S88_PTAG32_CLEAR_TRACE
static pthread_once_t g_s88_so_base_once = PTHREAD_ONCE_INIT;
static uintptr_t g_s88_so_base = 0;

static void hz3_s88_so_base_init_once(void) {
    g_s88_so_base = hz3_s79_find_so_base();
}

static inline uintptr_t hz3_s88_get_so_base(void) {
    pthread_once(&g_s88_so_base_once, hz3_s88_so_base_init_once);
    return g_s88_so_base;
}

#if HZ3_S98_PTAG32_CLEAR_MAP
#if HZ3_S98_PTAG32_CLEAR_MAP_LOG2 <= HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2
#error "S98: HZ3_S98_PTAG32_CLEAR_MAP_LOG2 must be > HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2"
#endif

typedef struct {
    uint32_t key;      // page_idx+1 (0=empty)
    uint32_t old_tag;  // old tag32 before clear
    uintptr_t ra;      // return address
    uint64_t seq;      // monotonically increasing sequence
} Hz3S98Entry;

typedef struct {
    Hz3S98Entry* map;
    size_t cap;
    size_t mask;
    pthread_mutex_t lock;
} Hz3S98Shard;

static pthread_once_t g_s98_once = PTHREAD_ONCE_INIT;
static Hz3S98Shard* g_s98_shards = NULL;
static size_t g_s98_shard_count = 0;
static size_t g_s98_shard_mask = 0;
static _Atomic(uint64_t) g_s98_seq = 0;

static inline size_t s98_hash_u32(uint32_t x) {
    // 32-bit mix (cheap)
    uint32_t h = x;
    h ^= h >> 16;
    h *= 0x7feb352dU;
    h ^= h >> 15;
    h *= 0x846ca68bU;
    h ^= h >> 16;
    return (size_t)h;
}

static void hz3_s98_init_once(void) {
    size_t total_cap = (size_t)1u << (unsigned)HZ3_S98_PTAG32_CLEAR_MAP_LOG2;
    size_t shard_count = (size_t)1u << (unsigned)HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2;
    size_t shard_cap = total_cap >> (unsigned)HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2;

    Hz3S98Shard* shards = (Hz3S98Shard*)calloc(shard_count, sizeof(Hz3S98Shard));
    Hz3S98Entry* storage = (Hz3S98Entry*)calloc(total_cap, sizeof(Hz3S98Entry));
    if (!shards || !storage) {
        free(storage);
        free(shards);
        return;
    }

    for (size_t i = 0; i < shard_count; i++) {
        shards[i].map = storage + (i * shard_cap);
        shards[i].cap = shard_cap;
        shards[i].mask = shard_cap - 1;
        pthread_mutex_init(&shards[i].lock, NULL);
    }

    g_s98_shards = shards;
    g_s98_shard_count = shard_count;
    g_s98_shard_mask = shard_count - 1;
}

static inline Hz3S98Shard* s98_pick_shard(uint32_t page_idx, size_t* idx0) {
    size_t h = s98_hash_u32(page_idx);
    size_t shard_idx = h & g_s98_shard_mask;
    Hz3S98Shard* shard = &g_s98_shards[shard_idx];
    *idx0 = (h >> (unsigned)HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2) & shard->mask;
    return shard;
}

static inline void hz3_s98_put(uint32_t page_idx, uint32_t old_tag, uintptr_t ra) {
    pthread_once(&g_s98_once, hz3_s98_init_once);
    if (!g_s98_shards) {
        return;
    }
    uint32_t key = page_idx + 1u;
    size_t idx0;
    Hz3S98Shard* shard = s98_pick_shard(page_idx, &idx0);
    pthread_mutex_lock(&shard->lock);
    size_t idx = idx0;
    for (size_t probe = 0; probe < shard->cap; probe++) {
        uint32_t cur = shard->map[idx].key;
        if (cur == 0 || cur == key) {
            uint64_t seq = atomic_fetch_add_explicit(&g_s98_seq, 1, memory_order_relaxed) + 1;
            shard->map[idx].key = key;
            shard->map[idx].old_tag = old_tag;
            shard->map[idx].ra = ra;
            shard->map[idx].seq = seq;
            pthread_mutex_unlock(&shard->lock);
            return;
        }
        idx = (idx + 1) & shard->mask;
    }
    // Full: overwrite starting slot (evict).
    {
        uint64_t seq = atomic_fetch_add_explicit(&g_s98_seq, 1, memory_order_relaxed) + 1;
        size_t evict_idx = idx0 & shard->mask;
        shard->map[evict_idx].key = key;
        shard->map[evict_idx].old_tag = old_tag;
        shard->map[evict_idx].ra = ra;
        shard->map[evict_idx].seq = seq;
    }
    pthread_mutex_unlock(&shard->lock);
}

int hz3_ptag32_clear_map_lookup(uint32_t page_idx,
                                uint64_t* seq_out,
                                uint32_t* old_tag_out,
                                void** ra_out,
                                uintptr_t* so_base_out,
                                unsigned long* off_out)
{
    if (seq_out) *seq_out = 0;
    if (old_tag_out) *old_tag_out = 0;
    if (ra_out) *ra_out = NULL;
    if (so_base_out) *so_base_out = 0;
    if (off_out) *off_out = 0;

    pthread_once(&g_s98_once, hz3_s98_init_once);
    if (!g_s98_shards) {
        return 0;
    }
    uint32_t key = page_idx + 1u;
    size_t idx0;
    Hz3S98Shard* shard = s98_pick_shard(page_idx, &idx0);
    pthread_mutex_lock(&shard->lock);
    size_t idx = idx0;
    for (size_t probe = 0; probe < shard->cap; probe++) {
        uint32_t cur = shard->map[idx].key;
        if (cur == 0) {
            pthread_mutex_unlock(&shard->lock);
            return 0;
        }
        if (cur == key) {
            uint32_t old_tag = shard->map[idx].old_tag;
            uintptr_t ra_u = shard->map[idx].ra;
            uint64_t seq = shard->map[idx].seq;
            pthread_mutex_unlock(&shard->lock);

            void* ra = (void*)ra_u;
            uintptr_t so_base = hz3_s88_get_so_base();
            unsigned long off = 0;
            if (so_base && ra) {
                off = (unsigned long)((uintptr_t)ra - so_base);
            }
            if (seq_out) *seq_out = seq;
            if (old_tag_out) *old_tag_out = old_tag;
            if (ra_out) *ra_out = ra;
            if (so_base_out) *so_base_out = so_base;
            if (off_out) *off_out = off;
            return 1;
        }
        idx = (idx + 1) & shard->mask;
    }
    pthread_mutex_unlock(&shard->lock);
    return 0;
}
#endif  // HZ3_S98_PTAG32_CLEAR_MAP

int hz3_ptag32_clear_last_snapshot(uint64_t* seq_out,
                                  uint32_t* page_idx_out,
                                  uint32_t* old_tag_out,
                                  void** ra_out,
                                  uintptr_t* so_base_out,
                                  unsigned long* off_out)
{
    if (seq_out) *seq_out = 0;
    if (page_idx_out) *page_idx_out = 0;
    if (old_tag_out) *old_tag_out = 0;
    if (ra_out) *ra_out = NULL;
    if (so_base_out) *so_base_out = 0;
    if (off_out) *off_out = 0;

    // Best-effort consistent snapshot via seq double-read.
    uint64_t seq1 = atomic_load_explicit(&g_s88_last_clear_seq, memory_order_acquire);
    if (seq1 == 0) {
        return 0;
    }

    uint32_t page_idx = atomic_load_explicit(&g_s88_last_clear_page_idx, memory_order_relaxed);
    uint32_t old_tag = atomic_load_explicit(&g_s88_last_clear_old_tag, memory_order_relaxed);
    uintptr_t ra_u = atomic_load_explicit(&g_s88_last_clear_ra, memory_order_relaxed);

    uint64_t seq2 = atomic_load_explicit(&g_s88_last_clear_seq, memory_order_acquire);
    if (seq2 != seq1) {
        // One retry is enough for debug.
        seq1 = seq2;
        page_idx = atomic_load_explicit(&g_s88_last_clear_page_idx, memory_order_relaxed);
        old_tag = atomic_load_explicit(&g_s88_last_clear_old_tag, memory_order_relaxed);
        ra_u = atomic_load_explicit(&g_s88_last_clear_ra, memory_order_relaxed);
        seq2 = atomic_load_explicit(&g_s88_last_clear_seq, memory_order_acquire);
        seq1 = seq2;
    }

    void* ra = (void*)ra_u;
    uintptr_t so_base = hz3_s88_get_so_base();
    unsigned long off = 0;
    if (so_base && ra) {
        off = (unsigned long)((uintptr_t)ra - so_base);
    }

    if (seq_out) *seq_out = seq1;
    if (page_idx_out) *page_idx_out = page_idx;
    if (old_tag_out) *old_tag_out = old_tag;
    if (ra_out) *ra_out = ra;
    if (so_base_out) *so_base_out = so_base;
    if (off_out) *off_out = off;
    return 1;
}
#endif

#if HZ3_S79_PTAG32_STORE_TRACE
void hz3_ptag32_store_trace(uint32_t page_idx, uint32_t old_tag, uint32_t new_tag, const char* where) {
    if (HZ3_S79_PTAG32_STORE_SHOT &&
        atomic_exchange_explicit(&g_s79_ptag32_trace_fired, 1, memory_order_acq_rel) != 0) {
        return;
    }

    uint32_t old_bin = hz3_pagetag32_bin(old_tag);
    uint8_t old_dst = hz3_pagetag32_dst(old_tag);
    uint32_t new_bin = hz3_pagetag32_bin(new_tag);
    uint8_t new_dst = hz3_pagetag32_dst(new_tag);
    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
    uintptr_t so_base = hz3_s79_find_so_base();
    unsigned long ra_off = 0;
    if (so_base && ra0) {
        ra_off = (unsigned long)((uintptr_t)ra0 - so_base);
    }

    fprintf(stderr,
            "[HZ3_S79_PTAG32_STORE] where=%s page_idx=%u old=0x%08x new=0x%08x "
            "old_bin=%u old_dst=%u new_bin=%u new_dst=%u ra=%p base=%p off=0x%lx "
            "sub4k=%d medium_base=%u bin_total=%u\n",
            (where ? where : "?"),
            page_idx,
            old_tag,
            new_tag,
            old_bin,
            (unsigned)old_dst,
            new_bin,
            (unsigned)new_dst,
            ra0,
            (void*)so_base,
            ra_off,
            (int)HZ3_SUB4K_ENABLE,
            (unsigned)HZ3_MEDIUM_BIN_BASE,
            (unsigned)HZ3_BIN_TOTAL);

    if (HZ3_S79_PTAG32_STORE_FAILFAST) {
        abort();
    }
}
#endif

#if HZ3_S88_PTAG32_CLEAR_TRACE
	void hz3_ptag32_clear_trace(uint32_t page_idx, uint32_t old_tag, const char* where) {
	    // Always capture the latest clear event (SSOT: S72 prints this on failure).
	    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
    atomic_store_explicit(&g_s88_last_clear_page_idx, page_idx, memory_order_relaxed);
    atomic_store_explicit(&g_s88_last_clear_old_tag, old_tag, memory_order_relaxed);
    atomic_store_explicit(&g_s88_last_clear_ra, (uintptr_t)ra0, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s88_last_clear_seq, 1, memory_order_release);

#if HZ3_S98_PTAG32_CLEAR_MAP
    hz3_s98_put(page_idx, old_tag, (uintptr_t)ra0);
#endif

    if (HZ3_S88_PTAG32_CLEAR_SHOT &&
        atomic_exchange_explicit(&g_s88_ptag32_clear_fired, 1, memory_order_acq_rel) != 0) {
        return;
    }

	    uint32_t old_bin = hz3_pagetag32_bin(old_tag);
	    uint8_t old_dst = hz3_pagetag32_dst(old_tag);
	    uintptr_t so_base = hz3_s79_find_so_base();
	    unsigned long ra_off = 0;
	    if (so_base && ra0) {
	        ra_off = (unsigned long)((uintptr_t)ra0 - so_base);
	    }
	    void* arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
	    void* page_addr = NULL;
	    if (arena_base) {
	        page_addr = (void*)((uintptr_t)arena_base + ((uintptr_t)page_idx << HZ3_ARENA_PAGE_SHIFT));
	    }

	    fprintf(stderr,
	            "[HZ3_S88_PTAG32_CLEAR] where=%s page_idx=%u old=0x%08x old_bin=%u old_dst=%u "
	            "page_addr=%p arena_base=%p ra=%p base=%p off=0x%lx sub4k=%d medium_base=%u bin_total=%u\n",
	            (where ? where : "?"),
	            page_idx,
	            old_tag,
	            old_bin,
	            (unsigned)old_dst,
	            page_addr,
	            arena_base,
	            ra0,
	            (void*)so_base,
	            ra_off,
	            (int)HZ3_SUB4K_ENABLE,
            (unsigned)HZ3_MEDIUM_BIN_BASE,
            (unsigned)HZ3_BIN_TOTAL);

    if (HZ3_S88_PTAG32_CLEAR_FAILFAST) {
        abort();
    }
}
#endif
#endif
