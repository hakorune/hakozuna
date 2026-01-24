#include "hz3_central_shadow.h"

#if HZ3_S86_CENTRAL_SHADOW

#include "hz3_arena.h"
#include "hz3_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>

#define SHADOW_KEY_EMPTY ((uintptr_t)0)
#define SHADOW_KEY_TOMB  ((uintptr_t)1)

#ifndef HZ3_S86_CENTRAL_SHADOW_LOG2
// Default: 16M entries (~384MiB at 24 bytes/entry) for long mstress runs.
// Override at build-time if needed (e.g., -DHZ3_S86_CENTRAL_SHADOW_LOG2=22).
#define HZ3_S86_CENTRAL_SHADOW_LOG2 24
#endif

#ifndef HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2
// Reduce contention by sharding the shadow map locks.
// 2^6=64 shards is enough to make 4-32 threads usable.
#define HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2 6
#endif

#if HZ3_S86_CENTRAL_SHADOW_LOG2 <= HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2
#error "S86: HZ3_S86_CENTRAL_SHADOW_LOG2 must be > HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2"
#endif

typedef struct {
    uintptr_t key;       // Key: object pointer (0=empty, 1=tombstone)
    void* expected_next; // Value: expected next pointer
    void* first_ra;      // S86 enhanced: return address from first push
} ShadowEntry;

static hz3_once_t g_shadow_once = HZ3_ONCE_INIT;
static atomic_int g_shadow_disabled = 0;
static atomic_int g_shadow_count = 0;

typedef struct {
    ShadowEntry* map;
    size_t cap;
    size_t mask;
    hz3_lock_t lock;
} ShadowShard;

static ShadowEntry* g_shadow_storage = NULL;
static ShadowShard* g_shadow_shards = NULL;
static size_t g_shadow_shard_count = 0;
static size_t g_shadow_shard_mask = 0;
static size_t g_shadow_total_cap = 0;

// Cached so_base (initialized once via hz3_once)
static hz3_once_t g_so_base_once = HZ3_ONCE_INIT;
static uintptr_t g_so_base = 0;

static inline unsigned long hz3_s86_tid(void) {
    return hz3_thread_id();
}

