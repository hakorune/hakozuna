// S49: Segment Packing Box
//
// Existing segment priority allocation to reduce fragmentation.
// Per-shard partial pool with pages-bucket structure.

#include "hz3_config.h"

#if HZ3_S49_SEGMENT_PACKING

#include "hz3_segment_packing.h"
#include "hz3_segment.h"  // S49-2: for hz3_segment_max_contig_free()
#include "hz3_types.h"
#include "hz3_owner_lease.h"
#include <stdatomic.h>
#include <pthread.h>

// ============================================================================
// Pack Pool Structure
// ============================================================================
//
// bucket[0] = unused (no pages available)
// bucket[1..MAX_PAGES] = segments with >= bucket pages available
//
// Each shard has its own pool.
//
// IMPORTANT: Shard assignment is round-robin and can collide (threads > shards).
// If multiple threads share the same owner shard concurrently, pack pool mutations
// become racy unless protected.

static struct {
    pthread_mutex_t lock;
    Hz3SegMeta* head[HZ3_S49_PACK_MAX_PAGES + 1];
} g_pack_pool[HZ3_NUM_SHARDS];

// Stats (event-only)
static _Atomic(uint32_t) g_pack_hits = 0;
static _Atomic(uint32_t) g_pack_misses = 0;

static pthread_once_t g_pack_pool_once = PTHREAD_ONCE_INIT;

static void hz3_pack_pool_init_once(void) {
    for (uint32_t owner = 0; owner < HZ3_NUM_SHARDS; owner++) {
        pthread_mutex_init(&g_pack_pool[owner].lock, NULL);
    }
}

static inline void hz3_pack_pool_lock(uint8_t owner) {
    pthread_once(&g_pack_pool_once, hz3_pack_pool_init_once);
    pthread_mutex_lock(&g_pack_pool[owner].lock);
}

static inline void hz3_pack_pool_unlock(uint8_t owner) {
    pthread_mutex_unlock(&g_pack_pool[owner].lock);
}

// S56-1: Pack best-fit stats
#if HZ3_S56_PACK_BESTFIT && HZ3_S56_PACK_BESTFIT_STATS
static _Atomic(uint64_t) g_pack_bestfit_hit = 0;   // K候補で成功
static _Atomic(uint64_t) g_pack_bestfit_miss = 0;  // K候補で失敗

// Forward declaration for atexit registration
static void hz3_s56_ensure_atexit_registered(void);
#endif

// ============================================================================
// Internal Helpers
// ============================================================================

// Insert segment into pack pool (pack_in_list prevents double insertion)
static void pack_insert(uint8_t owner, Hz3SegMeta* meta) {
    if (meta->pack_in_list) return;  // Already in pool

    // Calculate bucket from pack_max_fit
    uint8_t bucket = meta->pack_max_fit;
    if (bucket > HZ3_S49_PACK_MAX_PAGES) {
        bucket = HZ3_S49_PACK_MAX_PAGES;
    }
    if (bucket == 0) return;  // No space

    // Insert at head
    meta->pack_bucket = bucket;
    meta->pack_next = g_pack_pool[owner].head[bucket];
    g_pack_pool[owner].head[bucket] = meta;
    meta->pack_in_list = 1;
}

// Remove segment from pack pool
static void pack_remove(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta->pack_in_list) return;  // Not in pool

    uint8_t bucket = meta->pack_bucket;
    Hz3SegMeta** pp = &g_pack_pool[owner].head[bucket];

    while (*pp) {
        if (*pp == meta) {
            *pp = meta->pack_next;
            meta->pack_in_list = 0;
            meta->pack_next = NULL;
            return;
        }
        pp = &(*pp)->pack_next;
    }

    // Not found (shouldn't happen if pack_in_list is correct)
    meta->pack_in_list = 0;
    meta->pack_next = NULL;
}

// Pop first segment from bucket (returns NULL if empty)
static Hz3SegMeta* pack_pop(uint8_t owner, uint8_t bucket) {
    Hz3SegMeta* meta = g_pack_pool[owner].head[bucket];
    if (!meta) return NULL;

    g_pack_pool[owner].head[bucket] = meta->pack_next;
    meta->pack_in_list = 0;
    meta->pack_next = NULL;

    return meta;
}

// ============================================================================
// Public API: Primary Entry Point
// ============================================================================

