#define _GNU_SOURCE

#include "hz3_arena.h"
#include "hz3_types.h"
#include "hz3_oom.h"
#include "hz3_mem_budget.h"
#include "hz3_tcache.h"  // for hz3_dstbin_flush_remote_all, t_hz3_cache
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>   // for fprintf (gate metrics)
#include <sys/mman.h>
#include <string.h>
#include <sched.h>  // for sched_yield()
#include <time.h>   // for nanosleep()

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE MAP_FIXED
#endif

// ----------------------------------------------------------------------------
// Global atomic pointers for lock-free range check (S12-4)
// Published in do_init: end first (release), then base (release)
// Read order in hot path: base first (acquire), then end (relaxed)
// ----------------------------------------------------------------------------
_Atomic(void*) g_hz3_arena_base = NULL;
_Atomic(void*) g_hz3_arena_end = NULL;

#if HZ3_ARENA_PRESSURE_BOX
// Pressure epoch: incremented on arena exhaustion, threads check and flush once per epoch.
_Atomic(uint32_t) g_hz3_arena_pressure_epoch = 0;
#endif

#if HZ3_OOM_SHOT || HZ3_OOM_FAILFAST
static _Atomic(int) g_hz3_arena_alloc_full_diag_fired = 0;
#endif

#if HZ3_S47_SEGMENT_QUARANTINE
// S47: Used slots counter (updated under g_hz3_arena_lock)
static _Atomic(uint32_t) g_hz3_arena_used_slots = 0;
// S47-PolicyBox: Generation counter for bounded wait (incremented on segment release)
static _Atomic(uint32_t) g_hz3_arena_free_gen = 0;
#if HZ3_S47_ARENA_GATE
// S47-2: ArenaGateBox - leader election gate (0=free, 1=leader active)
static _Atomic(uint32_t) g_hz3_arena_gate = 0;
// S47-2: Metrics (event-only, for triage)
static _Atomic(uint32_t) g_hz3_gate_leader_count = 0;
static _Atomic(uint32_t) g_hz3_gate_follower_wait_ok = 0;  // free_gen changed
static _Atomic(uint32_t) g_hz3_gate_follower_timeout = 0;  // waited but no change
static _Atomic(uint32_t) g_hz3_gate_seg_freed = 0;         // segments freed
#endif
#endif

#if HZ3_SMALL_V2_PTAG_ENABLE
// Page tag array: 0 = not ours, mmap'd in init
_Atomic(uint16_t)* g_hz3_page_tag = NULL;
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
// S17: 32-bit PageTagMap (dst/bin direct)
_Atomic(uint32_t)* g_hz3_page_tag32 = NULL;
#endif

typedef struct {
    size_t size;
    uint32_t slots;
    _Atomic uint8_t* used;
    uint32_t alloc_cursor;  // next slot search start (under g_hz3_arena_lock)
} Hz3ArenaState;

static Hz3ArenaState g_hz3_arena;
static pthread_once_t g_hz3_arena_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_hz3_arena_lock = PTHREAD_MUTEX_INITIALIZER;

