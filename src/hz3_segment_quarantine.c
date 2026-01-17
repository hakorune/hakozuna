// ============================================================================
// S47: Segment Quarantine
// ============================================================================
//
// Deterministic drain: focus on ONE segment until it becomes fully free.

#include "hz3_config.h"

#if HZ3_S47_SEGMENT_QUARANTINE

#include "hz3_segment_quarantine.h"
#include "hz3_central.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_arena.h"
#include "hz3_tcache.h"
#include "hz3_sc.h"
#include "hz3_owner_lease.h"
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif

#include <stdatomic.h>
#include <string.h>
#include <pthread.h>

#if HZ3_OWNER_EXCL_ENABLE
#include "hz3_owner_excl.h"
#endif

// Debug flag (set to 1 for verbose logging)
#define S47_DEBUG 0

// ----------------------------------------------------------------------------
// Draining state (per shard)
// ----------------------------------------------------------------------------

static _Atomic(uintptr_t) g_hz3_draining_seg[HZ3_NUM_SHARDS];
static _Atomic(uint32_t)  g_hz3_draining_epoch[HZ3_NUM_SHARDS];

// ----------------------------------------------------------------------------
// S47-3: Pinned Avoidance (per-shard avoid list)
// ----------------------------------------------------------------------------
// Protected by OwnerExclBox on collision (live_count > 1)

#if HZ3_S47_AVOID_ENABLE

typedef struct {
    void* seg_base;
    uint32_t added_epoch;
} Hz3AvoidEntry;

static Hz3AvoidEntry g_hz3_avoid_list[HZ3_NUM_SHARDS][HZ3_S47_AVOID_SLOTS];
static uint32_t g_hz3_avoid_count[HZ3_NUM_SHARDS];

// Event-only metrics for triage
static _Atomic(uint32_t) g_hz3_avoid_hit = 0;       // Skipped due to avoid list
static _Atomic(uint32_t) g_hz3_avoid_add_count = 0; // Added to avoid list

#endif  // HZ3_S47_AVOID_ENABLE

// ----------------------------------------------------------------------------
// S47-4: Snapshot metrics (for triage)
// ----------------------------------------------------------------------------
// Updated in scan_and_select() for max_potential tracking
static _Atomic(uint32_t) g_hz3_s47_max_potential = 0;      // max(free_pages + sampled_pages)
static _Atomic(uint32_t) g_hz3_s47_pinned_candidates = 0;  // count(near-full but hopeless)

// TLS reference
extern __thread Hz3TCache t_hz3_cache;

// ----------------------------------------------------------------------------
// S47-PolicyBox: adaptive parameter adjustment
// ----------------------------------------------------------------------------

typedef struct {
    Hz3S47PolicyMode mode;

    // Adjustable parameters (_Atomic for thread-safety)
    _Atomic(uint32_t) headroom_slots;
    _Atomic(uint32_t) scan_budget_hard;
    _Atomic(uint32_t) drain_passes_hard;
    _Atomic(uint32_t) panic_wait_us;

    // Statistics (OBSERVE/LEARN modes)
    _Atomic(uint32_t) alloc_full_count;
    _Atomic(uint32_t) alloc_failed_count;
    _Atomic(uint32_t) wait_success_count;
} Hz3S47Policy;

static Hz3S47Policy g_hz3_s47_policy;
static pthread_once_t g_hz3_s47_policy_once = PTHREAD_ONCE_INIT;

// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

void* hz3_s47_get_draining_seg(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    return (void*)atomic_load_explicit(&g_hz3_draining_seg[shard], memory_order_acquire);
}

int hz3_s47_is_draining(uint8_t shard, void* seg_base) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    uintptr_t draining = atomic_load_explicit(&g_hz3_draining_seg[shard], memory_order_acquire);
    return draining != 0 && draining == (uintptr_t)seg_base;
}

// ----------------------------------------------------------------------------
// Internal: seg_base from run address
// ----------------------------------------------------------------------------

static inline void* hz3_run_to_seg_base(void* run) {
    return (void*)((uintptr_t)run & ~((uintptr_t)HZ3_SEG_SIZE - 1));
}

