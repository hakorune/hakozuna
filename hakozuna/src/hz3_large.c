#define _GNU_SOURCE

#include "hz3_large.h"
#include "hz3_config.h"
#include "hz3_large_debug.h"
#include "hz3_large_internal.h"
#include "hz3_oom.h"
#include "hz3_watch_ptr.h"
#include "hz3_platform.h"
#include "hz3_tcache.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#if HZ3_S182_LARGE_CACHE_SPINLOCK
#if !defined(_WIN32)
#include <sched.h>
#endif
#endif

#define HZ3_LARGE_MAGIC 0x485a334c41524745ULL  // "HZ3LARGE"
#define HZ3_LARGE_HASH_BITS 10
#define HZ3_LARGE_HASH_SIZE (1u << HZ3_LARGE_HASH_BITS)

static inline size_t hz3_large_user_offset(void);
static inline int hz3_large_os_munmap(void* base, size_t bytes);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline void hz3_s276_direct_clear_retained(Hz3LargeHdr* hdr);
#endif

static Hz3LargeHdr* g_hz3_large_buckets[HZ3_LARGE_HASH_SIZE];
static hz3_lock_t g_hz3_large_map_locks[HZ3_S181_LARGE_MAP_LOCK_STRIPES] = {
    [0 ... HZ3_S181_LARGE_MAP_LOCK_STRIPES - 1] = HZ3_LOCK_INITIALIZER
};

#if HZ3_S182_LARGE_CACHE_SPINLOCK
static atomic_flag g_hz3_large_cache_spin = ATOMIC_FLAG_INIT;

static inline void hz3_large_spin_pause_hint(void) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#else
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

static inline void hz3_large_spin_thread_yield(void) {
#if defined(_WIN32)
    SwitchToThread();
#else
    sched_yield();
#endif
}

static inline void hz3_large_cache_lock_acquire(void) {
    uint32_t spins = 0;
#if HZ3_S182_SPIN_ADAPTIVE_YIELD
    uint32_t next_yield_at = HZ3_S182_SPIN_YIELD_START;
    uint32_t stage = 0;
#endif
    while (atomic_flag_test_and_set_explicit(&g_hz3_large_cache_spin, memory_order_acquire)) {
        ++spins;
#if HZ3_S182_SPIN_PAUSE_INTERVAL > 0
        if ((spins % HZ3_S182_SPIN_PAUSE_INTERVAL) == 0u) {
            hz3_large_spin_pause_hint();
        }
#endif
#if HZ3_S182_SPIN_ADAPTIVE_YIELD
        if (spins >= next_yield_at) {
            hz3_large_spin_thread_yield();
            if (stage < HZ3_S182_SPIN_ADAPTIVE_MAX_STAGE) {
                ++stage;
            }
            next_yield_at += (HZ3_S182_SPIN_YIELD_INTERVAL << stage);
        }
#else
        if (spins >= HZ3_S182_SPIN_YIELD_START &&
            ((spins - HZ3_S182_SPIN_YIELD_START) % HZ3_S182_SPIN_YIELD_INTERVAL) == 0u) {
            hz3_large_spin_thread_yield();
        }
#endif
    }
}

static inline void hz3_large_cache_lock_release(void) {
    atomic_flag_clear_explicit(&g_hz3_large_cache_spin, memory_order_release);
}
#else
static hz3_lock_t g_hz3_large_lock = HZ3_LOCK_INITIALIZER;

static inline void hz3_large_cache_lock_acquire(void) {
    hz3_lock_acquire(&g_hz3_large_lock);
}

static inline void hz3_large_cache_lock_release(void) {
    hz3_lock_release(&g_hz3_large_lock);
}
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
// S50: Per-class LIFO cache
static Hz3LargeHdr* g_sc_head[HZ3_LARGE_SC_COUNT];
static size_t g_sc_bytes[HZ3_LARGE_SC_COUNT];
static uint32_t g_sc_nodes[HZ3_LARGE_SC_COUNT];
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
static hz3_lock_t g_sc_locks[HZ3_LARGE_SC_COUNT] = {
    [0 ... HZ3_LARGE_SC_COUNT - 1] = HZ3_LOCK_INITIALIZER
};
static _Atomic size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#else
static size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
// Relaxed hint for free-path precheck (S184). Real admission is still checked under lock.
static _Atomic size_t g_total_cached_bytes_hint = 0;
#endif
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
static _Atomic size_t g_s253_global_cached_bytes_peak = 0;

static inline void hz3_s253_update_global_cached_peak(size_t value) {
    size_t cur = atomic_load_explicit(&g_s253_global_cached_bytes_peak,
                                      memory_order_relaxed);
    while (value > cur) {
        if (atomic_compare_exchange_weak_explicit(
                &g_s253_global_cached_bytes_peak,
                &cur,
                value,
                memory_order_relaxed,
                memory_order_relaxed)) {
            break;
        }
    }
}
#endif
#if HZ3_S207_TARGETED_REUSE
static _Atomic uint16_t g_sc_reuse_credit[HZ3_LARGE_SC_COUNT];
#endif
#else
// Legacy: single LIFO
static Hz3LargeHdr* g_hz3_large_free_head = NULL;
static size_t g_hz3_large_cached_bytes = 0;
static size_t g_hz3_large_cached_nodes = 0;
#endif
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
static inline size_t hz3_large_total_cached_load(void) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    return atomic_load_explicit(&g_total_cached_bytes, memory_order_relaxed);
#else
    return g_total_cached_bytes;
#endif
}

static inline void hz3_large_total_cached_add(size_t bytes) {
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    size_t current;
#endif
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    current = atomic_fetch_add_explicit(&g_total_cached_bytes,
                                        bytes,
                                        memory_order_relaxed) + bytes;
#else
    atomic_fetch_add_explicit(&g_total_cached_bytes,
                              bytes,
                              memory_order_relaxed);
#endif
#else
    g_total_cached_bytes += bytes;
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    current = g_total_cached_bytes;
#endif
#endif
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    hz3_s253_update_global_cached_peak(current);
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
    atomic_fetch_add_explicit(&g_total_cached_bytes_hint, bytes, memory_order_relaxed);
#endif
}

static inline void hz3_large_total_cached_sub(size_t bytes) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    atomic_fetch_sub_explicit(&g_total_cached_bytes, bytes, memory_order_relaxed);
#else
    g_total_cached_bytes -= bytes;
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
    atomic_fetch_sub_explicit(&g_total_cached_bytes_hint, bytes, memory_order_relaxed);
#endif
}

#if HZ3_S267_LARGE_RSS_PLATEAU_OBS
static _Atomic size_t g_s267_global_cached_bytes_peak = 0;
static _Atomic size_t g_s267_global_cached_nodes_current = 0;
static _Atomic size_t g_s267_global_cached_nodes_peak = 0;
static _Atomic size_t g_s267_sc_bytes_peak[HZ3_LARGE_SC_COUNT];
static _Atomic size_t g_s267_sc_nodes_peak[HZ3_LARGE_SC_COUNT];
static _Atomic size_t g_s267_defer_push = 0;
static _Atomic size_t g_s267_defer_pop = 0;
static _Atomic size_t g_s267_defer_reject = 0;
static _Atomic size_t g_s267_defer_bytes_current = 0;
static _Atomic size_t g_s267_defer_bytes_peak = 0;

static inline void hz3_s267_update_peak(_Atomic size_t* slot, size_t value) {
    size_t cur = atomic_load_explicit(slot, memory_order_relaxed);
    while (value > cur) {
        if (atomic_compare_exchange_weak_explicit(
                slot, &cur, value, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_s267_large_rss_dump_final(void) {
    size_t current_nodes =
        atomic_load_explicit(&g_s267_global_cached_nodes_current, memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_S267_LARGE_RSS] cache_enable=%d scache=%d cache_budget=%d "
            "soft_bytes=%zu hard_bytes=%zu cache_max_bytes=%zu cache_max_nodes=%u "
            "cached_bytes_current=%zu cached_bytes_peak=%zu cached_nodes_current=%zu "
            "cached_nodes_peak=%zu defer_push=%zu defer_pop=%zu defer_reject=%zu "
            "defer_bytes_current=%zu defer_bytes_peak=%zu s186_defer=%d "
            "s212_defer_plus=%d s212_defer_max_bytes=%zu s212_free_budget=%u "
            "s212_drain_on_miss=%d s212_miss_stride=%u s212_miss_budget=%u "
            "s212_trigger_bytes=%zu\n",
            HZ3_LARGE_CACHE_ENABLE,
            HZ3_S50_LARGE_SCACHE,
            HZ3_LARGE_CACHE_BUDGET,
            (size_t)HZ3_LARGE_CACHE_SOFT_BYTES,
            (size_t)HZ3_LARGE_CACHE_HARD_BYTES,
            (size_t)HZ3_LARGE_CACHE_MAX_BYTES,
            (unsigned)HZ3_LARGE_CACHE_MAX_NODES,
            hz3_large_total_cached_load(),
            atomic_load_explicit(&g_s267_global_cached_bytes_peak, memory_order_relaxed),
            current_nodes,
            atomic_load_explicit(&g_s267_global_cached_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_push, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_pop, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_bytes_peak, memory_order_relaxed),
            HZ3_S186_LARGE_UNMAP_DEFER,
            HZ3_S212_LARGE_UNMAP_DEFER_PLUS,
            (size_t)HZ3_S212_UNMAP_DEFER_MAX_BYTES,
            (unsigned)HZ3_S212_UNMAP_DEFER_FREE_BUDGET,
            HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS,
            (unsigned)HZ3_S212_DEFER_DRAIN_MISS_STRIDE,
            (unsigned)HZ3_S212_DEFER_DRAIN_MISS_BUDGET,
            (size_t)HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES);

    int emitted = 0;
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        size_t bytes_peak =
            atomic_load_explicit(&g_s267_sc_bytes_peak[sc], memory_order_relaxed);
        size_t nodes_peak =
            atomic_load_explicit(&g_s267_sc_nodes_peak[sc], memory_order_relaxed);
        if (g_sc_bytes[sc] == 0 && g_sc_nodes[sc] == 0 &&
            bytes_peak == 0 && nodes_peak == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S267_LARGE_RSS_SC] sc=%d bytes_current=%zu bytes_peak=%zu "
                "nodes_current=%u nodes_peak=%zu\n",
                sc,
                g_sc_bytes[sc],
                bytes_peak,
                g_sc_nodes[sc],
                nodes_peak);
        emitted = 1;
    }
    if (!emitted) {
        fprintf(stderr, "[HZ3_S267_LARGE_RSS_SC] empty=1\n");
    }
}

static inline void hz3_s267_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s267_large_rss_dump_final);
    }
}