static void hz3_arena_do_init(void) {
    g_hz3_arena.size = (size_t)HZ3_ARENA_SIZE;
    g_hz3_arena.slots = (uint32_t)(g_hz3_arena.size / HZ3_SEG_SIZE);
    g_hz3_arena.alloc_cursor = 0;

    void* base = mmap(NULL, g_hz3_arena.size, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        hz3_oom_note("arena_init_base", g_hz3_arena.size, 0);
        atomic_store_explicit(&g_hz3_arena_base, NULL, memory_order_release);
        g_hz3_arena.slots = 0;
        return;
    }

    size_t used_bytes = (size_t)g_hz3_arena.slots * sizeof(_Atomic uint8_t);
    void* used = mmap(NULL, used_bytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (used == MAP_FAILED) {
        hz3_oom_note("arena_init_used", used_bytes, 0);
        munmap(base, g_hz3_arena.size);
        atomic_store_explicit(&g_hz3_arena_base, NULL, memory_order_release);
        g_hz3_arena.slots = 0;
        return;
    }

    g_hz3_arena.used = (_Atomic uint8_t*)used;
    for (uint32_t i = 0; i < g_hz3_arena.slots; i++) {
        atomic_store_explicit(&g_hz3_arena.used[i], 0, memory_order_relaxed);
    }

#if HZ3_SMALL_V2_PTAG_ENABLE
    void* page_tag = NULL;
    size_t page_tag_bytes = 0;
#endif
#if HZ3_SMALL_V2_PTAG_ENABLE
    // Allocate page tag array: 4GB / 4KB = 1M pages, 2 bytes each = 2MB
    page_tag_bytes = HZ3_ARENA_MAX_PAGES * sizeof(_Atomic(uint16_t));
    page_tag = mmap(NULL, page_tag_bytes, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page_tag == MAP_FAILED) {
        // Non-fatal: PTAG just won't work, fallback to v1 path
        g_hz3_page_tag = NULL;
        page_tag = NULL;
        page_tag_bytes = 0;
    } else {
        g_hz3_page_tag = (_Atomic(uint16_t)*)page_tag;
        // mmap guarantees zero-filled, but be explicit for clarity
        // (no loop needed, mmap returns zeroed memory)
    }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
    // S17: Allocate 32-bit page tag array: 4GB / 4KB = 1M pages, 4 bytes each = 4MB
    size_t page_tag32_bytes = HZ3_ARENA_MAX_PAGES * sizeof(_Atomic(uint32_t));
    void* page_tag32 = mmap(NULL, page_tag32_bytes, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page_tag32 == MAP_FAILED) {
        g_hz3_page_tag32 = NULL;
#if HZ3_PTAG_DSTBIN_FASTLOOKUP
        hz3_oom_note("arena_ptag32", page_tag32_bytes, 0);
#if HZ3_SMALL_V2_PTAG_ENABLE
        if (page_tag && page_tag_bytes > 0) {
            munmap(page_tag, page_tag_bytes);
            g_hz3_page_tag = NULL;
        }
#endif
        munmap(used, used_bytes);
        munmap(base, g_hz3_arena.size);
        g_hz3_arena.used = NULL;
        g_hz3_arena.slots = 0;
        atomic_store_explicit(&g_hz3_arena_base, NULL, memory_order_release);
        return;
#endif
    } else {
        g_hz3_page_tag32 = (_Atomic(uint32_t)*)page_tag32;
#if HZ3_PTAG32_HUGEPAGE
#ifdef MADV_HUGEPAGE
        // S19-1: Hint THP for random PTAG32 lookups (init-only).
        // Best-effort: ignore failures (older kernels / policies).
        (void)madvise(page_tag32, page_tag32_bytes, MADV_HUGEPAGE);
#endif
#endif
        // Touching is not strictly required; keep init cost minimal.
        // (mmap returns zeroed pages; faults occur naturally on first use)
    }
#endif

    // Compute arena end
    void* end = (char*)base + g_hz3_arena.size;

    // Atomic publish order: end FIRST, then base
    // This ensures readers who see base will also see a valid end
    atomic_store_explicit(&g_hz3_arena_end, end, memory_order_release);
    atomic_store_explicit(&g_hz3_arena_base, base, memory_order_release);
}

void hz3_arena_init_slow(void) {
    pthread_once(&g_hz3_arena_once, hz3_arena_do_init);
}

// ----------------------------------------------------------------------------
// S45: Arena accessor API (for mem_budget segment scan)
// ----------------------------------------------------------------------------
void* hz3_arena_get_base(void) {
    return atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
}

uint32_t hz3_arena_slots(void) {
    return g_hz3_arena.slots;
}

int hz3_arena_slot_used(uint32_t idx) {
    if (idx >= g_hz3_arena.slots) {
        return 0;
    }
    return atomic_load_explicit(&g_hz3_arena.used[idx], memory_order_relaxed);
}

// Fast path: reads base atomically, returns 0 if not initialized.
// Does NOT call pthread_once (safe for free hot path).
// NOTE: This function reads used[] - for PageTagMap hot path, use
// hz3_arena_page_index_fast() instead (no used[] read).
int hz3_arena_contains_fast(const void* ptr, uint32_t* idx_out, void** base_out) {
    // Acquire load to synchronize with release store in do_init
    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        return 0;  // Not initialized yet, false negative is OK
    }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t base = (uintptr_t)base_ptr;
    if (addr < base || addr >= base + g_hz3_arena.size) {
        return 0;
    }

    uint32_t idx = (uint32_t)((addr - base) / HZ3_SEG_SIZE);
    if (idx >= g_hz3_arena.slots) {
        return 0;
    }

    if (!atomic_load_explicit(&g_hz3_arena.used[idx], memory_order_acquire)) {
        return 0;
    }

    if (idx_out) {
        *idx_out = idx;
    }
    if (base_out) {
        *base_out = (void*)(base + (uintptr_t)idx * HZ3_SEG_SIZE);
    }
    return 1;
}

