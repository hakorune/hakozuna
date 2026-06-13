
// Drain budget (objects) per alloc_slow mixed-drain attempt.
#ifndef HZ3_S206B_MIXED_DRAIN_BUDGET
#define HZ3_S206B_MIXED_DRAIN_BUDGET 64
#endif

// Fail-fast if ptr->sc decode fails during mixed drain.
#ifndef HZ3_S206B_MIXED_DECODE_FAILFAST
#define HZ3_S206B_MIXED_DECODE_FAILFAST 0
#endif

#if HZ3_S206B_MIXED_OUTBOX_FLUSH_N < 1
#error "HZ3_S206B_MIXED_OUTBOX_FLUSH_N must be >= 1"
#endif

#if HZ3_S206B_MIXED_DRAIN_BUDGET < 1
#error "HZ3_S206B_MIXED_DRAIN_BUDGET must be >= 1"
#endif

// RemoteFlushUnrollBox (hz4 -> hz3 reverse import, Phase1):
// - Applies 4-way unroll in sparse remote stash flush loops.
// - Archived (NO-GO): moved to hz3_config_archive.h (#error on enable).

// RbufKeyBox (hz4 -> hz3 reverse import, Phase1):
// - Cache (dst<<8)|bin in sparse ring entry to avoid recomputing in flush path.
// - Archived (NO-GO): moved to hz3_config_archive.h (#error on enable).

// S161: Direct n==1 dispatch for remote stash flush (avoid list build + dispatch overhead).
// A/B: set to 0 to revert.
#ifndef HZ3_S161_REMOTE_STASH_N1_DIRECT
#define HZ3_S161_REMOTE_STASH_N1_DIRECT 1
#endif

// S178: Guard S161 direct path by sparse-ring occupancy.
// Motivation:
// - When ring backlog is large, repeated n==1 direct pushes can increase fixed cost.
// - Gate direct path to "small backlog only" and fall back to list dispatch otherwise.
#ifndef HZ3_S178_N1_DIRECT_RING_GUARD
#define HZ3_S178_N1_DIRECT_RING_GUARD 0
#endif

#ifndef HZ3_S178_N1_DIRECT_MAX_RING
#define HZ3_S178_N1_DIRECT_MAX_RING 8
#endif

#if HZ3_S178_N1_DIRECT_RING_GUARD && (HZ3_S178_N1_DIRECT_MAX_RING < 1)
#error "HZ3_S178_N1_DIRECT_MAX_RING must be >= 1 when HZ3_S178_N1_DIRECT_RING_GUARD=1"
#endif

// S97: Bucketize/merge within one flush_budget window by (dst,bin), turning
// repeated (dst,bin) dispatches into a single `push_list(n>1)` call.
// =0: OFF (default)
// =1: S97-1 (hash + open addressing)
// =2: S97-2 (direct-map + stamp, probe-less)
// =3: S97-3 (direct-map sparse-set, stamp-less)
// =4: S97-4 (direct-map + touched reset)
// =5: S97-5 (direct-map flat slot; epoch+idx in one table)
// =6: S97-8 (table-less radix sort + group; stack-only, no TLS tables)
#ifndef HZ3_S97_REMOTE_STASH_BUCKET
#define HZ3_S97_REMOTE_STASH_BUCKET 0
#endif
#if HZ3_S97_REMOTE_STASH_BUCKET > 6
#error "HZ3_S97_REMOTE_STASH_BUCKET must be 0..6"
#endif

// Max number of distinct keys held per bucketization round.
// When exceeded, current buckets are dispatched and the map is reset.
#ifndef HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS
#define HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS 128
#endif

// Skip `hz3_obj_set_next(ptr, NULL)` when draining sparse ring entries.
// Safe in release (push_list will overwrite tail->next), but incompatible with
// `HZ3_LIST_FAILFAST` / debug list checkers that require tail->next==NULL.
#ifndef HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
#define HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL 0
#endif

// S97-6 (OwnerStashPushMicroBatchBox) was archived (NO-GO). See hz3_config_archive.h.

// S128 (RemoteStash Defer-MinRun) was archived (NO-GO). See hz3_config_archive.h.

// S94 is mutually exclusive with S67/S67-2
#if HZ3_S94_SPILL_LITE && (HZ3_S67_SPILL_ARRAY || HZ3_S67_SPILL_ARRAY2)
#error "S94 and S67/S67-2 are mutually exclusive"
#endif