static inline void hz3_s267_note_scache_state_locked(int sc) {
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return;
    }
    hz3_s267_register_once();
    hz3_s267_update_peak(&g_s267_global_cached_bytes_peak,
                         hz3_large_total_cached_load());
    hz3_s267_update_peak(&g_s267_sc_bytes_peak[sc], g_sc_bytes[sc]);
    hz3_s267_update_peak(&g_s267_sc_nodes_peak[sc], (size_t)g_sc_nodes[sc]);
}

static inline void hz3_s267_on_scache_push(int sc, size_t bytes) {
    (void)bytes;
    size_t nodes =
        atomic_fetch_add_explicit(&g_s267_global_cached_nodes_current,
                                  1,
                                  memory_order_relaxed) + 1;
    hz3_s267_update_peak(&g_s267_global_cached_nodes_peak, nodes);
    hz3_s267_note_scache_state_locked(sc);
}

static inline void hz3_s267_on_scache_pop(int sc, size_t bytes) {
    (void)bytes;
    atomic_fetch_sub_explicit(&g_s267_global_cached_nodes_current,
                              1,
                              memory_order_relaxed);
    hz3_s267_note_scache_state_locked(sc);
}

static inline void hz3_s267_on_defer_push(size_t bytes, size_t current_bytes) {
    hz3_s267_register_once();
    atomic_fetch_add_explicit(&g_s267_defer_push, 1, memory_order_relaxed);
    atomic_store_explicit(&g_s267_defer_bytes_current,
                          current_bytes,
                          memory_order_relaxed);
    hz3_s267_update_peak(&g_s267_defer_bytes_peak, current_bytes);
    (void)bytes;
}

static inline void hz3_s267_on_defer_pop(size_t bytes, size_t current_bytes) {
    hz3_s267_register_once();
    atomic_fetch_add_explicit(&g_s267_defer_pop, 1, memory_order_relaxed);
    atomic_store_explicit(&g_s267_defer_bytes_current,
                          current_bytes,
                          memory_order_relaxed);
    (void)bytes;
}

static inline void hz3_s267_on_defer_reject(size_t bytes, size_t current_bytes) {
    hz3_s267_register_once();
    atomic_fetch_add_explicit(&g_s267_defer_reject, 1, memory_order_relaxed);
    atomic_store_explicit(&g_s267_defer_bytes_current,
                          current_bytes,
                          memory_order_relaxed);
    (void)bytes;
}
#else
static inline void hz3_s267_on_scache_push(int sc, size_t bytes) {
    (void)sc;
    (void)bytes;
}

static inline void hz3_s267_on_scache_pop(int sc, size_t bytes) {
    (void)sc;
    (void)bytes;
}

static inline void hz3_s267_on_defer_push(size_t bytes, size_t current_bytes) {
    (void)bytes;
    (void)current_bytes;
}

static inline void hz3_s267_on_defer_pop(size_t bytes, size_t current_bytes) {
    (void)bytes;
    (void)current_bytes;
}

static inline void hz3_s267_on_defer_reject(size_t bytes, size_t current_bytes) {
    (void)bytes;
    (void)current_bytes;
}
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
static inline void hz3_large_sc_lock_acquire(int sc);
static inline void hz3_large_sc_lock_release(int sc);
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
static uint32_t g_s270_sc_pushes_since_pop[HZ3_LARGE_SC_COUNT];

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS
static _Atomic size_t g_s270_eligible_pushes = 0;
static _Atomic size_t g_s270_hot_skips = 0;
static _Atomic size_t g_s270_trigger_skips = 0;
static _Atomic size_t g_s270_trim_calls = 0;
static _Atomic size_t g_s270_trim_batches = 0;
static _Atomic size_t g_s270_trim_cap_hits = 0;
static _Atomic size_t g_s270_trimmed_nodes = 0;
static _Atomic size_t g_s270_trimmed_bytes = 0;
static _Atomic size_t g_s270_trimmed_nodes_by_sc[HZ3_LARGE_SC_COUNT];
static _Atomic size_t g_s270_trimmed_bytes_by_sc[HZ3_LARGE_SC_COUNT];
#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS
static _Atomic size_t g_s270_quarantine_victim_calls = 0;
static _Atomic size_t g_s270_quarantine_victim_bytes = 0;
#endif
#if HZ3_S270_DEBUG_VALIDATE
static _Atomic size_t g_s270_validate_calls = 0;
static _Atomic size_t g_s270_validate_failures = 0;
static _Atomic size_t g_s270_validate_node_failures = 0;
static _Atomic size_t g_s270_validate_count_mismatch = 0;
static _Atomic size_t g_s270_validate_bytes_mismatch = 0;
static _Atomic size_t g_s270_validate_cycle_failures = 0;
static _Atomic size_t g_s270_validate_long_walk_failures = 0;
static _Atomic size_t g_s270_validate_victim_still_linked = 0;
static _Atomic size_t g_s270_validate_victim_failures = 0;
static _Atomic size_t g_s270_dispose_victim_calls = 0;
#endif

static void hz3_s270_large_idle_trim_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S270_LARGE_SCACHE_IDLE_TRIM] enabled=1 sc_min=%d "
            "target_nodes=%u trigger_nodes=%u idle_pushes=%u batch_max=%u "
            "rearm_after_trim=%d stats=1 "
            "eligible_pushes=%zu hot_skips=%zu trigger_skips=%zu "
            "trim_calls=%zu trim_batches=%zu trim_cap_hits=%zu "
            "trimmed_nodes=%zu trimmed_bytes=%zu\n",
            HZ3_S270_SC_MIN,
            (unsigned)HZ3_S270_TARGET_NODES_PER_SC,
            (unsigned)HZ3_S270_TRIGGER_NODES_PER_SC,
            (unsigned)HZ3_S270_IDLE_PUSHES_PER_SC,
            (unsigned)HZ3_S270_TRIM_BATCH_MAX,
            HZ3_S270_REARM_AFTER_TRIM,
            atomic_load_explicit(&g_s270_eligible_pushes, memory_order_relaxed),
            atomic_load_explicit(&g_s270_hot_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trigger_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trim_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trim_batches, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trim_cap_hits, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trimmed_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trimmed_bytes, memory_order_relaxed));

    int emitted = 0;
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        size_t nodes =
            atomic_load_explicit(&g_s270_trimmed_nodes_by_sc[sc], memory_order_relaxed);
        size_t bytes =
            atomic_load_explicit(&g_s270_trimmed_bytes_by_sc[sc], memory_order_relaxed);
        if (nodes == 0 && bytes == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S270_LARGE_SCACHE_IDLE_TRIM_SC] sc=%d "
                "trimmed_nodes=%zu trimmed_bytes=%zu\n",
                sc,
                nodes,
                bytes);
        emitted = 1;
    }
    if (!emitted) {
        fprintf(stderr, "[HZ3_S270_LARGE_SCACHE_IDLE_TRIM_SC] empty=1\n");
    }
#if HZ3_S270_DEBUG_VALIDATE
    fprintf(stderr,
            "[HZ3_S270_DEBUG_VALIDATE] enabled=1 failfast=%d "
            "validate_calls=%zu failures=%zu node_failures=%zu "
            "count_mismatch=%zu bytes_mismatch=%zu cycle_failures=%zu "
            "long_walk_failures=%zu victim_still_linked=%zu victim_failures=%zu "
            "dispose_victim_calls=%zu\n",
            HZ3_S270_DEBUG_FAILFAST,
            atomic_load_explicit(&g_s270_validate_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_node_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_count_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_bytes_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_cycle_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_long_walk_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_victim_still_linked,
                                  memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_victim_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_dispose_victim_calls, memory_order_relaxed));
#endif
#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS
    fprintf(stderr,
            "[HZ3_S270_DEBUG_QUARANTINE_VICTIMS] enabled=1 calls=%zu bytes=%zu\n",
            atomic_load_explicit(&g_s270_quarantine_victim_calls,
                                  memory_order_relaxed),
            atomic_load_explicit(&g_s270_quarantine_victim_bytes,
                                  memory_order_relaxed));
#endif
}

static inline void hz3_s270_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s270_large_idle_trim_dump_final);
    }
}