// Legacy contains: calls init_slow first, then fast path.
int hz3_arena_contains(const void* ptr, uint32_t* idx_out, void** base_out) {
    hz3_arena_init_slow();
    return hz3_arena_contains_fast(ptr, idx_out, base_out);
}

#if HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
// S12-4B: Clear page tags for a segment (all pages in 2MB segment)
// Called when segment is freed/recycled to prevent false positives
void hz3_arena_clear_segment_tags(void* seg_base) {
#if HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_DSTBIN_ENABLE
    if (!g_hz3_page_tag && !g_hz3_page_tag32) {
        return;  // PTAG not initialized
    }
#elif HZ3_SMALL_V2_PTAG_ENABLE
    if (!g_hz3_page_tag) {
        return;  // PTAG not initialized
    }
#else
    if (!g_hz3_page_tag32) {
        return;  // PTAG32 not initialized
    }
#endif
    uint32_t page_idx;
    if (!hz3_arena_page_index_fast(seg_base, &page_idx)) {
        return;  // Not in arena
    }
    // Clear all pages in segment (2MB / 4KB = 512 pages)
    uint32_t pages_per_seg = HZ3_SEG_SIZE >> HZ3_ARENA_PAGE_SHIFT;
#if HZ3_SMALL_V2_PTAG_ENABLE
    for (uint32_t i = 0; i < pages_per_seg; i++) {
        if (g_hz3_page_tag) {
            hz3_pagetag_clear(page_idx + i);
        }
    }
#endif

#if HZ3_PTAG_DSTBIN_ENABLE
    if (g_hz3_page_tag32) {
        for (uint32_t i = 0; i < pages_per_seg; i++) {
            hz3_pagetag32_clear(page_idx + i);
        }
    }
#endif
}
#endif

// S45: Slot search helper (extracted for reclaim retry)
static void* hz3_arena_try_alloc_slot(uint32_t* idx_out) {
    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        return NULL;
    }

    pthread_mutex_lock(&g_hz3_arena_lock);
    uint32_t start = g_hz3_arena.alloc_cursor;
    for (uint32_t pass = 0; pass < 2; pass++) {
        uint32_t begin = (pass == 0) ? start : 0;
        uint32_t end = (pass == 0) ? g_hz3_arena.slots : start;
        for (uint32_t i = begin; i < end; i++) {
            if (atomic_load_explicit(&g_hz3_arena.used[i], memory_order_relaxed)) {
                continue;
            }
            void* addr = (char*)base_ptr + (size_t)i * HZ3_SEG_SIZE;
            void* mapped = mmap(addr, HZ3_SEG_SIZE, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                                -1, 0);
            if (mapped == MAP_FAILED) {
                continue;
            }
            atomic_store_explicit(&g_hz3_arena.used[i], 1, memory_order_release);
#if HZ3_S47_SEGMENT_QUARANTINE
            atomic_fetch_add_explicit(&g_hz3_arena_used_slots, 1, memory_order_relaxed);
#endif
            g_hz3_arena.alloc_cursor = (i + 1 < g_hz3_arena.slots) ? (i + 1) : 0;
            pthread_mutex_unlock(&g_hz3_arena_lock);
            if (idx_out) {
                *idx_out = i;
            }
            return mapped;
        }
    }
    pthread_mutex_unlock(&g_hz3_arena_lock);
    return NULL;
}

