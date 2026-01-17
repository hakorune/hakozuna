// hz3_owner_excl.h - Owner Exclusive Lock Box (CollisionGuardBox)
//
// Box Theory:
// - 境界1箇所: 各 "my_shard only" 箱の公開 API 入口でのみ acquire/release
// - 非衝突時: ロック skip (hot path 汚さない)
// - 衝突時: per-shard mutex で排他
// - 切替可能: compile-time flag で ON/OFF
//
// Usage:
//   Hz3OwnerExclToken token = hz3_owner_excl_acquire(owner);
//   // ... critical section ...
//   hz3_owner_excl_release(token);
//

#ifndef HZ3_OWNER_EXCL_H
#define HZ3_OWNER_EXCL_H

#include "hz3_types.h"
#include "hz3_tcache.h"  // hz3_shard_live_count()
#include <stdint.h>

// Config: Enable/disable owner exclusive lock
#ifndef HZ3_OWNER_EXCL_ENABLE
#define HZ3_OWNER_EXCL_ENABLE 1
#endif

// Config: Always lock (even before collision is detected).
//
// Rationale:
// - When shard collision is tolerated (HZ3_SHARD_COLLISION_FAILFAST=0), a second
//   thread can join a shard while another thread is already inside a "my_shard-only"
//   critical section. If the first thread skipped locking, the new thread cannot
//   serialize against that in-flight critical section and races are still possible.
// - Always-lock removes that join-window race at the cost of extra locking on slow paths.
#ifndef HZ3_OWNER_EXCL_ALWAYS
#if HZ3_SHARD_COLLISION_FAILFAST
#define HZ3_OWNER_EXCL_ALWAYS 0
#else
#define HZ3_OWNER_EXCL_ALWAYS 1
#endif
#endif

// Config: One-shot log on first contention
#ifndef HZ3_OWNER_EXCL_SHOT
#define HZ3_OWNER_EXCL_SHOT 1
#endif

// Exclusive token type (prevents forgetting release)
typedef struct {
    uint8_t owner;
    uint8_t locked;  // 1 if lock was acquired, 0 if skipped
    uint8_t active;  // 1 if proceed, 0 if try_acquire failed
} Hz3OwnerExclToken;

// Acquire exclusive access for owner shard (collision-time only)
Hz3OwnerExclToken hz3_owner_excl_acquire(uint8_t owner);

// Try acquire exclusive access (event-only)
// - Returns active=0 when the lock is needed but currently busy.
Hz3OwnerExclToken hz3_owner_excl_try_acquire(uint8_t owner);

// Release exclusive access
void hz3_owner_excl_release(Hz3OwnerExclToken token);

// Stats (for atexit diagnostics / one-shot summaries)
void hz3_owner_excl_get_stats(uint64_t* contended, uint64_t* guarded_ops);

// Check if owner exclusive is needed (fast path check)
static inline int hz3_owner_excl_needed(uint8_t owner) {
#if HZ3_OWNER_EXCL_ENABLE
    return HZ3_OWNER_EXCL_ALWAYS || (hz3_shard_live_count(owner) > 1);
#else
    (void)owner;
    return 0;
#endif
}

#endif  // HZ3_OWNER_EXCL_H