static inline void hz3_s270_note_eligible_push(void) {
    atomic_fetch_add_explicit(&g_s270_eligible_pushes, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_trigger_skip(void) {
    atomic_fetch_add_explicit(&g_s270_trigger_skips, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_hot_skip(void) {
    atomic_fetch_add_explicit(&g_s270_hot_skips, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_trim_call(void) {
    atomic_fetch_add_explicit(&g_s270_trim_calls, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_trim_locked(int sc,
                                             size_t nodes,
                                             size_t bytes,
                                             int capped) {
    if (nodes == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s270_trimmed_nodes, nodes, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trimmed_bytes, bytes, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trimmed_nodes_by_sc[sc],
                              nodes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trimmed_bytes_by_sc[sc],
                              bytes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trim_batches, 1, memory_order_relaxed);
    if (capped) {
        atomic_fetch_add_explicit(&g_s270_trim_cap_hits, 1, memory_order_relaxed);
    }
}
#else
static inline void hz3_s270_register_once(void) {
}

static inline void hz3_s270_note_eligible_push(void) {
}

static inline void hz3_s270_note_trigger_skip(void) {
}

static inline void hz3_s270_note_hot_skip(void) {
}

static inline void hz3_s270_note_trim_call(void) {
}

static inline void hz3_s270_note_trim_locked(int sc,
                                             size_t nodes,
                                             size_t bytes,
                                             int capped) {
    (void)sc;
    (void)nodes;
    (void)bytes;
    (void)capped;
}
#endif

#if HZ3_S270_DEBUG_VALIDATE
static void hz3_s270_debug_fail(const char* where,
                                int sc,
                                const char* reason,
                                const Hz3LargeHdr* hdr,
                                size_t actual,
                                size_t expected,
                                _Atomic size_t* counter) {
    atomic_fetch_add_explicit(&g_s270_validate_failures, 1, memory_order_relaxed);
    if (counter) {
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    }
    fprintf(stderr,
            "[HZ3_S270_DEBUG_FAIL] where=%s sc=%d reason=%s hdr=%p "
            "actual=%zu expected=%zu map_base=%p map_size=%zu user_ptr=%p "
            "in_use=%u next_free=%p",
            where,
            sc,
            reason,
            (const void*)hdr,
            actual,
            expected,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0,
            hdr ? hdr->user_ptr : NULL,
            hdr ? (unsigned)hdr->in_use : 0u,
            hdr ? (void*)hdr->next_free : NULL);
#if HZ3_S240_LARGE_OWNER_FRONT
    fprintf(stderr,
            " state=%u owner=%u hdr_sc=%u flags=0x%x",
            hdr ? atomic_load_explicit(&hdr->state, memory_order_acquire) : 0u,
            hdr ? (unsigned)hdr->owner_shard : 255u,
            hdr ? (unsigned)hdr->sc : 255u,
            hdr ? (unsigned)hdr->flags : 0u);
#endif
    fprintf(stderr, "\n");
#if HZ3_S270_DEBUG_FAILFAST
    abort();
#endif
}

static int hz3_s270_validate_node_locked(int sc,
                                         const Hz3LargeHdr* hdr,
                                         const char* where) {
    if (!hdr) {
        hz3_s270_debug_fail(where,
                            sc,
                            "null-node",
                            hdr,
                            0,
                            1,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->magic != HZ3_LARGE_MAGIC) {
        hz3_s270_debug_fail(where,
                            sc,
                            "bad-magic",
                            hdr,
                            (size_t)hdr->magic,
                            (size_t)HZ3_LARGE_MAGIC,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->map_base != (const void*)hdr || hdr->map_size == 0) {
        hz3_s270_debug_fail(where,
                            sc,
                            "bad-map",
                            hdr,
                            hdr->map_size,
                            1,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->in_use != 0) {
        hz3_s270_debug_fail(where,
                            sc,
                            "in-use-cache-node",
                            hdr,
                            hdr->in_use,
                            0,
                            &g_s270_validate_node_failures);
        return 0;
    }
#if HZ3_S240_LARGE_OWNER_FRONT
    uint32_t state = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (state != HZ3_LARGE_STATE_GLOBAL) {
        hz3_s270_debug_fail(where,
                            sc,
                            "non-global-state",
                            hdr,
                            state,
                            HZ3_LARGE_STATE_GLOBAL,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->sc != UINT8_MAX && hdr->sc != (uint8_t)sc) {
        hz3_s270_debug_fail(where,
                            sc,
                            "sc-mismatch",
                            hdr,
                            hdr->sc,
                            (uint8_t)sc,
                            &g_s270_validate_node_failures);
        return 0;
    }
#endif
    return 1;
}

static void hz3_s270_validate_list_locked(int sc, const char* where) {
    atomic_fetch_add_explicit(&g_s270_validate_calls, 1, memory_order_relaxed);
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        hz3_s270_debug_fail(where,
                            sc,
                            "bad-sc",
                            NULL,
                            (size_t)sc,
                            HZ3_LARGE_SC_COUNT,
                            &g_s270_validate_node_failures);
        return;
    }

    Hz3LargeHdr* slow = g_sc_head[sc];
    Hz3LargeHdr* fast = g_sc_head[sc];
    while (fast && fast->next_free) {
        slow = slow ? slow->next_free : NULL;
        fast = fast->next_free->next_free;
        if (slow && fast && slow == fast) {
            hz3_s270_debug_fail(where,
                                sc,
                                "cycle",
                                slow,
                                0,
                                0,
                                &g_s270_validate_cycle_failures);
            return;
        }
    }

    size_t nodes = 0;
    size_t bytes = 0;
    size_t limit = (size_t)g_sc_nodes[sc] + HZ3_S270_TRIM_BATCH_MAX + 1024u;
    if (limit < 1024u) {
        limit = 1024u;
    }

    for (Hz3LargeHdr* cur = g_sc_head[sc]; cur; cur = cur->next_free) {
        if (++nodes > limit) {
            hz3_s270_debug_fail(where,
                                sc,
                                "walk-limit",
                                cur,
                                nodes,
                                limit,
                                &g_s270_validate_long_walk_failures);
            return;
        }
        if (!hz3_s270_validate_node_locked(sc, cur, where)) {
            return;
        }
        if (bytes + cur->map_size < bytes) {
            hz3_s270_debug_fail(where,
                                sc,
                                "bytes-overflow",
                                cur,
                                bytes,
                                cur->map_size,
                                &g_s270_validate_bytes_mismatch);
            return;
        }
        bytes += cur->map_size;
    }

    if (nodes != (size_t)g_sc_nodes[sc]) {
        hz3_s270_debug_fail(where,
                            sc,
                            "node-count-mismatch",
                            g_sc_head[sc],
                            nodes,
                            g_sc_nodes[sc],
                            &g_s270_validate_count_mismatch);
        return;
    }
    if (bytes != g_sc_bytes[sc]) {
        hz3_s270_debug_fail(where,
                            sc,
                            "byte-count-mismatch",
                            g_sc_head[sc],
                            bytes,
                            g_sc_bytes[sc],
                            &g_s270_validate_bytes_mismatch);
    }
}

static int hz3_s270_list_contains_locked(int sc, const Hz3LargeHdr* needle) {
    size_t limit = (size_t)g_sc_nodes[sc] + HZ3_S270_TRIM_BATCH_MAX + 1024u;
    size_t n = 0;
    for (Hz3LargeHdr* cur = g_sc_head[sc]; cur; cur = cur->next_free) {
        if (++n > limit) {
            return 0;
        }
        if (cur == needle) {
            return 1;
        }
    }
    return 0;
}

static void hz3_s270_validate_victims_detached_locked(int sc,
                                                      Hz3LargeHdr** victims,
                                                      size_t count,
                                                      const char* where) {
    for (size_t i = 0; i < count; i++) {
        Hz3LargeHdr* victim = victims[i];
        if (!hz3_s270_validate_node_locked(sc, victim, where)) {
            continue;
        }
        if (victim->next_free != NULL) {
            hz3_s270_debug_fail(where,
                                sc,
                                "victim-next-not-null",
                                victim,
                                (size_t)(uintptr_t)victim->next_free,
                                0,
                                &g_s270_validate_victim_failures);
        }
        if (hz3_s270_list_contains_locked(sc, victim)) {
            hz3_s270_debug_fail(where,
                                sc,
                                "victim-still-linked",
                                victim,
                                i,
                                count,
                                &g_s270_validate_victim_still_linked);
        }
    }
}

static void hz3_s270_validate_victim_before_dispose(Hz3LargeHdr* victim,
                                                    const char* where) {
    atomic_fetch_add_explicit(&g_s270_dispose_victim_calls, 1, memory_order_relaxed);
    if (!victim) {
        hz3_s270_debug_fail(where,
                            -1,
                            "dispose-null-victim",
                            victim,
                            0,
                            1,
                            &g_s270_validate_victim_failures);
        return;
    }
    int sc = -1;
#if HZ3_S240_LARGE_OWNER_FRONT
    sc = (victim->sc == UINT8_MAX) ? -1 : (int)victim->sc;
#endif
    (void)hz3_s270_validate_node_locked(sc, victim, where);
    if (victim->next_free != NULL) {
        hz3_s270_debug_fail(where,
                            sc,
                            "dispose-victim-next-not-null",
                            victim,
                            (size_t)(uintptr_t)victim->next_free,
                            0,
                            &g_s270_validate_victim_failures);
    }
}
#else
static inline void hz3_s270_validate_list_locked(int sc, const char* where) {
    (void)sc;
    (void)where;
}

static inline void hz3_s270_validate_victims_detached_locked(int sc,
                                                             Hz3LargeHdr** victims,
                                                             size_t count,
                                                             const char* where) {
    (void)sc;
    (void)victims;
    (void)count;
    (void)where;
}

static inline void hz3_s270_validate_victim_before_dispose(Hz3LargeHdr* victim,
                                                           const char* where) {
    (void)victim;
    (void)where;
}
#endif

static inline int hz3_s270_trim_sc(int sc) {
    return (sc >= HZ3_S270_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1));
}

static inline void hz3_s270_on_scache_pop_locked(int sc) {
    if (!hz3_s270_trim_sc(sc)) {
        return;
    }
    g_s270_sc_pushes_since_pop[sc] = 0;
}

static inline void hz3_s270_on_scache_push_locked(int sc) {
    if (!hz3_s270_trim_sc(sc)) {
        return;
    }
    hz3_s270_register_once();
    g_s270_sc_pushes_since_pop[sc]++;
}

static size_t hz3_s270_collect_idle_trim_locked(int sc,
                                                Hz3LargeHdr** victims,
                                                size_t cap) {
    if (!hz3_s270_trim_sc(sc) || cap == 0) {
        return 0;
    }
    hz3_s270_register_once();
    hz3_s270_note_eligible_push();

    if (g_sc_nodes[sc] <= (uint32_t)HZ3_S270_TRIGGER_NODES_PER_SC) {
        hz3_s270_note_trigger_skip();
        return 0;
    }
    if (g_s270_sc_pushes_since_pop[sc] < (uint32_t)HZ3_S270_IDLE_PUSHES_PER_SC) {
        hz3_s270_note_hot_skip();
        return 0;
    }

    hz3_s270_note_trim_call();
    hz3_s270_validate_list_locked(sc, "s270-pre-trim");

    uint32_t keep = (uint32_t)HZ3_S270_TARGET_NODES_PER_SC;
    if (keep >= g_sc_nodes[sc]) {
        return 0;
    }

    Hz3LargeHdr* keep_tail = g_sc_head[sc];
    for (uint32_t i = 1; keep_tail && i < keep; i++) {
        keep_tail = keep_tail->next_free;
    }
    if (!keep_tail || !keep_tail->next_free) {
        return 0;
    }

    Hz3LargeHdr* cur = keep_tail->next_free;
    keep_tail->next_free = NULL;

    size_t n = 0;
    size_t bytes = 0;
    while (cur && n < cap) {
        Hz3LargeHdr* next = cur->next_free;
        cur->next_free = NULL;
        victims[n++] = cur;
        bytes += cur->map_size;
        g_sc_bytes[sc] -= cur->map_size;
        g_sc_nodes[sc]--;
        hz3_large_total_cached_sub(cur->map_size);
        hz3_s267_on_scache_pop(sc, cur->map_size);
        hz3_large_debug_on_cache_remove_locked(cur);
        cur = next;
    }
    if (cur) {
        keep_tail->next_free = cur;
    }

    if (HZ3_S270_REARM_AFTER_TRIM &&
        g_sc_nodes[sc] > (uint32_t)HZ3_S270_TARGET_NODES_PER_SC) {
        g_s270_sc_pushes_since_pop[sc] =
            (uint32_t)HZ3_S270_IDLE_PUSHES_PER_SC - 1u;
    } else {
        g_s270_sc_pushes_since_pop[sc] = 0;
    }
    hz3_s270_note_trim_locked(sc, n, bytes, cur != NULL);
    hz3_s270_validate_victims_detached_locked(sc,
                                              victims,
                                              n,
                                              "s270-post-detach-victims");
    hz3_s270_validate_list_locked(sc, "s270-post-trim");
    return n;
}

static size_t hz3_s270_collect_idle_trim(int sc, Hz3LargeHdr** victims, size_t cap) {
    size_t n;
    hz3_large_sc_lock_acquire(sc);
    n = hz3_s270_collect_idle_trim_locked(sc, victims, cap);
    hz3_large_sc_lock_release(sc);
    return n;
}
#else
static inline void hz3_s270_on_scache_pop_locked(int sc) {
    (void)sc;
}
static inline void hz3_s270_on_scache_push_locked(int sc) {
    (void)sc;
}
static inline size_t hz3_s270_collect_idle_trim(int sc,
                                                Hz3LargeHdr** victims,
                                                size_t cap) {
    (void)sc;
    (void)victims;
    (void)cap;
    return 0;
}
#endif

#if HZ3_S184_LARGE_FREE_PRECHECK
static inline size_t hz3_large_total_cached_hint_load(void) {
    return atomic_load_explicit(&g_total_cached_bytes_hint, memory_order_relaxed);
}
#endif

static inline void hz3_large_sc_lock_acquire(int sc) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_lock_acquire(&g_sc_locks[sc]);
#else
    (void)sc;
#endif
}

static inline void hz3_large_sc_lock_release(int sc) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_lock_release(&g_sc_locks[sc]);
#else
    (void)sc;
#endif
}

static inline Hz3LargeHdr* hz3_large_sc_try_pop_locked(int sc, size_t need, int exact_fit) {
    Hz3LargeHdr* hdr = g_sc_head[sc];
    if (!hdr || hdr->magic != HZ3_LARGE_MAGIC) {
        return NULL;
    }
    if (exact_fit ? (hdr->map_size != need) : (hdr->map_size < need)) {
        return NULL;
    }
    g_sc_head[sc] = hdr->next_free;
    g_sc_bytes[sc] -= hdr->map_size;
    g_sc_nodes[sc]--;
    hz3_large_total_cached_sub(hdr->map_size);
    hz3_s267_on_scache_pop(sc, hdr->map_size);
    hz3_s270_on_scache_pop_locked(sc);
    hz3_large_debug_on_cache_remove_locked(hdr);
    return hdr;
}

static inline Hz3LargeHdr* hz3_large_sc_pop_head(int sc) {
    hz3_large_sc_lock_acquire(sc);
    Hz3LargeHdr* hdr = g_sc_head[sc];
    if (hdr) {
        g_sc_head[sc] = hdr->next_free;
        g_sc_bytes[sc] -= hdr->map_size;
        g_sc_nodes[sc]--;
        hz3_large_total_cached_sub(hdr->map_size);
        hz3_s267_on_scache_pop(sc, hdr->map_size);
        hz3_s270_on_scache_pop_locked(sc);
        hz3_large_debug_on_cache_remove_locked(hdr);
    }
    hz3_large_sc_lock_release(sc);
    return hdr;
}

static inline void hz3_large_sc_push_head(int sc, Hz3LargeHdr* hdr) {
    hz3_large_sc_lock_acquire(sc);
    hdr->next_free = g_sc_head[sc];
    g_sc_head[sc] = hdr;
    g_sc_bytes[sc] += hdr->map_size;
    g_sc_nodes[sc]++;
    hz3_large_total_cached_add(hdr->map_size);
    hz3_s267_on_scache_push(sc, hdr->map_size);
    hz3_s270_on_scache_push_locked(sc);
    hz3_large_debug_on_cache_insert_locked(hdr);
    hz3_large_sc_lock_release(sc);
}
#endif

#include "hz3_large_cache_policy.inc"

#include "hz3_large_map_ops.inc"

#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline int hz3_s276_try_activate_retained_from_cache(Hz3LargeHdr* hdr,
                                                            size_t size,
                                                            size_t alignment,
                                                            int sc,
                                                            int from_inbox) {
    if (!hz3_large_s276_can_activate_retained(hdr, size, alignment, sc)) {
        return 0;
    }
    hdr->in_use = 1;
    hdr->req_size = size;
    hdr->next_free = NULL;
    hz3_large_s276_on_activate_retained(hdr, sc, 1);
    if (from_inbox) {
        hz3_large_s276_note_inbox_activate_fast();
    } else {
        hz3_large_s276_note_front_activate_fast();
    }
    return 1;
}

static inline void hz3_s276_clear_retained_for_rekey(Hz3LargeHdr* hdr) {
    if (hdr && (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
        hz3_large_s276_note_front_activate_rekey();
        hz3_s276_direct_clear_retained(hdr);
    }
}
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
static inline void hz3_s270_dispose_detached_victim(Hz3LargeHdr* victim,
                                                    const char* where) {
    hz3_s270_validate_victim_before_dispose(victim, where);
    if (!victim) {
        return;
    }
#if HZ3_LARGE_DEBUG
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(victim);
    hz3_large_cache_lock_release();
#endif
#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS
    atomic_fetch_add_explicit(&g_s270_quarantine_victim_calls,
                              1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_quarantine_victim_bytes,
                              victim->map_size,
                              memory_order_relaxed);
    hz3_large_s240_on_munmap_store(victim);
    victim->next = NULL;
    victim->next_free = NULL;
    victim->magic = 0;
#else
    hz3_large_dispose_victim_unlocked(victim);
#endif
}
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
static inline int hz3_large_s218_c1_batch_enabled_for_sc(int sc) {
#if HZ3_S218_C1_LARGE_BATCH_MMAP
    return (sc >= HZ3_S218_C1_SC_MIN &&
            sc <= HZ3_S218_C1_SC_MAX &&
            sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline size_t hz3_large_s218_c1_seed_req_size(size_t class_size, size_t offset) {
    size_t reserve = offset;
#if HZ3_LARGE_CANARY_ENABLE
    reserve += HZ3_LARGE_CANARY_SIZE;
#endif
    if (class_size <= reserve) {
        return 1;
    }
    return class_size - reserve;
}

static Hz3LargeHdr* hz3_large_s218_c1_batch_mmap_alloc(size_t req_size,
                                                       size_t class_size,
                                                       size_t offset,
                                                       int sc,
                                                       size_t align_pad,
                                                       int aligned_alloc) {
#if HZ3_S218_C1_LARGE_BATCH_MMAP
    const size_t blocks = (size_t)HZ3_S218_C1_BATCH_BLOCKS;
    const size_t max_cache_per_batch = (size_t)HZ3_S218_C1_MAX_CACHE_PER_BATCH;
    if (!hz3_large_s218_c1_batch_enabled_for_sc(sc) || blocks < 2 || class_size == 0) {
        return NULL;
    }
    if (class_size > (SIZE_MAX / blocks)) {
        return NULL;
    }
    if (max_cache_per_batch == 0) {
        return NULL;
    }

    size_t hard_cap = hz3_large_hard_cap_for_sc(sc);
    const size_t seed_cap = (size_t)HZ3_S218_C1_SEED_CAP_BYTES;
    if (seed_cap > 0 && hard_cap > seed_cap) {
        hard_cap = seed_cap;
    }
    if (hard_cap < class_size) {
        return NULL;
    }
    // Require room for at least one seeded block; otherwise fallback to normal mmap.
    if ((hz3_large_total_cached_load() + class_size) > hard_cap) {
        return NULL;
    }

    const size_t total = class_size * blocks;
    void* region = hz3_large_os_mmap(total);
    if (region == MAP_FAILED) {
        return NULL;
    }
    if (aligned_alloc) {
        hz3_large_aligned_obs_on_aligned_batch_mmap(total);
    }

    uintptr_t user = (uintptr_t)region + offset;
    if (aligned_alloc && align_pad > 0) {
        user = (user + align_pad) & ~(uintptr_t)align_pad;
    }

    Hz3LargeHdr* hdr = (Hz3LargeHdr*)region;
    hdr->magic = HZ3_LARGE_MAGIC;
    hdr->req_size = req_size;
    hdr->map_base = region;
    hdr->map_size = class_size;
    hdr->user_ptr = (void*)user;
    hdr->next = NULL;
    hdr->in_use = 1;
    hdr->next_free = NULL;
    hz3_large_s240_on_activate(hdr, sc, 0);
#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif

    size_t cached_blocks = 0;
    size_t dropped_blocks = 0;
    size_t seed_req = hz3_large_s218_c1_seed_req_size(class_size, offset);
    void* dropped_bases[(HZ3_S218_C1_BATCH_BLOCKS > 1) ? (HZ3_S218_C1_BATCH_BLOCKS - 1) : 1];
    size_t dropped_count = 0;
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    Hz3LargeHdr* s270_victims[HZ3_S270_TRIM_BATCH_MAX];
    size_t s270_victim_count = 0;
#endif

#if !HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_large_cache_lock_acquire();
#endif

    for (size_t index = 1; index < blocks; index++) {
        void* extra_base = (void*)((char*)region + (index * class_size));
        Hz3LargeHdr* extra = (Hz3LargeHdr*)extra_base;
        extra->magic = HZ3_LARGE_MAGIC;
        extra->req_size = seed_req;
        extra->map_base = extra_base;
        extra->map_size = class_size;
        extra->user_ptr = (char*)extra_base + offset;
        extra->next = NULL;
        extra->in_use = 0;
        extra->next_free = NULL;
        hz3_large_s240_on_cache_seed(extra, sc);
#if HZ3_LARGE_CANARY_ENABLE
        hz3_large_debug_write_canary(extra);
#endif

        int can_cache = (cached_blocks < max_cache_per_batch);
        if (can_cache) {
            can_cache = (hz3_large_total_cached_load() + class_size <= hard_cap);
        }
        if (can_cache) {
            can_cache = hz3_large_reuse_try_admit_locked(sc);
        }
        if (can_cache) {
            hz3_large_s240_on_cache_store(extra, sc);
            hz3_large_sc_push_head(sc, extra);
            cached_blocks++;
        } else {
            dropped_bases[dropped_count++] = extra_base;
            dropped_blocks++;
        }
    }

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    s270_victim_count =
        hz3_s270_collect_idle_trim(sc, s270_victims, HZ3_S270_TRIM_BATCH_MAX);
#endif

#if !HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_large_cache_lock_release();
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    for (size_t index = 0; index < s270_victim_count; index++) {
        hz3_s270_dispose_detached_victim(s270_victims[index],
                                          "s270-batch-seed-dispose");
    }
#endif

    for (size_t index = 0; index < dropped_count; index++) {
        hz3_large_os_munmap(dropped_bases[index], class_size);
    }

    hz3_large_stats_on_s218_c1_batch(cached_blocks, dropped_blocks);
    return hdr;
#else
    (void)req_size;
    (void)class_size;
    (void)offset;
    (void)sc;
    (void)align_pad;
    (void)aligned_alloc;
    return NULL;
#endif
}
#endif

void* hz3_large_alloc(size_t size) {
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_alloc_call();

    if (size == 0) {
        size = 1;
    }
    hz3_large_aligned_obs_on_normal_alloc(size);

    size_t offset = hz3_large_user_offset();
    if (size > (SIZE_MAX - offset)) {
        return NULL;
    }

#if HZ3_LARGE_CANARY_ENABLE
    if (size > (SIZE_MAX - offset - HZ3_LARGE_CANARY_SIZE)) {
        return NULL;
    }
    size_t need = offset + size + HZ3_LARGE_CANARY_SIZE;
#else
    size_t need = offset + size;
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
    // S50: Round up to class size, then recompute sc from final size
    // This ensures alloc and free use the same class for the same block.
    int sc = hz3_large_sc(need);
    size_t class_size = hz3_large_sc_size(sc);
    if (class_size == 0) {
        // >64MB: キャッシュ対象外、実サイズ使用
        class_size = hz3_page_align(need);
    } else {
        class_size = hz3_page_align(class_size);
        // Recompute sc from page-aligned size to match what free() will compute
        sc = hz3_large_sc(class_size);
    }

    // Cache lookup: O(1) pop (sc < HZ3_LARGE_SC_COUNT-1 のみ)
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT - 1) {
        int try_sc = sc;
        Hz3LargeHdr* hdr = NULL;

#if HZ3_S240_LARGE_FRONT_CACHE
        hdr = hz3_large_s240_front_pop(sc, class_size);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, HZ3_LARGE_ALIGN, sc, 0)) {
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_alloc_front_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_front", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#if HZ3_S240_LARGE_OWNER_INBOX
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
        hdr = hz3_large_s246_inbox_drain_current_take_first_to_front(
            sc, (int)HZ3_S240_LARGE_INBOX_DRAIN_BATCH, size, NULL);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, HZ3_LARGE_ALIGN, sc, 1)) {
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_alloc_inbox_take_first_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_inbox_take_first", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#else
        hz3_large_s240_inbox_drain_current_to_front(
            sc, (int)HZ3_S240_LARGE_INBOX_DRAIN_BATCH);
        hdr = hz3_large_s240_front_pop(sc, class_size);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, HZ3_LARGE_ALIGN, sc, 1)) {
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_alloc_inbox_front_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_inbox_front", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#endif
#endif
#endif

        size_t obs_cache_pop_start_ns = hz3_large_aligned_obs_timing_start();

#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → sc+2)
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hz3_large_sc_lock_acquire(try_sc);
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            hz3_large_sc_lock_release(try_sc);
            if (hdr) {
                break;
            }
        }
#else
        hz3_large_sc_lock_acquire(sc);
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
        hz3_large_sc_lock_release(sc);
#endif
#else
        hz3_large_cache_lock_acquire();
#if HZ3_S52_LARGE_BESTFIT
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            if (hdr) {
                break;
            }
        }
#else
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
#endif
        hz3_large_cache_lock_release();
#endif
        hz3_large_aligned_obs_on_alloc_cache_pop_elapsed(obs_cache_pop_start_ns);

        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
            hz3_large_s251_on_global_pop(hdr);
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, try_sc, 1);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_alloc:
#endif
    hz3_large_stats_on_alloc_cache_miss();
    hz3_large_reuse_note_alloc_miss(sc);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    hz3_large_unmap_defer_drain_on_alloc_miss();
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#if HZ3_S193_DEMAND_SCAVENGE
    hz3_large_scavenge_on_mmap_miss(sc);
#endif
#else
    // Legacy: Cache から探す（first-fit）
    hz3_large_cache_lock_acquire();
    Hz3LargeHdr* prev = NULL;
    Hz3LargeHdr* cur = g_hz3_large_free_head;
    while (cur) {
        if (cur->map_size >= need) {
            // 見つかった: cache から外す
            if (prev) {
                prev->next_free = cur->next_free;
            } else {
                g_hz3_large_free_head = cur->next_free;
            }
            g_hz3_large_cached_bytes -= cur->map_size;
            g_hz3_large_cached_nodes--;
            hz3_large_s251_on_global_pop(cur);
            cur->in_use = 1;
            cur->req_size = size;
            cur->next_free = NULL;
            hz3_large_s240_on_activate(cur, hz3_large_sc(cur->map_size), 1);
            hz3_large_cache_lock_release();

            // bucket に再挿入（usable_size 等で参照される）
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(cur);
#endif
            hz3_large_insert(cur);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", cur->user_ptr, -1, -1);
#endif
            return cur->user_ptr;
        }
        prev = cur;
        cur = cur->next_free;
    }
    hz3_large_cache_lock_release();
#endif
#endif

    Hz3LargeHdr* hdr = NULL;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    hdr = hz3_large_s218_c1_batch_mmap_alloc(size, need, offset, sc, 0, 0);
#endif
    if (!hdr) {
        void* base = hz3_large_os_mmap(need);
        if (base == MAP_FAILED) {
            hz3_oom_note("large_mmap", need, 0);
            return NULL;
        }
        hdr = (Hz3LargeHdr*)base;
        hdr->magic = HZ3_LARGE_MAGIC;
        hdr->req_size = size;
        hdr->map_base = base;
        hdr->map_size = need;
        hdr->user_ptr = (char*)base + offset;
        hdr->next = NULL;
#if HZ3_LARGE_CACHE_ENABLE
        hdr->in_use = 1;
        hdr->next_free = NULL;
#endif
        hz3_large_s240_on_activate(hdr, sc, 0);
    }

#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif
    hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("large_alloc_mmap", hdr->user_ptr, -1, -1);
#endif
    return hdr->user_ptr;
}

void* hz3_large_aligned_alloc(size_t alignment, size_t size) {
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_alloc_call();

    if (alignment == 0 || (alignment & (alignment - 1u)) != 0) {
        hz3_large_aligned_obs_on_invalid(size, alignment);
        return NULL;
    }
    if (alignment < HZ3_LARGE_ALIGN) {
        alignment = HZ3_LARGE_ALIGN;
    }
    if (size == 0) {
        size = 1;
    }
    hz3_large_aligned_obs_on_aligned_alloc(size, alignment);

    size_t offset = hz3_large_user_offset();
    if (size > (SIZE_MAX - offset)) {
        return NULL;
    }
    size_t pad = alignment - 1u;
    if (size > (SIZE_MAX - offset - pad)) {
        return NULL;
    }
#if HZ3_LARGE_CANARY_ENABLE
    if (size > (SIZE_MAX - offset - pad - HZ3_LARGE_CANARY_SIZE)) {
        return NULL;
    }
    size_t need = offset + size + pad + HZ3_LARGE_CANARY_SIZE;
#else
    size_t need = offset + size + pad;
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    // S50: Round up to class size, then recompute sc from final size
    // This ensures alloc and free use the same class for the same block.
    int sc = hz3_large_sc(need);
    size_t class_size = hz3_large_sc_size(sc);
    if (class_size == 0) {
        // >64MB: キャッシュ対象外、実サイズ使用
        class_size = hz3_page_align(need);
    } else {
        class_size = hz3_page_align(class_size);
        // Recompute sc from page-aligned size to match what free() will compute
        sc = hz3_large_sc(class_size);
    }

    // Cache lookup: O(1) pop (sc < HZ3_LARGE_SC_COUNT-1 のみ)
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT - 1) {
        int try_sc = sc;
        Hz3LargeHdr* hdr = NULL;

#if HZ3_S240_LARGE_FRONT_CACHE
        size_t s243_front_first_start_ns = hz3_s243_residual_timing_start();
        hdr = hz3_large_s240_front_pop(sc, class_size);
        hz3_s243_residual_on_alloc_front_first_elapsed(
            s243_front_first_start_ns, hdr != NULL);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
            hz3_large_aligned_obs_on_aligned_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 1)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_aligned;
            }
#endif
            size_t s243_activate_start_ns = hz3_s243_residual_timing_start();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            size_t s276_meta_start_ns = hz3_s275_activation_timing_start();
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, alignment, sc, 0)) {
                hz3_s275_on_meta_front_elapsed(s276_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
                hz3_s243_residual_on_alloc_activate_front_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_aligned_front_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            size_t s275_userptr_start_ns = hz3_s275_activation_timing_start();
            uintptr_t start = (uintptr_t)hdr->map_base + offset;
            uintptr_t user = (start + pad) & ~(uintptr_t)pad;
            hz3_s275_on_userptr_front_elapsed(s275_userptr_start_ns);
            size_t s275_meta_start_ns = hz3_s275_activation_timing_start();
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->user_ptr = (void*)user;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
            hz3_s275_on_meta_front_elapsed(s275_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
            hz3_s243_residual_on_alloc_activate_front_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_aligned_front", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#if HZ3_S240_LARGE_OWNER_INBOX
        size_t s243_drain_start_ns = hz3_s243_residual_timing_start();
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
        int s243_drained = 0;
        hdr = hz3_large_s246_inbox_drain_current_take_first_to_front(
            sc, (int)HZ3_S240_LARGE_INBOX_DRAIN_BATCH, size, &s243_drained);
        hz3_s243_residual_on_alloc_inbox_drain_elapsed(
            s243_drain_start_ns, s243_drained);
        size_t s243_front_after_drain_start_ns = hz3_s243_residual_timing_start();
        hz3_s243_residual_on_alloc_front_after_drain_elapsed(
            s243_front_after_drain_start_ns, hdr != NULL);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
            hz3_large_aligned_obs_on_aligned_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 1)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_aligned;
            }
#endif
            size_t s243_activate_start_ns = hz3_s243_residual_timing_start();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            size_t s276_meta_start_ns = hz3_s275_activation_timing_start();
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, alignment, sc, 1)) {
                hz3_s275_on_meta_inbox_elapsed(s276_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
                hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_aligned_inbox_take_first_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            size_t s275_userptr_start_ns = hz3_s275_activation_timing_start();
            uintptr_t start = (uintptr_t)hdr->map_base + offset;
            uintptr_t user = (start + pad) & ~(uintptr_t)pad;
            hz3_s275_on_userptr_inbox_elapsed(s275_userptr_start_ns);
            size_t s275_meta_start_ns = hz3_s275_activation_timing_start();
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->user_ptr = (void*)user;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
            hz3_s275_on_meta_inbox_elapsed(s275_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
            hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_aligned_inbox_take_first", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#else
        int s243_drained = hz3_large_s240_inbox_drain_current_to_front(
            sc, (int)HZ3_S240_LARGE_INBOX_DRAIN_BATCH);
        hz3_s243_residual_on_alloc_inbox_drain_elapsed(
            s243_drain_start_ns, s243_drained);
        size_t s243_front_after_drain_start_ns = hz3_s243_residual_timing_start();
        hdr = hz3_large_s240_front_pop(sc, class_size);
        hz3_s243_residual_on_alloc_front_after_drain_elapsed(
            s243_front_after_drain_start_ns, hdr != NULL);
        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
            hz3_large_aligned_obs_on_aligned_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 1)) {
                hz3_large_debug_on_munmap_locked(hdr);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                hz3_s276_direct_clear_retained(hdr);
#endif
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_aligned;
            }
#endif
            size_t s243_activate_start_ns = hz3_s243_residual_timing_start();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            size_t s276_meta_start_ns = hz3_s275_activation_timing_start();
            if (hz3_s276_try_activate_retained_from_cache(hdr, size, alignment, sc, 1)) {
                hz3_s275_on_meta_inbox_elapsed(s276_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
                hz3_large_debug_write_canary(hdr);
#endif
                hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
                hz3_watch_ptr_on_alloc("large_aligned_inbox_front_retained", hdr->user_ptr, -1, -1);
#endif
                return hdr->user_ptr;
            }
            hz3_s276_clear_retained_for_rekey(hdr);
#endif
            size_t s275_userptr_start_ns = hz3_s275_activation_timing_start();
            uintptr_t start = (uintptr_t)hdr->map_base + offset;
            uintptr_t user = (start + pad) & ~(uintptr_t)pad;
            hz3_s275_on_userptr_inbox_elapsed(s275_userptr_start_ns);
            size_t s275_meta_start_ns = hz3_s275_activation_timing_start();
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->user_ptr = (void*)user;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, sc, 1);
            hz3_s275_on_meta_inbox_elapsed(s275_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
            hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_aligned_inbox_front", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
#endif
#endif
#endif

        size_t obs_cache_pop_start_ns = hz3_large_aligned_obs_timing_start();

#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → ...), aligned_alloc でも同じ方針
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hz3_large_sc_lock_acquire(try_sc);
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            hz3_large_sc_lock_release(try_sc);
            if (hdr) {
                break;
            }
        }
#else
        hz3_large_sc_lock_acquire(sc);
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
        hz3_large_sc_lock_release(sc);
#endif
#else
        hz3_large_cache_lock_acquire();
#if HZ3_S52_LARGE_BESTFIT
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            if (hdr) {
                break;
            }
        }