#if HZ3_S47_ARENA_GATE
// S47-2: ArenaGateBox - pressure compaction (called by leader or follower rescue)
// TLS safety: checks t_hz3_cache.initialized before using shard
static void run_pressure_compaction(void) {
#if HZ3_S47_SEGMENT_QUARANTINE
    // TLS safe: only call shard-dependent functions if initialized
    if (t_hz3_cache.initialized) {
        hz3_s47_policy_on_alloc_full();
        hz3_s47_compact_hard(t_hz3_cache.my_shard);
    }
#endif

#if HZ3_ARENA_PRESSURE_BOX
    // Broadcast pressure to all threads
    atomic_fetch_add_explicit(&g_hz3_arena_pressure_epoch, 1, memory_order_release);

    // Yield + focused reclaim (no try_alloc here - caller handles retry)
    for (int retry = 0; retry < 32; retry++) {
        sched_yield();
#if HZ3_MEM_BUDGET_ENABLE && HZ3_S45_FOCUS_RECLAIM
        for (int pass = 0; pass < 8; pass++) {
            Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();
            if (prog.sampled == 0) break;
        }
#endif
    }
#endif

#if HZ3_MEM_BUDGET_ENABLE
    // Emergency flush
    hz3_mem_budget_emergency_flush();

#if HZ3_S45_EMERGENCY_FLUSH_REMOTE && HZ3_PTAG_DSTBIN_ENABLE
    // TLS safe: remote flush requires initialized TLS
    if (t_hz3_cache.initialized) {
        hz3_dstbin_flush_remote_all();
    }
#endif

#if HZ3_S45_FOCUS_RECLAIM
    // Multi-pass focused reclaim
    for (int pass = 0; pass < HZ3_S45_FOCUS_PASSES; pass++) {
        Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();
        if (prog.sampled == 0 || prog.freed_pages == 0) {
            break;
        }
    }
#endif

    // Medium runs reclaim
    (void)hz3_mem_budget_reclaim_medium_runs();

    // Segment reclaim
    (void)hz3_mem_budget_reclaim_segments(HZ3_SEG_SIZE);
#endif
}
#endif  // HZ3_S47_ARENA_GATE

void* hz3_arena_alloc(uint32_t* idx_out) {
    hz3_arena_init_slow();

    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        hz3_oom_note("arena_alloc_base_null", g_hz3_arena.slots, g_hz3_arena.size);
        return NULL;
    }

#if HZ3_S47_SEGMENT_QUARANTINE
    // S47: Headroom check - trigger soft compact before exhaustion
    // Only run when TLS is initialized (original behavior)
    if (t_hz3_cache.initialized) {
        uint32_t used = hz3_arena_used_slots();
        uint32_t total = hz3_arena_total_slots();
        uint32_t headroom = hz3_s47_policy_get_headroom_slots();
        if (used >= total - headroom) {
            hz3_s47_compact_soft(t_hz3_cache.my_shard);
        }
    }
#endif

    // First attempt
    void* seg = hz3_arena_try_alloc_slot(idx_out);
    if (seg) {
        return seg;
    }

