// hz3_s147_reverse_mailbox.h - Reverse Mailbox (Pull-based remote free)
//
// S147: Push型（Producer→CAS→Owner）から Pull型（Producer→TLS→Owner poll）に切り替え
//
// Background (S146-B NO-GO):
// - CAS自体は軽い（失敗率0.002%）
// - バッチ化のリンク構築オーバーヘッドがCAS削減効果を上回った
// - perf: `hz3_owner_stash_push_one` 16.38% of cycles
//        `lock cmpxchg` 96.95% of push_one time（メモリレイテンシ）
//
// Design:
// - Producer: write to TLS outbox (no atomic), atomic_store(notify) at threshold
// - Owner: scan all threads' outboxes, atomic_exchange(items) to collect
// - Producer/Owner sync via `notify` flag (排他制御)
//
// Memory:
// - outbox[owner] × entries (NOT outbox[owner][sc] which explodes)
// - Entry has (ptr, sc) → 16KB/thread (vs 1MB if per-sc)
//
// Archive location: hakozuna/hz3/archive/research/s147_reverse_mailbox/
#pragma once

#include <stdint.h>
#include <stdatomic.h>

// ============================================================================
// S147 Feature Flag
// ============================================================================

#ifndef HZ3_S147_REVERSE_MAILBOX
#define HZ3_S147_REVERSE_MAILBOX 0
#endif

// ============================================================================
// S147 Tuning Parameters
// ============================================================================

// Outbox batch size per owner (items before forced flush)
#ifndef HZ3_S147_OUTBOX_BATCH
#define HZ3_S147_OUTBOX_BATCH 16
#endif

// Notify threshold (atomic_store(notify, 1) when count >= threshold)
// Lower = more frequent owner wake, higher = more latency
#ifndef HZ3_S147_NOTIFY_THRESHOLD
#define HZ3_S147_NOTIFY_THRESHOLD 8
#endif

// Maximum threads (for global tcache registry)
#ifndef HZ3_S147_MAX_THREADS
#define HZ3_S147_MAX_THREADS 256
#endif

// Debug/Stats flags
#ifndef HZ3_S147_STATS
#define HZ3_S147_STATS 0
#endif

#ifndef HZ3_S147_FAILFAST
#define HZ3_S147_FAILFAST 0
#endif

// ============================================================================
// S147 Structures
// ============================================================================

#if HZ3_S147_REVERSE_MAILBOX

// Entry: ptr + sc (packed to minimize memory)
// Using 16-byte struct for alignment
typedef struct {
    void*   ptr;
    uint8_t sc;
    uint8_t _pad[7];
} Hz3S147Entry;

_Static_assert(sizeof(Hz3S147Entry) == 16, "Hz3S147Entry must be 16 bytes");

// Outbox: per-owner batch buffer in TLS
//
// Concurrency model:
// - Producer (TLS owner): writes items[0..count-1], increments count
// - notify=0: Producer can write (Owner won't drain)
// - notify=1: Producer must fallback (Owner may drain)
//
// Memory per thread: HZ3_NUM_SHARDS * sizeof(Hz3S147Outbox)
//   = 32 * (16*16 + 8) = ~8.5KB (for 32 shards, 16 items)
typedef struct {
    Hz3S147Entry items[HZ3_S147_OUTBOX_BATCH];
    uint8_t      count;                 // current item count [0, BATCH]
    _Atomic uint8_t notify;             // 0=empty/producer-ok, 1=has-items/owner-draining
    uint8_t      _pad[6];
} Hz3S147Outbox;

// Global thread registry (for Owner scan)
// Populated during tcache init, cleared on tcache destructor
#ifndef HZ3_S147_THREAD_REGISTRY_DECLARED
#define HZ3_S147_THREAD_REGISTRY_DECLARED
struct Hz3TCache;  // forward declaration
extern struct Hz3TCache* g_s147_all_tcaches[HZ3_S147_MAX_THREADS];
extern _Atomic int g_s147_thread_count;
#endif

// ============================================================================
// S147 API (internal)
// ============================================================================

// Producer path: Push single object to outbox
// Returns 1 on success, 0 if should fallback to owner_stash
int hz3_s147_push(uint8_t owner, int sc, void* ptr);

// Owner path: Pull objects from all producers' outboxes
// Returns number of objects placed in out[]
int hz3_s147_pull(int sc, void** out, int want);

// Flush outbox to owner_stash (thread exit cleanup)
void hz3_s147_flush_to_owner_stash(uint8_t owner);

// Flush all outboxes (thread exit)
void hz3_s147_flush_all(void);

// Thread registration (called from tcache init)
void hz3_s147_register_thread(void);

// Thread unregistration (called from tcache destructor)
void hz3_s147_unregister_thread(void);

// Initialize S147 subsystem (pthread_once)
void hz3_s147_init(void);

// ============================================================================
// S147 Stats
// ============================================================================

#if HZ3_S147_STATS
extern _Atomic uint64_t g_s147_push_calls;
extern _Atomic uint64_t g_s147_push_accepted;
extern _Atomic uint64_t g_s147_push_fallback;
extern _Atomic uint64_t g_s147_pull_calls;
extern _Atomic uint64_t g_s147_pull_objs;
extern _Atomic uint64_t g_s147_flush_calls;
extern _Atomic uint64_t g_s147_flush_objs;

#define S147_STAT_INC(x) atomic_fetch_add_explicit(&(x), 1, memory_order_relaxed)
#define S147_STAT_ADD(x, n) atomic_fetch_add_explicit(&(x), (n), memory_order_relaxed)
#else
#define S147_STAT_INC(x) ((void)0)
#define S147_STAT_ADD(x, n) ((void)0)
#endif

#else  // !HZ3_S147_REVERSE_MAILBOX

// Stubs when disabled

static inline int hz3_s147_push(uint8_t owner, int sc, void* ptr) {
    (void)owner; (void)sc; (void)ptr;
    return 0;  // always fallback
}

static inline int hz3_s147_pull(int sc, void** out, int want) {
    (void)sc; (void)out; (void)want;
    return 0;
}

static inline void hz3_s147_flush_to_owner_stash(uint8_t owner) {
    (void)owner;
}

static inline void hz3_s147_flush_all(void) {}

static inline void hz3_s147_register_thread(void) {}

static inline void hz3_s147_unregister_thread(void) {}

static inline void hz3_s147_init(void) {}

#endif  // HZ3_S147_REVERSE_MAILBOX