#else
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
#endif
        hz3_large_cache_lock_release();
#endif
        hz3_large_aligned_obs_on_alloc_cache_pop_elapsed(obs_cache_pop_start_ns);

        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
            hz3_large_aligned_obs_on_aligned_cache_hit();
            hz3_large_s251_on_global_pop(hdr);
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 1)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_aligned;
            }
#endif
            size_t s243_activate_start_ns = hz3_s243_residual_timing_start();
            // recalculate user_ptr for new alignment
            size_t s275_userptr_start_ns = hz3_s275_activation_timing_start();
            uintptr_t start = (uintptr_t)hdr->map_base + offset;
            uintptr_t user = (start + pad) & ~(uintptr_t)pad;
            hz3_s275_on_userptr_global_elapsed(s275_userptr_start_ns);
            size_t s275_meta_start_ns = hz3_s275_activation_timing_start();
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->user_ptr = (void*)user;
            hdr->next_free = NULL;
            hz3_large_s240_on_activate(hdr, try_sc, 1);
            hz3_s275_on_meta_global_elapsed(s275_meta_start_ns);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
            hz3_s243_residual_on_alloc_global_cache_hit_elapsed(s243_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_aligned_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_aligned:
#endif
    hz3_large_stats_on_alloc_cache_miss();
    hz3_large_aligned_obs_on_aligned_cache_miss();
    hz3_large_reuse_note_alloc_miss(sc);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    hz3_large_unmap_defer_drain_on_alloc_miss();
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#if HZ3_S193_DEMAND_SCAVENGE
    hz3_large_scavenge_on_mmap_miss(sc);
#endif
#endif

    Hz3LargeHdr* hdr = NULL;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    hdr = hz3_large_s218_c1_batch_mmap_alloc(size, need, offset, sc, pad, 1);
#endif
    if (!hdr) {
        void* base = hz3_large_os_mmap(need);
        if (base == MAP_FAILED) {
            hz3_oom_note("large_mmap", need, 0);
            return NULL;
        }
        hz3_large_aligned_obs_on_aligned_mmap(need);

        size_t s275_userptr_start_ns = hz3_s275_activation_timing_start();
        uintptr_t start = (uintptr_t)base + offset;
        uintptr_t user = (start + pad) & ~(uintptr_t)pad;
        hz3_s275_on_userptr_mmap_elapsed(s275_userptr_start_ns);

        size_t s275_meta_start_ns = hz3_s275_activation_timing_start();
        hdr = (Hz3LargeHdr*)base;
        hdr->magic = HZ3_LARGE_MAGIC;
        hdr->req_size = size;
        hdr->map_base = base;
        hdr->map_size = need;
        hdr->user_ptr = (void*)user;
        hdr->next = NULL;
#if HZ3_LARGE_CACHE_ENABLE
        hdr->in_use = 1;
        hdr->next_free = NULL;
#endif
        hz3_large_s240_on_activate(hdr, sc, 0);
        hz3_s275_on_meta_mmap_elapsed(s275_meta_start_ns);
    }

    size_t s243_mmap_activate_start_ns = hz3_s243_residual_timing_start();
#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif
    hz3_large_insert(hdr);
    hz3_s243_residual_on_alloc_mmap_activate_elapsed(s243_mmap_activate_start_ns);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("large_aligned_mmap", hdr->user_ptr, -1, -1);
#endif
    return hdr->user_ptr;
}