#if HZ3_S47_ARENA_GATE
    // S47-2: ArenaGateBox - leader election with retry loop
    for (int gate_attempt = 0; gate_attempt < 4; gate_attempt++) {
        uint32_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(
                &g_hz3_arena_gate, &expected, 1,
                memory_order_acquire, memory_order_relaxed)) {
            // ===== Leader =====
            atomic_fetch_add_explicit(&g_hz3_gate_leader_count, 1, memory_order_relaxed);
            uint32_t gen_before = hz3_arena_free_gen();
            run_pressure_compaction();
#if HZ3_OOM_SHOT && HZ3_S47_SEGMENT_QUARANTINE
            // S47-4: One-shot snapshot after pressure compaction
            {
                static _Atomic(int) snapshot_done = 0;
                if (atomic_exchange_explicit(&snapshot_done, 1, memory_order_relaxed) == 0) {
                    fprintf(stderr, "[SNAPSHOT] max_potential=%u pinned=%u avoid_hits=%u avoid_used=%u\n",
                            hz3_s47_get_max_potential(),
                            hz3_s47_get_pinned_candidates(),
                            hz3_s47_get_avoid_hits(),
                            hz3_s47_get_avoid_used());
                }
            }
#endif
            if (hz3_arena_free_gen() != gen_before) {
                atomic_fetch_add_explicit(&g_hz3_gate_seg_freed, 1, memory_order_relaxed);
            }
            // Gate release (must always execute)
            atomic_store_explicit(&g_hz3_arena_gate, 0, memory_order_release);
        } else {
            // ===== Follower =====
            // Light compaction: flush local bins to feed central
#if HZ3_MEM_BUDGET_ENABLE
            hz3_mem_budget_emergency_flush();
#if HZ3_S45_EMERGENCY_FLUSH_REMOTE && HZ3_PTAG_DSTBIN_ENABLE
            if (t_hz3_cache.initialized) {
                hz3_dstbin_flush_remote_all();
            }
#endif
#endif
            // Wait for free_gen change OR gate release
            uint32_t gen0 = hz3_arena_free_gen();
            int wait_ok = 0;
            for (int i = 0; i < HZ3_S47_GATE_WAIT_LOOPS; i++) {
                if (hz3_arena_free_gen() != gen0) {
                    wait_ok = 1;
                    break;  // Segment freed
                }
                if (atomic_load_explicit(&g_hz3_arena_gate, memory_order_relaxed) == 0) {
                    break;  // Leader finished, retry election
                }
                sched_yield();
            }
            if (wait_ok) {
                atomic_fetch_add_explicit(&g_hz3_gate_follower_wait_ok, 1, memory_order_relaxed);
            } else {
                atomic_fetch_add_explicit(&g_hz3_gate_follower_timeout, 1, memory_order_relaxed);
            }
        }
        // Try alloc after each gate attempt
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) {
            return seg;
        }
    }
#else  // !HZ3_S47_ARENA_GATE
#if HZ3_S47_SEGMENT_QUARANTINE
    // S47-PolicyBox: Aggressive drain before PRESSURE_BOX
    // Only run when TLS is initialized
    if (t_hz3_cache.initialized) {
        hz3_s47_policy_on_alloc_full();
        hz3_s47_compact_hard(t_hz3_cache.my_shard);
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) {
            hz3_s47_policy_on_wait_success();
            return seg;
        }
    }
#endif

#if HZ3_ARENA_PRESSURE_BOX
    // Pressure Box: broadcast arena exhaustion to all threads via epoch.
    // Other threads respond in their fast path by dropping to slow path and flushing.
    atomic_fetch_add_explicit(&g_hz3_arena_pressure_epoch, 1, memory_order_release);

    // Yield + aggressive focused reclaim: give threads time to respond, then reclaim.
    for (int retry = 0; retry < 32; retry++) {
        sched_yield();
#if HZ3_MEM_BUDGET_ENABLE && HZ3_S45_FOCUS_RECLAIM
        // Multiple passes of focused reclaim per yield (objects flow in gradually)
        for (int pass = 0; pass < 8; pass++) {
            Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();
            if (prog.freed_bytes > 0) {
                seg = hz3_arena_try_alloc_slot(idx_out);
                if (seg) return seg;
            }
            if (prog.sampled == 0) break;
        }
#endif
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) return seg;
    }