Hz3SegMeta* hz3_pack_try_alloc(uint8_t owner, size_t npages, uint32_t tries) {
    if (npages == 0 || npages > HZ3_S49_PACK_MAX_PAGES) {
        return NULL;
    }
    if (owner >= HZ3_NUM_SHARDS) return NULL;

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(owner);
    hz3_pack_pool_lock(owner);
#define PACK_RETURN(_ptr)            \
    do {                             \
        Hz3SegMeta* _ret = (_ptr);   \
        hz3_pack_pool_unlock(owner); \
        hz3_owner_lease_release(lease_token); \
        return _ret;                 \
    } while (0)

#if HZ3_S56_PACK_BESTFIT
    // S56-1: Bounded best-fit (collect K candidates, pick min score)
#if HZ3_S56_PACK_BESTFIT_STATS
    // Ensure atexit handler is registered once
    hz3_s56_ensure_atexit_registered();
#endif

    Hz3SegMeta* candidates[HZ3_S56_PACK_BESTFIT_K];
    uint16_t contigs[HZ3_S56_PACK_BESTFIT_K];
    uint32_t scores[HZ3_S56_PACK_BESTFIT_K];
    int num_candidates = 0;

    uint32_t remaining = tries;
    uint32_t tries_per_bucket = HZ3_S49_PACK_TRIES_PER_BUCKET;

    // Iterate buckets from npages to MAX_PAGES (best-fit by bucket size)
    for (uint8_t bucket = (uint8_t)npages; bucket <= HZ3_S49_PACK_MAX_PAGES && remaining > 0; bucket++) {
        uint32_t bucket_tries = tries_per_bucket;

        while (bucket_tries > 0 && remaining > 0) {
            Hz3SegMeta* meta = pack_pop(owner, bucket);
            if (!meta) {
                break;  // Bucket empty
            }
            remaining--;
            bucket_tries--;

            // Check contiguous free
            uint16_t max_contig = hz3_segment_max_contig_free(meta);

            if (max_contig >= npages) {
                // Valid candidate - compute score
                // score = (free_pages << 8) + (max_contig - npages)
                // - Lower score = more packed (fewer free pages)
                // - Tiebreak: tighter fit (less waste)
                uint32_t score = ((uint32_t)meta->free_pages << 8) +
                                 (uint32_t)(max_contig - npages);

                // Add to candidates and stop as soon as we have K entries.
                // Rationale: keep overhead strictly bounded (S56-1 is a "K-sample" best-fit).
                candidates[num_candidates] = meta;
                contigs[num_candidates] = max_contig;
                scores[num_candidates] = score;
                num_candidates++;
                if (num_candidates >= HZ3_S56_PACK_BESTFIT_K) {
                    // Stop scanning once we have enough candidates.
                    goto s56_select_best;
                }
            } else {
                // Not enough contiguous - update max_fit and return via pack_insert
                uint8_t new_max_fit = (max_contig > HZ3_S49_PACK_MAX_PAGES)
                                    ? HZ3_S49_PACK_MAX_PAGES
                                    : (uint8_t)max_contig;
                meta->pack_max_fit = new_max_fit;
                pack_insert(owner, meta);  // Safe: pack_insert can rebucket
            }
        }
    }

 s56_select_best:
    // Return best candidate and reinsert others
    if (num_candidates > 0) {
        int best_idx = 0;
        uint32_t best_score = scores[0];
        for (int i = 1; i < num_candidates; i++) {
            if (scores[i] < best_score) {
                best_idx = i;
                best_score = scores[i];
            }
        }

        Hz3SegMeta* best = candidates[best_idx];

        // Return non-best candidates via pack_insert (NOT hz3_pack_reinsert)
        for (int i = 0; i < num_candidates; i++) {
            if (i != best_idx) {
                Hz3SegMeta* non_best = candidates[i];
                uint16_t nb_contig = contigs[i];
                non_best->pack_max_fit = (nb_contig > HZ3_S49_PACK_MAX_PAGES)
                                       ? HZ3_S49_PACK_MAX_PAGES
                                       : (uint8_t)nb_contig;
                pack_insert(owner, non_best);  // NOT hz3_pack_reinsert
            }
        }

#if HZ3_S56_PACK_BESTFIT_STATS
        atomic_fetch_add_explicit(&g_pack_bestfit_hit, 1, memory_order_relaxed);
#endif
        atomic_fetch_add_explicit(&g_pack_hits, 1, memory_order_relaxed);
        PACK_RETURN(best);
    }

#if HZ3_S56_PACK_BESTFIT_STATS
    atomic_fetch_add_explicit(&g_pack_bestfit_miss, 1, memory_order_relaxed);
#endif
    atomic_fetch_add_explicit(&g_pack_misses, 1, memory_order_relaxed);
    PACK_RETURN(NULL);

#else
    // S49: Original first-match per bucket algorithm
    uint32_t remaining = tries;
    uint32_t tries_per_bucket = HZ3_S49_PACK_TRIES_PER_BUCKET;

    // S49-2: Iterate from npages → MAX (best-fit)
    // Note: MAX→npages wastes big holes and increases fragmentation
    for (uint8_t bucket = (uint8_t)npages; bucket <= HZ3_S49_PACK_MAX_PAGES && remaining > 0; bucket++) {
        uint32_t bucket_tries = tries_per_bucket;

        while (bucket_tries > 0 && remaining > 0) {
            Hz3SegMeta* meta = pack_pop(owner, bucket);
            if (!meta) break;  // Bucket empty

            remaining--;
            bucket_tries--;

            // S49-2: Use max_contig_free instead of free_pages for judgment
            uint16_t max_contig = hz3_segment_max_contig_free(meta);

            if (max_contig >= npages) {
                // This segment has enough contiguous space
                atomic_fetch_add_explicit(&g_pack_hits, 1, memory_order_relaxed);
                PACK_RETURN(meta);
            }

            // Not enough contiguous space - update pack_max_fit and reinsert
            meta->pack_max_fit = (max_contig > HZ3_S49_PACK_MAX_PAGES)
                               ? HZ3_S49_PACK_MAX_PAGES
                               : (uint8_t)max_contig;

            // Reinsert to appropriate bucket (or drop if pack_max_fit == 0)
            if (meta->pack_max_fit > 0) {
                pack_insert(owner, meta);
            }
        }
    }

    atomic_fetch_add_explicit(&g_pack_misses, 1, memory_order_relaxed);
    PACK_RETURN(NULL);
#endif
#undef PACK_RETURN
}