static inline int hz3_s281_retained_free_fast_route(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && \
    HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST
    if (!hdr || (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        return 0;
    }
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_size == 0) {
        return 0;
    }

    int sc = hz3_large_sc(hdr->map_size);
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT - 1) {
        return 0;
    }

    uint8_t cur = hz3_large_s240_current_owner();
    if (cur == UINT8_MAX || hdr->owner_shard == UINT8_MAX) {
        return 0;
    }

#if HZ3_S240_LARGE_FRONT_CACHE
    if (hdr->owner_shard == cur) {
        hdr->in_use = 0;
        hdr->next = NULL;
        if (hz3_large_s240_front_push(sc, hdr)) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            hz3_large_s276_note_front_push_retained();
#endif
            hz3_large_stats_on_free_cached();
            return 1;
        }
        return 0;
    }
#endif

#if HZ3_S240_LARGE_OWNER_INBOX
    if (hdr->owner_shard != cur) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        if (hz3_large_s240_inbox_push_remote(sc, hdr, cur)) {
            hz3_large_s276_note_inbox_push_retained();
            hz3_large_stats_on_free_cached();
            return 1;
        }
#endif
        return 0;
    }
#endif
#else
    (void)hdr;
#endif
    return 0;
}

int hz3_large_free(void* ptr) {
    if (!ptr) {
        return 0;
    }

    size_t s243_free_total_start_ns = hz3_s243_residual_timing_start();
    size_t s243_free_take_start_ns = hz3_s243_residual_timing_start();
    Hz3LargeHdr* hdr = hz3_large_take(ptr);
    hz3_s243_residual_on_free_take_elapsed(s243_free_take_start_ns, hdr != NULL);
    if (!hdr) {
        hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
        return 0;
    }
    hz3_large_aligned_obs_on_free(hdr);
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_free_call();
    size_t obs_free_admit_start_ns = hz3_large_aligned_obs_timing_start();
    size_t s243_free_fallback_start_ns = 0;
    int s243_free_fallback_active = 0;
    int s280_had_fast_route = 0;
    int s280_front_reject = 0;
    int s280_inbox_reject = 0;
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("large_free", ptr, -1, -1);
#endif
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && (HZ3_S186_LARGE_UNMAP_DEFER || HZ3_S212_LARGE_UNMAP_DEFER_PLUS)
    hz3_large_unmap_defer_drain_budget();
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST
    if (hz3_s281_retained_free_fast_route(hdr)) {
        return 1;
    }
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
    // Safety: validate hdr before accessing map_size
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_size == 0) {
        hz3_large_aligned_obs_on_admit_invalid();
        hz3_s280_free_route_on_sc_invalid();
        if (!s243_free_fallback_active) {
            s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
            s243_free_fallback_active = 1;
        }
        goto do_munmap;
    }

    int sc = hz3_large_sc(hdr->map_size);
    hz3_s280_free_route_on_taken(hdr);
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    Hz3LargeHdr* s270_victims[HZ3_S270_TRIM_BATCH_MAX];
    size_t s270_victim_count = 0;