// ----------------------------------------------------------------------------
// S47-3: Avoid list helpers (must be called under OwnerExcl protection)
// ----------------------------------------------------------------------------

#if HZ3_S47_AVOID_ENABLE

// Check if segment is in avoid list (caller must hold OwnerExcl)
static int hz3_s47_is_avoided(uint8_t shard, void* seg_base) {
    for (uint32_t i = 0; i < g_hz3_avoid_count[shard]; i++) {
        if (g_hz3_avoid_list[shard][i].seg_base == seg_base) {
            return 1;
        }
    }
    return 0;
}

// Add segment to avoid list (FIFO eviction if full)
static void hz3_s47_avoid_add(uint8_t shard, void* seg_base, uint32_t epoch) {
    // Check if already in list
    if (hz3_s47_is_avoided(shard, seg_base)) return;

    if (g_hz3_avoid_count[shard] >= HZ3_S47_AVOID_SLOTS) {
#if HZ3_S49_SEGMENT_PACKING
        // S49: Evicted entry returns to pack pool
        void* evicted_seg = g_hz3_avoid_list[shard][0].seg_base;
        Hz3SegMeta* evicted_meta = hz3_segmap_get(evicted_seg);
        if (evicted_meta) {
            hz3_pack_on_avoid_expire(shard, evicted_meta);
        }
#endif
        // Evict oldest entry (shift left)
        for (uint32_t i = 0; i < HZ3_S47_AVOID_SLOTS - 1; i++) {
            g_hz3_avoid_list[shard][i] = g_hz3_avoid_list[shard][i + 1];
        }
        g_hz3_avoid_count[shard] = HZ3_S47_AVOID_SLOTS - 1;
    }
    g_hz3_avoid_list[shard][g_hz3_avoid_count[shard]].seg_base = seg_base;
    g_hz3_avoid_list[shard][g_hz3_avoid_count[shard]].added_epoch = epoch;
    g_hz3_avoid_count[shard]++;

#if HZ3_S49_SEGMENT_PACKING
    // S49: Remove newly avoided segment from pack pool
    Hz3SegMeta* meta = hz3_segmap_get(seg_base);
    if (meta) {
        hz3_pack_on_avoid(shard, meta);
    }
#endif
}

// Expire old entries from avoid list (call before selection)
static void hz3_s47_avoid_expire(uint8_t shard, uint32_t current_epoch) {
    uint32_t write = 0;
    for (uint32_t i = 0; i < g_hz3_avoid_count[shard]; i++) {
        if (current_epoch - g_hz3_avoid_list[shard][i].added_epoch < HZ3_S47_AVOID_TTL) {
            g_hz3_avoid_list[shard][write++] = g_hz3_avoid_list[shard][i];
        }
#if HZ3_S49_SEGMENT_PACKING
        else {
            // S49: Expired entry returns to pack pool
            void* expired_seg = g_hz3_avoid_list[shard][i].seg_base;
            Hz3SegMeta* expired_meta = hz3_segmap_get(expired_seg);
            if (expired_meta) {
                hz3_pack_on_avoid_expire(shard, expired_meta);
            }
        }
#endif
    }
    g_hz3_avoid_count[shard] = write;
}

// Remove specific segment from avoid list (on successful free)
static void hz3_s47_avoid_remove(uint8_t shard, void* seg_base) {
    uint32_t write = 0;
    for (uint32_t i = 0; i < g_hz3_avoid_count[shard]; i++) {
        if (g_hz3_avoid_list[shard][i].seg_base != seg_base) {
            g_hz3_avoid_list[shard][write++] = g_hz3_avoid_list[shard][i];
        }
    }
    g_hz3_avoid_count[shard] = write;
}

#endif  // HZ3_S47_AVOID_ENABLE

// ----------------------------------------------------------------------------
// Scan: sample from central, pick best segment
// ----------------------------------------------------------------------------

typedef struct {
    void* seg_base;
    uint32_t pages_sum;  // pages in central for this segment
} Hz3SegCandidate;

#define HZ3_S47_MAX_CANDIDATES 32