#endif

#if HZ3_MEM_BUDGET_ENABLE
    // Emergency flush (event-only): feed central before reclaim.
    hz3_mem_budget_emergency_flush();

#if HZ3_S45_EMERGENCY_FLUSH_REMOTE && HZ3_PTAG_DSTBIN_ENABLE
    // S45-FOCUS: Flush remote stash to central (increase sampling pool)
    // TLS guard: remote flush touches TLS, skip if not initialized
    if (t_hz3_cache.initialized) {
        hz3_dstbin_flush_remote_all();
    }
#endif

#if HZ3_S45_FOCUS_RECLAIM
    // S45-FOCUS: Multi-pass focused reclaim
    // Each pass samples from central and focuses on ONE segment
    // Progress tracking: sampled==0 || freed_pages==0 → break immediately
    for (int pass = 0; pass < HZ3_S45_FOCUS_PASSES; pass++) {
        Hz3FocusProgress prog = hz3_mem_budget_reclaim_focus_one_segment();

        // Check for segment release success
        if (prog.freed_bytes > 0) {
            seg = hz3_arena_try_alloc_slot(idx_out);
            if (seg) {
                return seg;
            }
            // Slot taken by another thread → fall through to Phase 2
            break;
        }

        // Progress check: no samples OR no pages freed → stop early
        if (prog.sampled == 0 || prog.freed_pages == 0) {
            break;
        }
        // sampled > 0 && freed_pages > 0 but segment not released
        // → continue to next pass (pages accumulate in segment)
    }
#endif

    // S45 Phase 2: Pop medium runs from central and return to segments
    // This increases segment free_pages, enabling Phase 1 reclaim
    (void)hz3_mem_budget_reclaim_medium_runs();

    // S45 Phase 1: Free fully-empty segments back to arena
    size_t reclaimed = hz3_mem_budget_reclaim_segments(HZ3_SEG_SIZE);
    if (reclaimed > 0) {
        seg = hz3_arena_try_alloc_slot(idx_out);
        if (seg) {
            return seg;
        }
    }
#endif
#endif  // HZ3_S47_ARENA_GATE

#if HZ3_S47_SEGMENT_QUARANTINE
    // S47-PolicyBox: Signal final allocation failure
    hz3_s47_policy_on_alloc_failed();
#endif

#if HZ3_OOM_SHOT || HZ3_OOM_FAILFAST
    if (atomic_exchange_explicit(&g_hz3_arena_alloc_full_diag_fired, 1, memory_order_relaxed) == 0) {
        uint32_t used_slots = 0;
        uint32_t total_slots = g_hz3_arena.slots;
        uint32_t headroom_slots = 0;
        uint32_t free_gen = 0;
        uint32_t max_potential = 0;
        uint32_t pinned_candidates = 0;
        uint32_t avoid_hits = 0;
        uint32_t avoid_used = 0;
        uint32_t gate_leader = 0;
        uint32_t gate_ok = 0;
        uint32_t gate_timeout = 0;
        uint32_t gate_seg_freed = 0;
        uint32_t pressure_epoch = 0;
        uint32_t pack_hits = 0;
        uint32_t pack_misses = 0;

#if HZ3_S47_SEGMENT_QUARANTINE
        used_slots = hz3_arena_used_slots();
        total_slots = hz3_arena_total_slots();
        headroom_slots = hz3_s47_policy_get_headroom_slots();
        free_gen = hz3_arena_free_gen();
        max_potential = hz3_s47_get_max_potential();
        pinned_candidates = hz3_s47_get_pinned_candidates();
        avoid_hits = hz3_s47_get_avoid_hits();
        avoid_used = hz3_s47_get_avoid_used();
#endif

#if HZ3_S47_ARENA_GATE
        gate_leader = atomic_load_explicit(&g_hz3_gate_leader_count, memory_order_relaxed);
        gate_ok = atomic_load_explicit(&g_hz3_gate_follower_wait_ok, memory_order_relaxed);
        gate_timeout = atomic_load_explicit(&g_hz3_gate_follower_timeout, memory_order_relaxed);
        gate_seg_freed = atomic_load_explicit(&g_hz3_gate_seg_freed, memory_order_relaxed);
#endif

#if HZ3_ARENA_PRESSURE_BOX
        pressure_epoch = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_relaxed);