#endif
    size_t s243_free_meta_start_ns = hz3_s243_residual_timing_start();
#if HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META
    if ((hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        hz3_large_s240_on_free_take(hdr, sc);
    }
#else
    hz3_large_s240_on_free_take(hdr, sc);
#endif
    hz3_s243_residual_on_free_s240_meta_elapsed(s243_free_meta_start_ns);

    // >64MB はキャッシュ対象外（巨大ブロックで cap を食い潰すのを防ぐ）
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT - 1) {
        hz3_large_aligned_obs_on_admit_oversize();
        hz3_s280_free_route_on_sc_oversize();
        if (!s243_free_fallback_active) {
            s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
            s243_free_fallback_active = 1;
        }
        goto do_munmap;
    }

#if HZ3_S240_LARGE_FRONT_CACHE
    uint8_t s240_cur_owner = hz3_large_s240_current_owner();
    if (hdr->owner_shard != UINT8_MAX && hdr->owner_shard == s240_cur_owner) {
        int s280_retained_route = (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0;
        s280_had_fast_route = 1;
        hz3_s280_free_route_on_same_owner();
        hdr->in_use = 0;
        hdr->next = NULL;
        size_t s243_front_push_start_ns = hz3_s243_residual_timing_start();
        int s243_front_pushed = hz3_large_s240_front_push(sc, hdr);
        hz3_s280_free_route_on_front_result(s280_retained_route,
                                            s243_front_pushed);
        hz3_s243_residual_on_free_front_push_elapsed(
            s243_front_push_start_ns, s243_front_pushed);
        if (s243_front_pushed) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if ((hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
                hz3_large_s276_note_front_push_retained();
            }
#endif
            hz3_large_stats_on_free_cached();
            hz3_large_aligned_obs_on_admit_cache_insert();
            hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
            hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
            return 1;
        }
        s280_front_reject = 1;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        if ((hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
            hz3_large_s276_note_front_push_reject_clear();
            hz3_s276_direct_clear_retained(hdr);
        }
#endif
    }
#if HZ3_S240_LARGE_OWNER_INBOX
    if (hdr->owner_shard != UINT8_MAX && hdr->owner_shard != s240_cur_owner) {
        int s280_inbox_retained_route = 0;
        s280_had_fast_route = 1;
        hz3_s280_free_route_on_remote_owner();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        int s276_try_inbox_retain =
            (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0 &&
            HZ3_S276_LARGE_DIRECT_RETAIN_INBOX;
        s280_inbox_retained_route = s276_try_inbox_retain;
        if (!s276_try_inbox_retain) {
            hz3_s276_direct_clear_retained(hdr);
        }
#endif
        size_t s243_inbox_push_start_ns = hz3_s243_residual_timing_start();
        int s243_inbox_pushed = hz3_large_s240_inbox_push_remote(sc, hdr, s240_cur_owner);
        hz3_s280_free_route_on_inbox_result(s280_inbox_retained_route,
                                            s243_inbox_pushed);
        hz3_s243_residual_on_free_inbox_push_elapsed(
            s243_inbox_push_start_ns, s243_inbox_pushed);
        if (s243_inbox_pushed) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (s276_try_inbox_retain) {
                hz3_large_s276_note_inbox_push_retained();
            }
#endif
            hz3_large_stats_on_free_cached();
            hz3_large_aligned_obs_on_admit_cache_insert();
            hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
            hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
            return 1;
        }
        s280_inbox_reject = 1;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        if (s276_try_inbox_retain) {
            hz3_large_s276_note_inbox_push_reject_clear();
            hz3_s276_direct_clear_retained(hdr);
        }
#endif
    }
#endif
#endif
#if HZ3_S240_LARGE_OWNER_FRONT
    if (hdr->owner_shard == UINT8_MAX) {
        hz3_s280_free_route_on_unknown_owner();
    }
#endif

    if (!s243_free_fallback_active) {
        hz3_s280_free_route_on_global_fallback(s280_had_fast_route,
                                               s280_front_reject,
                                               s280_inbox_reject);
        s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
        s243_free_fallback_active = 1;
    }

    // NOTE: If we apply madvise(MADV_DONTNEED) for the freed mapping, we MUST
    // not make it visible to other threads (cache lists) until after the
    // madvise is done. Otherwise another thread can reallocate the same mapping
    // and observe its contents being discarded while "in use".
    //
    // Therefore we:
    //   1) decide evict/can_cache + madvise range under lock
    //   2) unlock + (maybe) madvise
    //   3) lock again and insert into cache (evict again if needed)
    // S53/S207-1: hard cap (optionally widened for high-band classes).
    size_t hard_cap = hz3_large_hard_cap_for_sc(sc);
    size_t insert_cap = hz3_large_insert_cap_for_sc(sc, hard_cap);
    size_t evict_budget_left = hz3_large_evict_budget_for_sc(sc);
    int evict_budget_exhausted = 0;

    int can_cache = 0;
#if HZ3_S184_LARGE_FREE_PRECHECK
    int force_lock_admission = hz3_large_reuse_trackable_sc(sc);
#endif

    // S53: soft 判定と madvise 範囲は lock 内で計算してローカルへ退避
#if HZ3_LARGE_CACHE_BUDGET
    int do_madvise = 0;
    uintptr_t madvise_start = 0;
    size_t madvise_size = 0;
#endif

#if HZ3_S184_LARGE_FREE_PRECHECK
    // S184: lock前に relaxed hint で fast admit を先読み。
    // 最終判定は後段の lock 再取得時に必ず行う。
    if (!force_lock_admission) {
        size_t total_hint = hz3_large_total_cached_hint_load();
        size_t projected_hint = total_hint + hdr->map_size;
        if (projected_hint >= total_hint && projected_hint <= hard_cap) {
            can_cache = 1;
            hz3_large_aligned_obs_on_admit_precheck_yes();
#if HZ3_LARGE_CACHE_BUDGET
            do_madvise = (projected_hint > HZ3_LARGE_CACHE_SOFT_BYTES);
            if (do_madvise) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                madvise_start = (start + 4095) & ~(uintptr_t)4095;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (madvise_start < end) {
                    madvise_size = (end - madvise_start) & ~(size_t)4095;
                }
            }
#endif
        }
    }
#endif

    if (!can_cache) {
        hz3_large_cache_lock_acquire();

#if HZ3_S50_LARGE_SCACHE_EVICT
#if HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
        // Cap 超過時: evict してスペースを作る
        // S51: 同一 class を優先しつつ、victim をまとめて lock 外 munmap。
        for (;;) {
            Hz3LargeHdr* victims[HZ3_S185_EVICT_BATCH_MAX];
            size_t vcount = hz3_large_collect_evict_victims_locked(
                sc, hdr->map_size, hard_cap, victims, HZ3_S185_EVICT_BATCH_MAX, &evict_budget_left);
            if (vcount == 0) {
                break;
            }
            hz3_large_cache_lock_release();
            hz3_large_munmap_victims_unlocked(victims, vcount);
            hz3_large_cache_lock_acquire();
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#else
        // Cap 超過時: evict してスペースを作る
        // S51: 同一 class から先に evict（キャッシュ hit 率向上）
        while (hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
               evict_budget_left > 0) {
            Hz3LargeHdr* victim = NULL;
            // First: try same class
            victim = hz3_large_sc_pop_head(sc);
            if (!victim) {
                // Fallback: largest class
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    victim = hz3_large_sc_pop_head(i);
                    if (victim) {
                        break;
                    }
                }
            }
            if (!victim) break;  // evict 対象なし → munmap へ
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();  // hard 発火時のみ出力
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_large_cache_lock_release();
            hz3_large_dispose_victim_unlocked(victim);
            hz3_large_cache_lock_acquire();
            evict_budget_left--;
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#endif
#endif

        can_cache = (hz3_large_total_cached_load() + hdr->map_size <= insert_cap);
        hz3_large_aligned_obs_on_admit_lock_decision(can_cache);
#if HZ3_LARGE_CACHE_BUDGET
        if (can_cache && !do_madvise) {
            do_madvise = ((hz3_large_total_cached_load() + hdr->map_size) > HZ3_LARGE_CACHE_SOFT_BYTES);
            if (do_madvise) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                madvise_start = (start + 4095) & ~(uintptr_t)4095;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (madvise_start < end) {
                    madvise_size = (end - madvise_start) & ~(size_t)4095;
                }
            }
        }
#endif

#if HZ3_S191_LARGE_FREE_FAST_INSERT && !HZ3_S207_TARGETED_REUSE
        // S191: If no madvise work is needed, keep the first lock and insert directly.
        if (can_cache) {
            int needs_unlock_for_madvise = 0;
#if HZ3_LARGE_CACHE_BUDGET
            needs_unlock_for_madvise = (do_madvise && madvise_size > 0);
#elif HZ3_S51_LARGE_MADVISE
            needs_unlock_for_madvise = 1;
#endif
            if (!needs_unlock_for_madvise) {
                if (hz3_large_total_cached_load() + hdr->map_size <= insert_cap) {
                    hdr->in_use = 0;
                    hdr->next = NULL;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                    hz3_s276_direct_clear_retained(hdr);
#endif
                    hz3_large_s240_on_cache_store(hdr, sc);
                    hz3_large_sc_push_head(sc, hdr);
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
                    s270_victim_count =
                        hz3_s270_collect_idle_trim(sc,
                                                   s270_victims,
                                                   HZ3_S270_TRIM_BATCH_MAX);
#endif
                    hz3_large_stats_on_free_cached();
                    hz3_large_aligned_obs_on_admit_cache_insert();
                    hz3_large_cache_lock_release();
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
                    for (size_t i = 0; i < s270_victim_count; i++) {
                        hz3_s270_dispose_detached_victim(
                            s270_victims[i],
                            "s270-free-fast-insert-dispose");
                    }
#endif
                    hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
                    hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
                    hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
                    return 1;
                }
                hz3_large_aligned_obs_on_admit_final_fit_fail();
                can_cache = 0;
            }
        }
#endif

        hz3_large_cache_lock_release();
    }

    if (can_cache) {
        // S53: madvise は “cacheに入れる前” に実行する（reuse race 回避）
        // NOTE: hdr is not visible in any global list at this point.

        // S53: madvise は lock 外で実行
#if HZ3_LARGE_CACHE_BUDGET
#if HZ3_S193_DEMAND_SCAVENGE
        const int run_free_side_purge = 0;
#else
        const int run_free_side_purge = 1;
#endif
        if (run_free_side_purge && do_madvise && madvise_size > 0) {
#if HZ3_S53_THROTTLE
            // S53-2: Throttle check
            int do_actual_madvise = 1;
            uint32_t fc = atomic_fetch_add(&g_hz3_s53_free_counter, 1);
            if ((fc % HZ3_S53_MADVISE_MIN_INTERVAL) != 0) {
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_throttle_skips, 1);
#endif
                do_actual_madvise = 0;
                goto s53_done;
            }

            int64_t npages = (int64_t)(madvise_size >> 12);
            int64_t old_counter = atomic_fetch_sub(&g_hz3_s53_scavenge_counter_pages, npages);
            if (old_counter - npages >= 0) {
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_throttle_skips, 1);
#endif
                do_actual_madvise = 0;
                goto s53_done;
            }

            // 発火
            atomic_store(&g_hz3_s53_scavenge_counter_pages, HZ3_S53_MADVISE_RATE_PAGES);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_throttle_fires, 1);