static void hz3_s47_scan_and_select(uint8_t shard, uint32_t budget) {
    // TLS guard
    if (!t_hz3_cache.initialized) return;

#if HZ3_S47_AVOID_ENABLE
    // S47-3: Expire old avoid list entries BEFORE selection
    uint32_t current_epoch = 0;
#if HZ3_ARENA_PRESSURE_BOX
    current_epoch = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_acquire);
#endif
    hz3_s47_avoid_expire(shard, current_epoch);
#endif

    // Already draining?
    if (atomic_load_explicit(&g_hz3_draining_seg[shard], memory_order_acquire) != 0) {
        return;  // Already have a draining segment
    }

    Hz3SegCandidate candidates[HZ3_S47_MAX_CANDIDATES];
    int num_candidates = 0;

    // Temp buffer for popped runs (to push back later)
    void* popped[HZ3_S47_SCAN_BUDGET_HARD];
    int popped_sc[HZ3_S47_SCAN_BUDGET_HARD];
    uint32_t num_popped = 0;

    // Sample from central (all size classes)
    uint32_t sampled = 0;
    for (int sc = 0; sc < HZ3_NUM_SC && sampled < budget; sc++) {
        while (sampled < budget) {
            void* run = hz3_central_pop(shard, sc);
            if (!run) break;

            // Save for push back
            if (num_popped < HZ3_S47_SCAN_BUDGET_HARD) {
                popped[num_popped] = run;
                popped_sc[num_popped] = sc;
                num_popped++;
            }
            sampled++;

            // Get seg_base
            void* seg_base = hz3_run_to_seg_base(run);
            Hz3SegMeta* meta = hz3_segmap_get(seg_base);
            if (!meta) continue;

            // Owner check
            if (meta->owner != shard) continue;

            // Find or add candidate
            int found = -1;
            for (int i = 0; i < num_candidates; i++) {
                if (candidates[i].seg_base == seg_base) {
                    found = i;
                    break;
                }
            }

            uint32_t pages = hz3_sc_to_pages(sc);
            if (found >= 0) {
                candidates[found].pages_sum += pages;
            } else if (num_candidates < HZ3_S47_MAX_CANDIDATES) {
                candidates[num_candidates].seg_base = seg_base;
                candidates[num_candidates].pages_sum = pages;
                num_candidates++;
            }
        }
    }

    // Push back all popped runs (scatter prevention)
    for (uint32_t i = 0; i < num_popped; i++) {
        hz3_central_push(shard, popped_sc[i], popped[i]);
    }

    if (num_candidates == 0) return;

    // Find best candidate: prioritize segments closer to fully free
    void* best_seg = NULL;
    uint32_t best_score = 0;

    // S47-4: Track max_potential and pinned_candidates for snapshot
    uint32_t local_max_potential = 0;
    uint32_t local_pinned_count = 0;

    for (int i = 0; i < num_candidates; i++) {
        Hz3SegMeta* meta = hz3_segmap_get(candidates[i].seg_base);
        if (!meta) continue;

#if HZ3_S47_AVOID_ENABLE
        // S47-3: Skip avoided segments (pinned, failed to free)
        if (hz3_s47_is_avoided(shard, candidates[i].seg_base)) {
            atomic_fetch_add_explicit(&g_hz3_avoid_hit, 1, memory_order_relaxed);
            continue;
        }
#endif

        // S47-4: Raw potential for snapshot (before weighting)
        uint32_t raw_potential = meta->free_pages + candidates[i].pages_sum;
        if (raw_potential > local_max_potential) {
            local_max_potential = raw_potential;
        }
        // Pinned: near full (>=510 free) but can't reach 512
        if (meta->free_pages >= HZ3_PAGES_PER_SEG - 2 && raw_potential < HZ3_PAGES_PER_SEG) {
            local_pinned_count++;
        }

        // S47-3: Weighted scoring - prioritize segments with more free_pages
        // (segments already close to fully free are more likely to succeed)
        uint32_t potential = meta->free_pages * HZ3_S47_FREEPAGES_WEIGHT
                           + candidates[i].pages_sum;
        if (potential > best_score) {
            best_score = potential;
            best_seg = candidates[i].seg_base;
        }
    }

    // S47-4: Update global snapshot (atomic max)
    uint32_t old_max = atomic_load_explicit(&g_hz3_s47_max_potential, memory_order_relaxed);
    while (local_max_potential > old_max) {
        if (atomic_compare_exchange_weak_explicit(&g_hz3_s47_max_potential, &old_max,
                local_max_potential, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
    atomic_fetch_add_explicit(&g_hz3_s47_pinned_candidates, local_pinned_count, memory_order_relaxed);

    // Lower threshold: accept segment if >12.5% potential free (was 50%)
    if (best_seg && best_score >= HZ3_PAGES_PER_SEG / 8) {
        // Register draining segment
        uint32_t epoch = 0;
#if HZ3_ARENA_PRESSURE_BOX
        epoch = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_acquire);
#endif
        atomic_store_explicit(&g_hz3_draining_seg[shard], (uintptr_t)best_seg, memory_order_release);
        atomic_store_explicit(&g_hz3_draining_epoch[shard], epoch, memory_order_release);
#if HZ3_S49_SEGMENT_PACKING
        // S49: Remove draining segment from pack pool
        Hz3SegMeta* draining_meta = hz3_segmap_get(best_seg);
        if (draining_meta) {
            hz3_pack_on_drain(shard, draining_meta);
        }
#endif
#if S47_DEBUG
        fprintf(stderr, "[S47] scan: shard=%d draining_seg=%p best_score=%u\n", shard, best_seg, best_score);
#endif
    }
#if S47_DEBUG
    else if (num_candidates > 0) {
        fprintf(stderr, "[S47] scan: shard=%d no_good_candidate num=%d best_score=%u (need %u)\n",
                shard, num_candidates, best_score, HZ3_PAGES_PER_SEG / 8);
    }
#endif
}

// ----------------------------------------------------------------------------
// Drain: pop from central, return runs belonging to draining_seg
// ----------------------------------------------------------------------------

uint32_t hz3_s47_drain(uint8_t shard, uint32_t budget) {
    // TLS guard
    if (!t_hz3_cache.initialized) return 0;

    uint32_t freed_pages = 0;
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_try_acquire(shard);
    if (!lease_token.active) {
        return 0;
    }

    void* draining = hz3_s47_get_draining_seg(shard);
    if (!draining) {
#if S47_DEBUG
        static int miss_count = 0;
        if (++miss_count <= 5) {
            fprintf(stderr, "[S47] drain: shard=%d no_draining_seg\n", shard);
        }
#endif
        goto s47_drain_cleanup;
    }

    Hz3SegMeta* draining_meta = hz3_segmap_get(draining);
    if (!draining_meta) {
        // Invalid draining_seg, clear it
        atomic_store_explicit(&g_hz3_draining_seg[shard], 0, memory_order_release);
        goto s47_drain_cleanup;
    }

    uint32_t processed = 0;

    for (int sc = 0; sc < HZ3_NUM_SC && processed < budget; sc++) {
        while (processed < budget) {
            void* run = hz3_central_pop(shard, sc);
            if (!run) break;
            processed++;

            void* seg_base = hz3_run_to_seg_base(run);
            Hz3SegMeta* meta = hz3_segmap_get(seg_base);

            // Owner check: if not our shard, push back
            if (!meta || meta->owner != shard) {
                hz3_central_push(shard, sc, run);
                continue;
            }

            // Is this run from draining_seg?
            if (seg_base == draining) {
                // Return to segment
                uint32_t pages = hz3_sc_to_pages(sc);
                // Calculate start_page from run address
                size_t offset = (uintptr_t)run - (uintptr_t)seg_base;
                size_t start_page = offset / HZ3_PAGE_SIZE;
                hz3_segment_free_run(meta, start_page, pages);
                freed_pages += pages;

                // Check if segment is now fully free
                if (meta->free_pages >= HZ3_PAGES_PER_SEG) {
                    // Safety: never free the current thread's active segment.
                    // A stale t_hz3_cache.current_seg would become a UAF after hz3_segment_free().
                    if (t_hz3_cache.initialized) {
                        if (t_hz3_cache.current_seg == draining_meta ||
                            ((void*)t_hz3_cache.small_current_seg == draining)) {
                            atomic_store_explicit(&g_hz3_draining_seg[shard], 0, memory_order_release);
                            goto s47_drain_cleanup;
                        }
#if HZ3_LANE_SPLIT
                        if (t_hz3_cache.current_seg_base == draining ||
                            t_hz3_cache.small_current_seg_base == draining) {
                            atomic_store_explicit(&g_hz3_draining_seg[shard], 0, memory_order_release);
                            goto s47_drain_cleanup;
                        }
#endif
                    }
                    // Release segment to arena
#if S47_DEBUG
                    fprintf(stderr, "[S47] drain: RELEASED seg=%p freed_pages=%u\n", draining, freed_pages);
#endif
                    hz3_segment_free(draining);
#if HZ3_S47_AVOID_ENABLE
                    // S47-3: Remove from avoid list (address may be reused)
                    hz3_s47_avoid_remove(shard, draining);
#endif
                    atomic_store_explicit(&g_hz3_draining_seg[shard], 0, memory_order_release);
                    goto s47_drain_cleanup;
                }
            } else {
                // Not draining_seg, push back
                hz3_central_push(shard, sc, run);
            }
        }
    }

#if S47_DEBUG
    if (freed_pages > 0) {
        Hz3SegMeta* meta = hz3_segmap_get(draining);
        fprintf(stderr, "[S47] drain: shard=%d freed=%u pages, seg free_pages=%u (need 512)\n",
                shard, freed_pages, meta ? meta->free_pages : 0);
    }
#endif

s47_drain_cleanup:
    hz3_owner_lease_release(lease_token);
    return freed_pages;
}

// ----------------------------------------------------------------------------
// TTL: release draining_seg if too old
// ----------------------------------------------------------------------------

void hz3_s47_check_ttl(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) return;

    uintptr_t draining = atomic_load_explicit(&g_hz3_draining_seg[shard], memory_order_acquire);
    if (draining == 0) return;

    uint32_t start_epoch = atomic_load_explicit(&g_hz3_draining_epoch[shard], memory_order_acquire);
    uint32_t current_epoch = 0;
#if HZ3_ARENA_PRESSURE_BOX
    current_epoch = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_acquire);