// ============================================================================
// Public API: Event Handlers
// ============================================================================

void hz3_pack_on_new(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    hz3_pack_pool_lock(owner);

    // New segment has all pages free
    meta->pack_max_fit = HZ3_S49_PACK_MAX_PAGES;
    meta->pack_bucket = 0;
    meta->pack_in_list = 0;
    meta->pack_next = NULL;

    pack_insert(owner, meta);
    hz3_pack_pool_unlock(owner);
}

void hz3_pack_on_alloc(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(owner);
    hz3_pack_pool_lock(owner);

    // After alloc_run, check if segment still has space
    if (meta->free_pages == 0) {
        // Full - remove from pack pool
        pack_remove(owner, meta);
        hz3_pack_pool_unlock(owner);
        hz3_owner_lease_release(lease_token);
        return;
    }

    // Update pack_max_fit (free_pages is upper bound for contiguous pages)
    if (meta->free_pages < meta->pack_max_fit) {
        meta->pack_max_fit = (uint8_t)meta->free_pages;
    }

    // If not in pool, add it; if in pool with wrong bucket, update
    if (meta->pack_in_list) {
        uint8_t new_bucket = meta->pack_max_fit;
        if (new_bucket > HZ3_S49_PACK_MAX_PAGES) {
            new_bucket = HZ3_S49_PACK_MAX_PAGES;
        }
        if (new_bucket != meta->pack_bucket) {
            pack_remove(owner, meta);
            pack_insert(owner, meta);
        }
    } else {
        pack_insert(owner, meta);
    }
    hz3_pack_pool_unlock(owner);
    hz3_owner_lease_release(lease_token);
}

void hz3_pack_on_free(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    hz3_pack_pool_lock(owner);

    // After free_run, reset pack_max_fit (pages returned, may have larger contiguous block)
    // Conservative: set to min(free_pages, MAX_PAGES)
    uint8_t new_max = (meta->free_pages > HZ3_S49_PACK_MAX_PAGES)
                    ? HZ3_S49_PACK_MAX_PAGES
                    : (uint8_t)meta->free_pages;
    meta->pack_max_fit = new_max;

    // Update bucket if needed
    if (meta->pack_in_list) {
        uint8_t new_bucket = new_max;
        if (new_bucket > HZ3_S49_PACK_MAX_PAGES) {
            new_bucket = HZ3_S49_PACK_MAX_PAGES;
        }
        if (new_bucket != meta->pack_bucket) {
            pack_remove(owner, meta);
            if (new_max > 0) {
                pack_insert(owner, meta);
            }
        }
    } else if (new_max > 0) {
        pack_insert(owner, meta);
    }
    hz3_pack_pool_unlock(owner);
}

void hz3_pack_on_drain(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    hz3_pack_pool_lock(owner);

    // Draining segment - remove from pack pool
    pack_remove(owner, meta);
    hz3_pack_pool_unlock(owner);
}

void hz3_pack_on_avoid(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    hz3_pack_pool_lock(owner);

    // Avoided segment - remove from pack pool
    pack_remove(owner, meta);
    hz3_pack_pool_unlock(owner);
}