#endif
s53_done:
            if (do_actual_madvise) {
                hz3_large_soft_purge((void*)madvise_start, madvise_size);
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
                atomic_fetch_add(&g_budget_soft_hits, 1);
#endif
            }
#if HZ3_LARGE_CACHE_STATS
            hz3_large_cache_stats_dump();
#endif
#else
            // THROTTLE=0: 従来通り毎回 madvise
            hz3_large_soft_purge((void*)madvise_start, madvise_size);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
            atomic_fetch_add(&g_budget_soft_hits, 1);
            hz3_large_cache_stats_dump();
#endif
#endif
        }
#elif HZ3_S51_LARGE_MADVISE
#if !HZ3_S193_DEMAND_SCAVENGE
        // S51: 既存の常時 madvise（BUDGET 無効時のみ）
        {
            uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
            uintptr_t aligned_start = (start + 4095) & ~(uintptr_t)4095;  // next page
            uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
            if (aligned_start < end) {
                size_t aligned_size = (end - aligned_start) & ~(size_t)4095;  // page truncate
                if (aligned_size > 0) {
                    hz3_large_soft_purge((void*)aligned_start, aligned_size);
                }
            }
        }
#endif
#endif
        // Finally: insert into cache (under lock). If we can no longer fit,
        // fall back to munmap (safe, but may lose caching benefits).
        hz3_large_cache_lock_acquire();

        // Recompute hard cap under lock (another thread may have changed totals)
        hard_cap = hz3_large_hard_cap_for_sc(sc);
        insert_cap = hz3_large_insert_cap_for_sc(sc, hard_cap);