#endif

    if (current_epoch - start_epoch >= HZ3_S47_QUARANTINE_MAX_EPOCHS) {
        // TTL expired, release draining_seg
#if HZ3_S47_AVOID_ENABLE
        // S47-3: Add to avoid list (pinned segment, failed to free)
        hz3_s47_avoid_add(shard, (void*)draining, current_epoch);
        atomic_fetch_add_explicit(&g_hz3_avoid_add_count, 1, memory_order_relaxed);
#endif
        atomic_store_explicit(&g_hz3_draining_seg[shard], 0, memory_order_release);
    }
}

// ----------------------------------------------------------------------------
// Compact: soft and hard variants
// ----------------------------------------------------------------------------

void hz3_s47_compact_soft(uint8_t shard) {
#if S47_DEBUG
    static int call_count = 0;
    if (++call_count <= 3) {
        fprintf(stderr, "[S47] compact_soft: shard=%d\n", shard);
    }
#endif

#if HZ3_S47_SOFT_SKIP_HOPELESS
    // S47-4: Skip soft when hopeless (max_potential < 512)
    // last_max==0 means "no observation yet" → don't skip
    uint32_t last_max = atomic_load_explicit(&g_hz3_s47_max_potential, memory_order_relaxed);
    if (last_max > 0 && last_max < HZ3_PAGES_PER_SEG) {
        return;  // hopeless, skip soft compaction
    }
#endif

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_try_acquire(shard);
    if (!lease_token.active) {
        return;
    }

#if HZ3_OWNER_EXCL_ENABLE
    // OwnerExcl: protect avoid_list on collision
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(shard);
#endif

    // Check TTL first
    hz3_s47_check_ttl(shard);

    // Scan and select draining segment if needed
    hz3_s47_scan_and_select(shard, HZ3_S47_SCAN_BUDGET_SOFT);

    // Single drain pass
    for (int pass = 0; pass < HZ3_S47_DRAIN_PASSES_SOFT; pass++) {
        uint32_t freed = hz3_s47_drain(shard, HZ3_S47_SCAN_BUDGET_SOFT);
        if (freed == 0) break;
    }

#if HZ3_OWNER_EXCL_ENABLE
    hz3_owner_excl_release(excl_token);
#endif
    hz3_owner_lease_release(lease_token);
}