static uintptr_t hz3_s86_find_so_base_impl(void) {
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

static void hz3_s86_so_base_init_once(void) {
    g_so_base = hz3_s86_find_so_base_impl();
}

// Get cached so_base (initialize on first call)
static inline uintptr_t hz3_s86_get_so_base(void) {
    hz3_once(&g_so_base_once, hz3_s86_so_base_init_once);
    return g_so_base;
}

static inline unsigned long hz3_s86_ra_to_off(void* ra) {
    uintptr_t base = hz3_s86_get_so_base();
    if (base && ra) {
        return (unsigned long)((uintptr_t)ra - base);
    }
    return 0;
}

static void hz3_s86_shadow_init_once(void) {
    size_t total_cap = (size_t)1u << (unsigned)HZ3_S86_CENTRAL_SHADOW_LOG2;
    size_t shard_count = (size_t)1u << (unsigned)HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2;
    size_t shard_cap = total_cap >> (unsigned)HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2;

    ShadowShard* shards = (ShadowShard*)calloc(shard_count, sizeof(ShadowShard));
    ShadowEntry* storage = (ShadowEntry*)calloc(total_cap, sizeof(ShadowEntry));
    if (!shards || !storage) {
        atomic_store_explicit(&g_shadow_disabled, 1, memory_order_relaxed);
        fprintf(stderr,
                "[HZ3_S86_CENTRAL_SHADOW] where=calloc_failed (shadow disabled) "
                "log2=%u shards_log2=%u entries=%zu bytes=%zu\n",
                (unsigned)HZ3_S86_CENTRAL_SHADOW_LOG2,
                (unsigned)HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2,
                total_cap,
                total_cap * sizeof(ShadowEntry) + shard_count * sizeof(ShadowShard));
        free(storage);
        free(shards);
        return;
    }

    for (size_t i = 0; i < shard_count; i++) {
        ShadowShard* s = &shards[i];
        s->map = storage + (i * shard_cap);
        s->cap = shard_cap;
        s->mask = shard_cap - 1;
        hz3_lock_init(&s->lock);
    }

    g_shadow_storage = storage;
    g_shadow_shards = shards;
    g_shadow_shard_count = shard_count;
    g_shadow_shard_mask = shard_count - 1;
    g_shadow_total_cap = total_cap;

    // Initialize so_base cache now
    (void)hz3_s86_get_so_base();

    fprintf(stderr,
            "[HZ3_S86_CENTRAL_SHADOW] where=init log2=%u shards_log2=%u verify_pop=%d entries=%zu bytes=%zu\n",
            (unsigned)HZ3_S86_CENTRAL_SHADOW_LOG2,
            (unsigned)HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2,
            (int)HZ3_S86_CENTRAL_SHADOW_VERIFY_POP,
            total_cap,
            total_cap * sizeof(ShadowEntry) + shard_count * sizeof(ShadowShard));
}

// Log duplicate push with BOTH first and second push offsets
static inline void hz3_s86_log_dup(void* obj, void* existing_next, void* new_next,
                                    void* first_ra, void* second_ra) {
    uintptr_t so_base = hz3_s86_get_so_base();
    unsigned long first_off = hz3_s86_ra_to_off(first_ra);
    unsigned long second_off = hz3_s86_ra_to_off(second_ra);

    uint32_t page_idx = 0;
    uint32_t tag32 = 0;
    uint32_t bin = 0;
    uint8_t dst = 0;
    int have_tag = 0;
    if (g_hz3_arena_base && g_hz3_page_tag32 && hz3_arena_page_index_fast(obj, &page_idx)) {
        tag32 = hz3_pagetag32_load(page_idx);
        if (tag32 != 0) {
            have_tag = 1;
            bin = hz3_pagetag32_bin(tag32);
            dst = hz3_pagetag32_dst(tag32);
        }
    }

    if (have_tag) {
        fprintf(stderr,
                "[HZ3_S86_CENTRAL_SHADOW] where=push_dup tid=0x%lx obj=%p "
                "first_push_off=0x%lx second_push_off=0x%lx "
                "existing_next=%p new_next=%p "
                "page_idx=%u tag32=0x%08x bin=%u dst=%u base=%p\n",
                hz3_s86_tid(), obj,
                first_off, second_off,
                existing_next, new_next,
                page_idx, tag32, bin, (unsigned)dst,
                (void*)so_base);
    } else {
        fprintf(stderr,
                "[HZ3_S86_CENTRAL_SHADOW] where=push_dup tid=0x%lx obj=%p "
                "first_push_off=0x%lx second_push_off=0x%lx "
                "existing_next=%p new_next=%p "
                "page_idx=%u base=%p\n",
                hz3_s86_tid(), obj,
                first_off, second_off,
                existing_next, new_next,
                page_idx,
            (void*)so_base);
    }
}

static inline void hz3_s86_log_mismatch(const char* where, void* obj, void* actual_next, void* shadow_next) {
    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
    uintptr_t so_base = hz3_s86_get_so_base();
    unsigned long ra_off = hz3_s86_ra_to_off(ra0);
    fprintf(stderr,
            "[HZ3_S86_CENTRAL_SHADOW] where=%s tid=0x%lx obj=%p actual_next=%p shadow_next=%p ra=%p base=%p off=0x%lx\n",
            where, hz3_s86_tid(), obj, actual_next, shadow_next, ra0, (void*)so_base, ra_off);
}

// Hash function for pointer (returns mixed value, not yet masked).
static inline size_t shadow_hash(uintptr_t key) {
    uintptr_t p = key;
    // Mix bits for better distribution
    p = (p >> 4) ^ (p >> 12) ^ (p >> 20);
    return (size_t)p;
}

static inline ShadowShard* shadow_pick_shard(uintptr_t key, size_t* idx0) {
    size_t h = shadow_hash(key);
    size_t shard_idx = h & g_shadow_shard_mask;
    ShadowShard* shard = &g_shadow_shards[shard_idx];
    *idx0 = (h >> (unsigned)HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2) & shard->mask;
    return shard;
}

// Find slot for key in one shard (linear probe).
// Returns index to insert/lookup. Sets *found=1 if key already exists.
// Returns cap only if table truly full (no empty/tombstone).
static size_t shadow_find_slot(ShadowEntry* map, size_t cap, size_t mask,
                               uintptr_t key, size_t idx0, int* found) {
    size_t idx = idx0;
    size_t start = idx;
    size_t first_tomb = cap;
    *found = 0;
    do {
        uintptr_t cur = map[idx].key;
        if (cur == key) {
            *found = 1;
            return idx;
        }
        if (cur == SHADOW_KEY_EMPTY) {
            return (first_tomb < cap) ? first_tomb : idx;
        }
        if (cur == SHADOW_KEY_TOMB && first_tomb >= cap) {
            first_tomb = idx;
        }
        idx = (idx + 1) & mask;
    } while (idx != start);
    return (first_tomb < cap) ? first_tomb : cap;
}

// Find existing entry for key in one shard.
// Returns index if found, or cap if not found.
static size_t shadow_find_existing(ShadowEntry* map, size_t cap, size_t mask,
                                   uintptr_t key, size_t idx0) {
    size_t idx = idx0;
    size_t start = idx;
    do {
        uintptr_t cur = map[idx].key;
        if (cur == SHADOW_KEY_EMPTY) {
            return cap;
        }
        if (cur == key) {
            return idx;
        }
        idx = (idx + 1) & mask;
    } while (idx != start);
    return cap;
}

int hz3_central_shadow_is_enabled(void) {
    return !atomic_load_explicit(&g_shadow_disabled, memory_order_relaxed);
}

int hz3_central_shadow_record(void* ptr, void* next, void* ra) {
    if (atomic_load_explicit(&g_shadow_disabled, memory_order_relaxed)) {
        return 0;  // Disabled, silently succeed
    }
    hz3_once(&g_shadow_once, hz3_s86_shadow_init_once);
    if (!g_shadow_shards || !g_shadow_storage) {
        return 0;
    }
    uintptr_t key = (uintptr_t)ptr;
    if (key == SHADOW_KEY_EMPTY || key == SHADOW_KEY_TOMB) {
        // Highly unexpected (invalid ptr). Disable to avoid false positives.
        if (!atomic_exchange_explicit(&g_shadow_disabled, 1, memory_order_relaxed)) {
            fprintf(stderr, "[HZ3_S86_CENTRAL_SHADOW] where=bad_ptr_key (shadow disabled)\n");
        }
        return 0;
    }

    size_t idx0;
    ShadowShard* shard = shadow_pick_shard(key, &idx0);
    hz3_lock_acquire(&shard->lock);

    // Check for duplicate (existing entry)
    size_t existing = shadow_find_existing(shard->map, shard->cap, shard->mask, key, idx0);
    if (existing < shard->cap) {
        // Duplicate push detected!
        void* old_next = shard->map[existing].expected_next;
        void* first_ra = shard->map[existing].first_ra;
        hz3_lock_release(&shard->lock);
        hz3_s86_log_dup(ptr, old_next, next, first_ra, ra);
        abort();
    }

    // Find empty slot
    int found = 0;
    size_t idx = shadow_find_slot(shard->map, shard->cap, shard->mask, key, idx0, &found);
    if (idx >= shard->cap) {
#if HZ3_S86_CENTRAL_SHADOW_EVICT_ON_FULL
        // Shard is full (no empty/tombstone). Evict by overwriting the primary slot.
        // This keeps S86 usable on long runs at the cost of potentially missing
        // dup-pushes whose first push was evicted.
        size_t evict_idx = idx0 & shard->mask;
        shard->map[evict_idx].key = key;
        shard->map[evict_idx].expected_next = next;
        shard->map[evict_idx].first_ra = ra;
        hz3_lock_release(&shard->lock);
        return 0;
#else
        // Map full, disable and continue
        if (!atomic_exchange_explicit(&g_shadow_disabled, 1, memory_order_relaxed)) {
            int cnt = atomic_load_explicit(&g_shadow_count, memory_order_relaxed);
            fprintf(stderr,
                    "[HZ3_S86_CENTRAL_SHADOW] where=map_full (shadow disabled) log2=%u shards_log2=%u count=%d cap=%zu\n",
                    (unsigned)HZ3_S86_CENTRAL_SHADOW_LOG2,
                    (unsigned)HZ3_S86_CENTRAL_SHADOW_SHARDS_LOG2,
                    cnt,
                    g_shadow_total_cap);
        }
        hz3_lock_release(&shard->lock);
        return 0;  // Continue silently
#endif
    }

    // Record entry with first_ra
    shard->map[idx].key = key;
    shard->map[idx].expected_next = next;
    shard->map[idx].first_ra = ra;
    atomic_fetch_add_explicit(&g_shadow_count, 1, memory_order_relaxed);

    hz3_lock_release(&shard->lock);
    return 0;
}

int hz3_central_shadow_verify_and_remove(void* ptr, void* actual_next) {
    if (atomic_load_explicit(&g_shadow_disabled, memory_order_relaxed)) {
        return 0;  // Disabled, silently succeed
    }
    hz3_once(&g_shadow_once, hz3_s86_shadow_init_once);
    if (!g_shadow_shards || !g_shadow_storage) {
        return 0;
    }
    uintptr_t key = (uintptr_t)ptr;
    if (key == SHADOW_KEY_EMPTY || key == SHADOW_KEY_TOMB) {
        return 0;
    }

    size_t idx0;
    ShadowShard* shard = shadow_pick_shard(key, &idx0);
    hz3_lock_acquire(&shard->lock);

    size_t idx = shadow_find_existing(shard->map, shard->cap, shard->mask, key, idx0);
    if (idx >= shard->cap) {
        // Not in shadow map - this is unexpected but not a corruption
        // (could be pushed before shadow was enabled)
        hz3_lock_release(&shard->lock);
        return 0;
    }

    void* expected = shard->map[idx].expected_next;

    // Remove entry first (before potential abort)
    shard->map[idx].key = SHADOW_KEY_TOMB;
    shard->map[idx].expected_next = NULL;
    shard->map[idx].first_ra = NULL;
    atomic_fetch_sub_explicit(&g_shadow_count, 1, memory_order_relaxed);

    hz3_lock_release(&shard->lock);

    // Check for mismatch
    if (actual_next != expected) {
        hz3_s86_log_mismatch("pop_mismatch", ptr, actual_next, expected);
        abort();
    }

    // Check for self-loop
    if (ptr == actual_next) {
        hz3_s86_log_mismatch("pop_selfloop", ptr, actual_next, expected);
        abort();
    }

    return 0;
}

int hz3_central_shadow_remove(void* ptr) {
    if (atomic_load_explicit(&g_shadow_disabled, memory_order_relaxed)) {
        return 0;
    }
    hz3_once(&g_shadow_once, hz3_s86_shadow_init_once);
    if (!g_shadow_shards || !g_shadow_storage) {
        return 0;
    }
    uintptr_t key = (uintptr_t)ptr;
    if (key == SHADOW_KEY_EMPTY || key == SHADOW_KEY_TOMB) {
        return 0;
    }

    size_t idx0;
    ShadowShard* shard = shadow_pick_shard(key, &idx0);
    hz3_lock_acquire(&shard->lock);

    size_t idx = shadow_find_existing(shard->map, shard->cap, shard->mask, key, idx0);
    if (idx < shard->cap) {
        shard->map[idx].key = SHADOW_KEY_TOMB;
        shard->map[idx].expected_next = NULL;
        shard->map[idx].first_ra = NULL;
        atomic_fetch_sub_explicit(&g_shadow_count, 1, memory_order_relaxed);
    }

    hz3_lock_release(&shard->lock);
    return 0;
}

#endif  // HZ3_S86_CENTRAL_SHADOW