#if HZ3_S50_LARGE_SCACHE_EVICT
#if HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
        for (;;) {
            Hz3LargeHdr* victims[HZ3_S185_EVICT_BATCH_MAX];
            size_t vcount = hz3_large_collect_evict_victims_locked(
                sc, hdr->map_size, hard_cap, victims, HZ3_S185_EVICT_BATCH_MAX, &evict_budget_left);
            if (vcount == 0) {
                break;
            }
            hz3_large_cache_lock_release();
            hz3_large_munmap_victims_unlocked(victims, vcount);
            hz3_large_cache_lock_acquire();
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#else
        while (hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
               evict_budget_left > 0) {
            Hz3LargeHdr* victim = NULL;
            victim = hz3_large_sc_pop_head(sc);
            if (!victim) {
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    victim = hz3_large_sc_pop_head(i);
                    if (victim) {
                        break;
                    }
                }
            }
            if (!victim) break;
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_large_cache_lock_release();
            hz3_large_dispose_victim_unlocked(victim);
            hz3_large_cache_lock_acquire();
            evict_budget_left--;
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#endif
#endif

        if (hz3_large_total_cached_load() + hdr->map_size <= insert_cap) {
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            if (hz3_large_total_cached_load() + hdr->map_size > hard_cap) {
                hz3_large_stat_inc(&g_s237_slack_admit_hits);
            }
#endif
            if (!hz3_large_reuse_try_admit_locked(sc)) {
                hz3_large_aligned_obs_on_admit_reuse_skip();
                hz3_large_cache_lock_release();
                goto do_munmap;
            }
            hdr->in_use = 0;
            hdr->next = NULL;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            hz3_s276_direct_clear_retained(hdr);
#endif
            hz3_large_s240_on_cache_store(hdr, sc);
            hz3_large_sc_push_head(sc, hdr);
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
            s270_victim_count =
                hz3_s270_collect_idle_trim(sc, s270_victims, HZ3_S270_TRIM_BATCH_MAX);
#endif
            hz3_large_stats_on_free_cached();
            hz3_large_aligned_obs_on_admit_cache_insert();
            hz3_large_cache_lock_release();
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
            for (size_t i = 0; i < s270_victim_count; i++) {
                hz3_s270_dispose_detached_victim(s270_victims[i],
                                                 "s270-free-final-insert-dispose");
            }
#endif
            hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
            hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
            hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
            return 1;
        }

        hz3_large_cache_lock_release();
        hz3_large_aligned_obs_on_admit_final_fit_fail();
        goto do_munmap;
    }

    // Cache できない（cap超過など）: munmap へ
#else
    // Legacy: 上限チェック
    hz3_large_cache_lock_acquire();
    if (g_hz3_large_cached_bytes + hdr->map_size <= HZ3_LARGE_CACHE_MAX_BYTES &&
        g_hz3_large_cached_nodes < HZ3_LARGE_CACHE_MAX_NODES) {
        // Cache に追加
        hdr->in_use = 0;
        hdr->next = NULL;           // bucket list から切断済み
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        hz3_s276_direct_clear_retained(hdr);
#endif
        hz3_large_s240_on_cache_store(hdr, hz3_large_sc(hdr->map_size));
        hdr->next_free = g_hz3_large_free_head;
        g_hz3_large_free_head = hdr;
        g_hz3_large_cached_bytes += hdr->map_size;
        g_hz3_large_cached_nodes++;
        hz3_large_stats_on_free_cached();
        hz3_large_aligned_obs_on_admit_cache_insert();
        hz3_large_cache_lock_release();
        hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
        hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
        hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
        return 1;
    }
    hz3_large_cache_lock_release();
#endif
#endif

#if HZ3_S50_LARGE_SCACHE && HZ3_LARGE_CACHE_ENABLE
do_munmap:
#endif
    if (!s243_free_fallback_active) {
        s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
        s243_free_fallback_active = 1;
    }
    // Cache full または無効: munmap
    hz3_large_aligned_obs_on_admit_do_munmap();
    hz3_s280_free_route_on_munmap();
    hz3_large_stats_on_free_munmap();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    hz3_s276_direct_clear_retained(hdr);
#endif
    hz3_large_s240_on_munmap_store(hdr);
    hdr->magic = 0;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    if (hz3_large_try_defer_direct_unmap(hdr)) {
        hz3_large_aligned_obs_on_admit_defer_munmap();
        hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
        hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
        hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
        return 1;
    }
#endif
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(hdr);
    hz3_large_cache_lock_release();
    hz3_large_os_munmap(hdr->map_base, hdr->map_size);
    hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
    hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
    hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
    return 1;
}

size_t hz3_large_usable_size(const void* ptr) {
    if (!ptr) {
        return 0;
    }

    size_t size = 0;
    if (hz3_large_find_size(ptr, &size)) {
        return size;
    }
    return 0;
}