void hz3_s47_compact_hard(uint8_t shard) {
#if S47_DEBUG
    static int call_count = 0;
    if (++call_count <= 3) {
        fprintf(stderr, "[S47] compact_hard: shard=%d\n", shard);
    }
#endif

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_try_acquire(shard);
    if (!lease_token.active) {
        return;
    }

#if HZ3_OWNER_EXCL_ENABLE
    // OwnerExcl: protect avoid_list on collision
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(shard);
#endif

    // Check TTL first
    hz3_s47_check_ttl(shard);

    // Collision note:
    // - When threads>shards, OwnerLease hold time dominates tail latency.
    // - Keep hard compaction bounded under collision.
    uint32_t scan_budget = HZ3_S47_SCAN_BUDGET_HARD;
    uint32_t drain_budget = HZ3_S47_SCAN_BUDGET_HARD;

    // Get drain passes from PolicyBox (dynamic)
    uint32_t passes = hz3_s47_policy_get_drain_passes_hard();

    if (lease_token.locked) {
        // Use SOFT budgets (smaller work chunk) and cap passes.
        scan_budget = HZ3_S47_SCAN_BUDGET_SOFT;
        drain_budget = HZ3_S47_SCAN_BUDGET_SOFT;
        uint32_t ops_budget = (uint32_t)HZ3_OWNER_LEASE_OPS_BUDGET;
        if (ops_budget == 0) {
            ops_budget = 1;
        }
        if (passes > ops_budget) {
            passes = ops_budget;
        }
    }

    // Scan and select draining segment if needed
    hz3_s47_scan_and_select(shard, scan_budget);

    // Multiple drain passes
    for (uint32_t pass = 0; pass < passes; pass++) {
        uint32_t freed = hz3_s47_drain(shard, drain_budget);
        if (freed == 0) break;

        // Check if draining_seg was released
        if (hz3_s47_get_draining_seg(shard) == NULL) break;
    }

#if HZ3_OWNER_EXCL_ENABLE
    hz3_owner_excl_release(excl_token);
#endif
    hz3_owner_lease_release(lease_token);
}

