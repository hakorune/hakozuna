// hz3_s147_patch.inc - Mainline integration patches for S147 Reverse Mailbox
//
// This file documents the necessary patches to integrate S147 with mainline hz3.
// Copy/adapt these snippets to the appropriate files.
//
// ============================================================================
// PATCH 1: hz3_config_scale.h - Add S147 feature flag and config
// ============================================================================
//
// Add near the end of hz3_config_scale.h (after S142):
//
// ```c
// // ============================================================================
// // S147: Reverse Mailbox (Pull-based remote free)
// // ============================================================================
// //
// // Motivation:
// // - S146-B (CAS batch) was NO-GO: link overhead > CAS savings
// // - perf: hz3_owner_stash_push_one is 16.38% of cycles
// //        lock cmpxchg is 96.95% of push_one time (memory latency)
// //
// // Design:
// // - Push型 → Pull型に切り替え
// // - Producer: write to TLS outbox (no atomic)
// // - Owner: scan all threads' outboxes (atomic_exchange)
// //
// // Expected: +5-15% on T=32/R=90%
//
// #ifndef HZ3_S147_REVERSE_MAILBOX
// #define HZ3_S147_REVERSE_MAILBOX 0  // default OFF for A/B
// #endif
//
// #ifndef HZ3_S147_OUTBOX_BATCH
// #define HZ3_S147_OUTBOX_BATCH 16
// #endif
//
// #ifndef HZ3_S147_NOTIFY_THRESHOLD
// #define HZ3_S147_NOTIFY_THRESHOLD 8
// #endif
//
// #ifndef HZ3_S147_MAX_THREADS
// #define HZ3_S147_MAX_THREADS 256
// #endif
//
// #ifndef HZ3_S147_STATS
// #define HZ3_S147_STATS 0
// #endif
//
// #ifndef HZ3_S147_FAILFAST
// #define HZ3_S147_FAILFAST 0
// #endif
// ```

// ============================================================================
// PATCH 2: hz3_types.h - Add S147 fields to Hz3TCache
// ============================================================================
//
// Add to Hz3TCache struct (near other TLS fields):
//
// ```c
// #if HZ3_S147_REVERSE_MAILBOX
//     // S147: Reverse Mailbox outbox per owner
//     Hz3S147Outbox s147_outbox[HZ3_NUM_SHARDS];
//     int s147_tid;  // index in g_s147_all_tcaches (-1 if not registered)
// #endif
// ```
//
// Also add include at top:
// ```c
// #if HZ3_S147_REVERSE_MAILBOX
// #include "hz3_s147_reverse_mailbox.h"
// #endif
// ```

// ============================================================================
// PATCH 3: hz3_tcache.c - Thread registration/deregistration
// ============================================================================
//
// In hz3_tcache_ensure_init_slow(), add after shard assignment:
//
// ```c
// #if HZ3_S147_REVERSE_MAILBOX
//     // S147: Initialize outbox and register thread
//     memset(t_hz3_cache.s147_outbox, 0, sizeof(t_hz3_cache.s147_outbox));
//     t_hz3_cache.s147_tid = -1;
//     hz3_s147_register_thread();
// #endif
// ```
//
// In hz3_tcache_destructor(), add before other cleanup:
//
// ```c
// #if HZ3_S147_REVERSE_MAILBOX
//     // S147: Flush all outboxes and unregister
//     hz3_s147_flush_all();
//     hz3_s147_unregister_thread();
// #endif
// ```

// ============================================================================
// PATCH 4: hz3_owner_stash_push.inc - Producer path hook
// ============================================================================
//
// In hz3_owner_stash_push_one(), add at the very top (before any existing logic):
//
// ```c
// #if HZ3_S147_REVERSE_MAILBOX
//     // S147: Try reverse mailbox first (no CAS path)
//     if (hz3_s147_push(owner, sc, ptr)) {
//         return;  // success, no CAS needed
//     }
//     // fallback to owner_stash CAS path below
// #endif
// ```

// ============================================================================
// PATCH 5: hz3_owner_stash_pop.inc - Owner path hook
// ============================================================================
//
// In hz3_owner_stash_pop_batch(), add before draining from owner stash:
//
// ```c
// #if HZ3_S147_REVERSE_MAILBOX
//     // S147: Pull from reverse mailbox first
//     int s147_got = hz3_s147_pull(sc, out + got, want - got);
//     got += s147_got;
//     if (got >= want) {
//         return got;
//     }
//     // continue to owner stash drain if need more
// #endif
// ```
//
// The best location is after the spill checks (S67/S94) but before the
// owner stash atomic drain.

// ============================================================================
// PATCH 6: Makefile - Add S147 source to build
// ============================================================================
//
// In hakozuna/hz3/Makefile, add to HZ3_SOURCES:
//
// ```make
// ifeq ($(HZ3_S147_REVERSE_MAILBOX),1)
//     HZ3_SOURCES += archive/research/s147_reverse_mailbox/hz3_s147_reverse_mailbox.c
// endif
// ```
//
// Or simpler: always include the .c file (stubs when disabled)

// ============================================================================
// PATCH 7: Enable via HZ3_LDPRELOAD_DEFS_EXTRA
// ============================================================================
//
// For A/B testing:
//
// ```bash
// # Baseline (S147 OFF)
// make -C hakozuna/hz3 clean all_ldpreload_scale
//
// # Test (S147 ON)
// make -C hakozuna/hz3 clean all_ldpreload_scale \
//   HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S147_REVERSE_MAILBOX=1'
// ```

// ============================================================================
// COMPILE FLAGS FOR INTEGRATION
// ============================================================================
//
// When S147 is enabled, define these to enable the TLS fields:
//
// ```c
// #define HZ3_S147_TID_FIELD 1
// #define HZ3_S147_OUTBOX_FIELD 1
// ```
//
// These guards allow the implementation to compile even without the TLS
// fields (falls back to always-fallback mode).