#endif

#if HZ3_S49_SEGMENT_PACKING
        pack_hits = hz3_pack_get_hits();
        pack_misses = hz3_pack_get_misses();
#endif

        fprintf(stderr,
                "[HZ3_ARENA_ALLOC_FULL] used=%u/%u headroom=%u seg_size=0x%llx pages_per_seg=%u "
                "max_potential=%u pinned=%u avoid_hits=%u avoid_used=%u "
                "gate_leader=%u gate_ok=%u gate_timeout=%u gate_seg_freed=%u free_gen=%u "
                "pressure_epoch=%u pack_hit=%u pack_miss=%u\n",
                used_slots, total_slots, headroom_slots,
                (unsigned long long)HZ3_SEG_SIZE, (unsigned)HZ3_PAGES_PER_SEG,
                max_potential, pinned_candidates, avoid_hits, avoid_used,
                gate_leader, gate_ok, gate_timeout, gate_seg_freed, free_gen,
                pressure_epoch, pack_hits, pack_misses);
    }
#endif  // HZ3_OOM_SHOT || HZ3_OOM_FAILFAST

    hz3_oom_note("arena_alloc_full", g_hz3_arena.slots, g_hz3_arena.size);
    return NULL;
}

// S12-5A: Free a 2MB slot inside arena
// Uses madvise(MADV_DONTNEED) to release physical memory while keeping virtual address
void hz3_arena_free(uint32_t idx) {
    if (idx >= g_hz3_arena.slots) {
        return;
    }

    void* base_ptr = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base_ptr) {
        return;
    }

    pthread_mutex_lock(&g_hz3_arena_lock);
    if (atomic_load_explicit(&g_hz3_arena.used[idx], memory_order_relaxed)) {
        void* addr = (char*)base_ptr + (size_t)idx * HZ3_SEG_SIZE;
        // madvise(MADV_DONTNEED): release physical memory, keep virtual address
        madvise(addr, HZ3_SEG_SIZE, MADV_DONTNEED);
        atomic_store_explicit(&g_hz3_arena.used[idx], 0, memory_order_release);
        g_hz3_arena.alloc_cursor = idx;
#if HZ3_S47_SEGMENT_QUARANTINE
        atomic_fetch_sub_explicit(&g_hz3_arena_used_slots, 1, memory_order_relaxed);
        // S47-PolicyBox: Increment free_gen for bounded wait coordination
        atomic_fetch_add_explicit(&g_hz3_arena_free_gen, 1, memory_order_release);
#endif
    }
    pthread_mutex_unlock(&g_hz3_arena_lock);
}

#if HZ3_S47_SEGMENT_QUARANTINE
// S47: Accessor for used_slots (read outside lock is OK, relaxed)
uint32_t hz3_arena_used_slots(void) {
    return atomic_load_explicit(&g_hz3_arena_used_slots, memory_order_relaxed);
}

// S47: Accessor for total slots
uint32_t hz3_arena_total_slots(void) {
    return g_hz3_arena.slots;
}

// S47-PolicyBox: Accessor for free_gen (for bounded wait coordination)
uint32_t hz3_arena_free_gen(void) {
    return atomic_load_explicit(&g_hz3_arena_free_gen, memory_order_acquire);
}
#endif