// ----------------------------------------------------------------------------
// S47-PolicyBox: initialization and event handlers
// ----------------------------------------------------------------------------

// Internal init (called via pthread_once)
static void hz3_s47_policy_do_init(void) {
    g_hz3_s47_policy.mode = HZ3_S47_POLICY_MODE;
    atomic_store_explicit(&g_hz3_s47_policy.headroom_slots, HZ3_S47_HEADROOM_SLOTS, memory_order_relaxed);
    atomic_store_explicit(&g_hz3_s47_policy.scan_budget_hard, HZ3_S47_SCAN_BUDGET_HARD, memory_order_relaxed);
    atomic_store_explicit(&g_hz3_s47_policy.drain_passes_hard, HZ3_S47_DRAIN_PASSES_HARD, memory_order_relaxed);
    atomic_store_explicit(&g_hz3_s47_policy.panic_wait_us, HZ3_S47_PANIC_WAIT_US, memory_order_relaxed);
    atomic_store_explicit(&g_hz3_s47_policy.alloc_full_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_hz3_s47_policy.alloc_failed_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_hz3_s47_policy.wait_success_count, 0, memory_order_relaxed);
}

// Public init (pthread_once safe)
void hz3_s47_policy_init(void) {
    pthread_once(&g_hz3_s47_policy_once, hz3_s47_policy_do_init);
}