void hz3_pack_on_avoid_expire(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    hz3_pack_pool_lock(owner);

    // Avoid TTL expired - re-add to pack pool
    // Reset pack_max_fit based on current free_pages
    uint8_t new_max = (meta->free_pages > HZ3_S49_PACK_MAX_PAGES)
                    ? HZ3_S49_PACK_MAX_PAGES
                    : (uint8_t)meta->free_pages;
    meta->pack_max_fit = new_max;

    if (new_max > 0) {
        pack_insert(owner, meta);
    }
    hz3_pack_pool_unlock(owner);
}

void hz3_pack_reinsert(uint8_t owner, Hz3SegMeta* meta) {
    if (!meta) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(owner);
    hz3_pack_pool_lock(owner);

    // S49-2: Reinsert with pre-set pack_max_fit (does NOT overwrite)
    // Caller must set pack_max_fit to max_contig_free before calling
    if (meta->pack_max_fit > 0) {
        pack_insert(owner, meta);
    }
    hz3_pack_pool_unlock(owner);
    hz3_owner_lease_release(lease_token);
}

// ============================================================================
// Stats
// ============================================================================

uint32_t hz3_pack_get_hits(void) {
    return atomic_load_explicit(&g_pack_hits, memory_order_relaxed);
}

uint32_t hz3_pack_get_misses(void) {
    return atomic_load_explicit(&g_pack_misses, memory_order_relaxed);
}

// ============================================================================
// S54: Pack Pool Observation API (PackBox境界)
// ============================================================================

#if HZ3_SEG_SCAVENGE_OBSERVE
// S54: Pack pool observation API (my_shard only, read-only)
// Iterates pack pool buckets and calculates segment statistics for OBSERVE mode.
// Safety:
// - owner shard only (no cross-shard access)
// - read-only traversal (no mutation)
// - pack_max_fit is updated via hz3_segment_max_contig_free() slow path
void hz3_pack_observe_owner(uint8_t owner,
                             uint32_t* out_segments,
                             uint32_t* out_free_pages,
                             uint32_t* out_candidate_pages) {
    if (!out_segments || !out_free_pages || !out_candidate_pages) return;
    if (owner >= HZ3_NUM_SHARDS) return;
    hz3_pack_pool_lock(owner);

    uint32_t total_segments = 0;
    uint32_t total_free_pages = 0;
    uint32_t candidate_pages = 0;

    // Iterate pack pool (owner shard only, partially-free segments)
    // bucket[0] is unused, iterate bucket[1..MAX_PAGES]
    for (uint8_t bucket = 1; bucket <= HZ3_S49_PACK_MAX_PAGES; bucket++) {
        Hz3SegMeta* meta = g_pack_pool[owner].head[bucket];
        while (meta) {
            total_segments++;
            total_free_pages += meta->free_pages;

            // Calculate max contiguous free pages (slow path, event-only acceptable)
            uint16_t max_contig = hz3_segment_max_contig_free(meta);
            if (max_contig >= HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES) {
                candidate_pages += max_contig;
            }

            meta = meta->pack_next;
        }
    }

    *out_segments = total_segments;
    *out_free_pages = total_free_pages;
    *out_candidate_pages = candidate_pages;
    hz3_pack_pool_unlock(owner);
}
#endif  // HZ3_SEG_SCAVENGE_OBSERVE

// ============================================================================
// S56-1: Atexit Statistics
// ============================================================================

#if HZ3_S56_PACK_BESTFIT && HZ3_S56_PACK_BESTFIT_STATS
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static void hz3_s56_pack_bestfit_atexit(void) {
    uint64_t hit = atomic_load_explicit(&g_pack_bestfit_hit, memory_order_relaxed);
    uint64_t miss = atomic_load_explicit(&g_pack_bestfit_miss, memory_order_relaxed);
    uint64_t total = hit + miss;

    if (total > 0) {
        fprintf(stderr, "[HZ3_S56_PACK_BESTFIT] hit=%lu miss=%lu hit_rate=%.2f%%\n",
                hit, miss, 100.0 * hit / total);
    }
}

static pthread_once_t hz3_s56_pack_bestfit_atexit_once = PTHREAD_ONCE_INIT;

static void hz3_s56_pack_bestfit_atexit_register(void) {
    atexit(hz3_s56_pack_bestfit_atexit);
}

// Call this during initialization (e.g., from first pack_try_alloc call)
static void hz3_s56_ensure_atexit_registered(void) {
    pthread_once(&hz3_s56_pack_bestfit_atexit_once, hz3_s56_pack_bestfit_atexit_register);
}
#endif  // HZ3_S56_PACK_BESTFIT && HZ3_S56_PACK_BESTFIT_STATS

#endif  // HZ3_S49_SEGMENT_PACKING