// Event: slot allocation failed (entering bounded wait loop)
void hz3_s47_policy_on_alloc_full(void) {
    hz3_s47_policy_init();
    if (g_hz3_s47_policy.mode == HZ3_S47_POLICY_FROZEN) return;

    atomic_fetch_add_explicit(&g_hz3_s47_policy.alloc_full_count, 1, memory_order_relaxed);

    if (g_hz3_s47_policy.mode == HZ3_S47_POLICY_LEARN) {
        // LEARN: increase headroom for earlier detection next time
        uint32_t cur = atomic_load_explicit(&g_hz3_s47_policy.headroom_slots, memory_order_relaxed);
        if (cur < 256) {
            atomic_store_explicit(&g_hz3_s47_policy.headroom_slots, cur + 16, memory_order_relaxed);
        }
    }
}

// Event: final allocation failed (bounded wait exhausted)
void hz3_s47_policy_on_alloc_failed(void) {
    hz3_s47_policy_init();
    if (g_hz3_s47_policy.mode == HZ3_S47_POLICY_FROZEN) return;

    atomic_fetch_add_explicit(&g_hz3_s47_policy.alloc_failed_count, 1, memory_order_relaxed);

    if (g_hz3_s47_policy.mode == HZ3_S47_POLICY_LEARN) {
        // LEARN: increase drain passes and wait time
        uint32_t passes = atomic_load_explicit(&g_hz3_s47_policy.drain_passes_hard, memory_order_relaxed);
        if (passes < 8) {
            atomic_store_explicit(&g_hz3_s47_policy.drain_passes_hard, passes + 1, memory_order_relaxed);
        }
        uint32_t wait = atomic_load_explicit(&g_hz3_s47_policy.panic_wait_us, memory_order_relaxed);
        if (wait < 500) {
            atomic_store_explicit(&g_hz3_s47_policy.panic_wait_us, wait + 50, memory_order_relaxed);
        }
    }
}

// Event: succeeded after bounded wait
void hz3_s47_policy_on_wait_success(void) {
    hz3_s47_policy_init();
    if (g_hz3_s47_policy.mode == HZ3_S47_POLICY_FROZEN) return;
    atomic_fetch_add_explicit(&g_hz3_s47_policy.wait_success_count, 1, memory_order_relaxed);
}

// ----------------------------------------------------------------------------
// S47-PolicyBox: parameter getters
// ----------------------------------------------------------------------------

uint32_t hz3_s47_policy_get_headroom_slots(void) {
    hz3_s47_policy_init();
    return atomic_load_explicit(&g_hz3_s47_policy.headroom_slots, memory_order_relaxed);
}

uint32_t hz3_s47_policy_get_panic_wait_us(void) {
    hz3_s47_policy_init();
    return atomic_load_explicit(&g_hz3_s47_policy.panic_wait_us, memory_order_relaxed);
}

uint32_t hz3_s47_policy_get_drain_passes_hard(void) {
    hz3_s47_policy_init();
    return atomic_load_explicit(&g_hz3_s47_policy.drain_passes_hard, memory_order_relaxed);
}

// ----------------------------------------------------------------------------
// S47-4: Snapshot metric getters
// ----------------------------------------------------------------------------

uint32_t hz3_s47_get_max_potential(void) {
    return atomic_load_explicit(&g_hz3_s47_max_potential, memory_order_relaxed);
}

uint32_t hz3_s47_get_pinned_candidates(void) {
    return atomic_load_explicit(&g_hz3_s47_pinned_candidates, memory_order_relaxed);
}

uint32_t hz3_s47_get_avoid_hits(void) {
#if HZ3_S47_AVOID_ENABLE
    return atomic_load_explicit(&g_hz3_avoid_hit, memory_order_relaxed);
#else
    return 0;
#endif
}

uint32_t hz3_s47_get_avoid_used(void) {
#if HZ3_S47_AVOID_ENABLE
    uint32_t total = 0;
    for (int i = 0; i < HZ3_NUM_SHARDS; i++) {
        total += g_hz3_avoid_count[i];
    }
    return total;
#else
    return 0;
#endif
}

#endif  // HZ3_S47_SEGMENT_QUARANTINE
