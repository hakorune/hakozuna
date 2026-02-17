
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

// ============================================================================
// S110: SegHdr PerPage FreeMeta (segment-local metadata)
// ============================================================================
//
// Motivation:
// - hz3_free PTAG lookup (global table) is a bottleneck (40.83% vs mimalloc 32.65%)
// - mimalloc uses segment-local metadata for better cache locality
//
// Status (scale lane):
// - S110-0: GO (meta hit-rate 100% observed)
// - S110-1 (PTAG bypass in hz3_free): NO-GO (slower than PTAG32 due to seg lookup overhead)
//
// New field in Hz3SegHdr:
//   _Atomic(uint16_t) page_bin_plus1[HZ3_PAGES_PER_SEG]
//   0 = unknown/not-allocated, 1..N = bin + 1
//
// Enable the metadata field and its write/clear boundaries.
// Default OFF (research opt-in only).
#ifndef HZ3_S110_META_ENABLE
#define HZ3_S110_META_ENABLE 0
#endif

// S113: SegMath + page_bin_plus1 fast dispatch
// Enabled by default when S110_META_ENABLE is on.
#ifndef HZ3_S113_FREE_FAST_ENABLE
#define HZ3_S113_FREE_FAST_ENABLE (HZ3_S110_META_ENABLE)
#endif

// Enable atexit one-shot stats (observation; safe even when FREE_FAST is OFF).
#ifndef HZ3_S110_STATS
#define HZ3_S110_STATS 0
#endif

// Debug-only: fail fast on inconsistent meta/bin/dst in S110 paths.
#ifndef HZ3_S110_FAILFAST
#define HZ3_S110_FAILFAST 0
#endif

// S110_FREE_FAST_ENABLE and deprecated aliases moved to hz3_config_archive.h

// ============================================================================
// S142: Lock-Free MPSC Central Bin (pthread_mutex overhead elimination)
// ============================================================================
//
// Motivation:
// - perf shows pthread_mutex overhead:
//   - hz3_small_xfer (mutex): 6.12%
//   - hz3_central_bin (mutex): 5.60%
//   - Total: 12.11%
//
// Design:
// - MPSC confirmed: Pop is always from my_shard owner (TLS t_hz3_cache.my_shard)
// - Push uses CAS loop (multiple producers from remote free)
// - Pop uses atomic_exchange + push-back remainder
//
// Expected effect: 12.11% → ~1% (net gain +10%)
//
// Phases:
// - S142-A: hz3_central_bin lock-free (Phase 1) - GO (2026-01-17)
// - S142-B: hz3_small_xfer lock-free (Phase 2) - GO (2026-01-17)
//
// Results (S142-A: central lock-free):
// - T=8 RANDOM: +11.8%, r90: T=8 +5.9%, T=16 +25.7%, T=32 +56.3%
// - R=0 fixed cost: -0.6% (noise)
//
// Results (S142-B: xfer lock-free):
// - R=0 T=8: +1.9% (no regression)
// - r90 T=8: +41%, r90 T=32: +105%

#ifndef HZ3_S142_CENTRAL_LOCKFREE
// Default OFF at header level; scale/p32 lanes enable via Makefile.
#define HZ3_S142_CENTRAL_LOCKFREE 0
#endif

// Debug: fail fast on count underflow (indicates bug)
#ifndef HZ3_S142_FAILFAST
#define HZ3_S142_FAILFAST 0
#endif

// Compile-time guard: S142 is incompatible with S86 shadow (needs mutex)
#if HZ3_S142_CENTRAL_LOCKFREE && HZ3_S86_CENTRAL_SHADOW
#error "HZ3_S142_CENTRAL_LOCKFREE=1 requires HZ3_S86_CENTRAL_SHADOW=0"
#endif

// Phase 2: Transfer cache lock-free
#ifndef HZ3_S142_XFER_LOCKFREE
// Default OFF at header level; scale/p32 lanes enable via Makefile.
#define HZ3_S142_XFER_LOCKFREE 0
#endif

// ============================================================================
// S154: Spill Overflow Prefetch (owner_stash_pop_batch optimization)
// ============================================================================
//
// Motivation:
// - Spill overflow walk (lines 43-49, 289-294) has pointer chasing
// - Prefetch next pointer to hide memory latency
// - Expected: +0.5-1.5% on xmalloc-test (remote-heavy workloads)
//
// Design:
// - Add __builtin_prefetch(hz3_obj_get_next(ov), 0, 3) in spill_overflow loop
// - Distance 1 fixed (simple optimization, low risk)
//
// Results (2026-01-19):
// - xmalloc-test: +7.06% (1.925s → 1.789s)
// - SSOT: small -2.00%, medium +4.00%, mixed +0.30% (all pass -3% threshold)
// - GO: Enable by default
//
#ifndef HZ3_S154_SPILL_PREFETCH
#define HZ3_S154_SPILL_PREFETCH 1  // GO (2026-01-19): +7.06% on xmalloc-test
#endif

// ============================================================================
// S220: CPU-local Remote Recycle Queue (RRQ) for medium remote free
// ============================================================================
//
// Goal:
// - reduce medium remote publish/drain fixed cost on MT remote-heavy lanes by
//   reusing remote-freed objects on the current CPU before inbox/central path.
//
// Design:
// - remote medium dispatch (n==1) can push directly to RRQ when dst!=self.
// - medium alloc slow path tries RRQ pop before mixed/inbox steal/central.
// - fully opt-in and fail-fast reversible.
//
#ifndef HZ3_S220_CPU_RRQ
#define HZ3_S220_CPU_RRQ 0
#endif

#ifndef HZ3_S220_CPU_RRQ_SC_MIN
#define HZ3_S220_CPU_RRQ_SC_MIN 5
#endif

#ifndef HZ3_S220_CPU_RRQ_SC_MAX
#define HZ3_S220_CPU_RRQ_SC_MAX 7
#endif

#ifndef HZ3_S220_CPU_RRQ_CAP_PER_CPU_PER_SC
#define HZ3_S220_CPU_RRQ_CAP_PER_CPU_PER_SC 64
#endif

#ifndef HZ3_S220_CPU_RRQ_MAX_CPUS
#define HZ3_S220_CPU_RRQ_MAX_CPUS 256
#endif

// 0: sched_getcpu() (fallback to shard id on error)
// 1: use shard id directly (lower overhead / less locality fidelity)
#ifndef HZ3_S220_CPU_RRQ_CPU_ID_MODE
#define HZ3_S220_CPU_RRQ_CPU_ID_MODE 0
#endif

#ifndef HZ3_S220_CPU_RRQ_REFILL_BURST_MAX
#define HZ3_S220_CPU_RRQ_REFILL_BURST_MAX 8
#endif

#ifndef HZ3_S220_CPU_RRQ_STATS
#define HZ3_S220_CPU_RRQ_STATS 0
#endif

#ifndef HZ3_S220_CPU_RRQ_STATS_SC
#define HZ3_S220_CPU_RRQ_STATS_SC 0
#endif

#if HZ3_S220_CPU_RRQ_SC_MIN < 0
#error "HZ3_S220_CPU_RRQ_SC_MIN must be >= 0"
#endif

#if HZ3_S220_CPU_RRQ_SC_MAX < HZ3_S220_CPU_RRQ_SC_MIN
#error "HZ3_S220_CPU_RRQ_SC_MAX must be >= HZ3_S220_CPU_RRQ_SC_MIN"
#endif

#if HZ3_S220_CPU_RRQ_CAP_PER_CPU_PER_SC < 1
#error "HZ3_S220_CPU_RRQ_CAP_PER_CPU_PER_SC must be >= 1"
#endif

#if HZ3_S220_CPU_RRQ_MAX_CPUS < 1
#error "HZ3_S220_CPU_RRQ_MAX_CPUS must be >= 1"
#endif

#if HZ3_S220_CPU_RRQ_CPU_ID_MODE < 0 || HZ3_S220_CPU_RRQ_CPU_ID_MODE > 1
#error "HZ3_S220_CPU_RRQ_CPU_ID_MODE must be 0 or 1"
#endif

#if HZ3_S220_CPU_RRQ_REFILL_BURST_MAX < 1
#error "HZ3_S220_CPU_RRQ_REFILL_BURST_MAX must be >= 1"
#endif

#if HZ3_S220_CPU_RRQ_STATS_SC && !HZ3_S220_CPU_RRQ_STATS
#error "HZ3_S220_CPU_RRQ_STATS_SC requires HZ3_S220_CPU_RRQ_STATS=1"
#endif

// ============================================================================
// S221: Medium Dispatch Scoped Minibatch (archived; hard-off)
// ============================================================================
//
// Goal:
// - keep inbox recycle model (owner drain/batch refill) but reduce remote
//   publish fixed cost for medium remote n==1 dispatch.
//
// Design:
// - only for configured sc range and remote dst!=self.
// - aggregate into TLS outbox (dst,sc) and flush as list when threshold reached
//   or at remote stash flush boundary.
// - archived as NO-GO: enabling is blocked by hz3_config_archive.h.
//
#ifndef HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
#define HZ3_S221_MEDIUM_DISPATCH_MINIBATCH 0
#endif

#ifndef HZ3_S221_SC_MIN
#define HZ3_S221_SC_MIN 5
#endif

#ifndef HZ3_S221_SC_MAX
#define HZ3_S221_SC_MAX 7
#endif

#ifndef HZ3_S221_OUTBOX_FLUSH_N
#define HZ3_S221_OUTBOX_FLUSH_N 8
#endif

// 1: only n==1 dispatch is eligible (default)
// 0: any n>=1 dispatch in sc range is eligible
#ifndef HZ3_S221_ONLY_N1
#define HZ3_S221_ONLY_N1 1
#endif

#if HZ3_S221_SC_MIN < 0
#error "HZ3_S221_SC_MIN must be >= 0"
#endif

#if HZ3_S221_SC_MAX < HZ3_S221_SC_MIN
#error "HZ3_S221_SC_MAX must be >= HZ3_S221_SC_MIN"
#endif

#if HZ3_S221_OUTBOX_FLUSH_N < 2
#error "HZ3_S221_OUTBOX_FLUSH_N must be >= 2"
#endif

#if HZ3_S221_ONLY_N1 < 0 || HZ3_S221_ONLY_N1 > 1
#error "HZ3_S221_ONLY_N1 must be 0 or 1"
#endif

// ============================================================================
// S222: Central Atomic Fastpath (default ON)
// ============================================================================
//
// Goal:
// - reduce medium central lock overhead by adding an atomic stack fastpath for
//   selected size classes before legacy locked central path.
//
// Design:
// - producer (`hz3_central_push_list`) pushes to per-(shard,sc) atomic head.
// - consumer (`hz3_central_pop_batch`) first drains atomic head, then falls
//   back to legacy locked central bin.
// - sc scope is explicit to protect guard lanes.
//
#ifndef HZ3_S222_CENTRAL_ATOMIC_FAST
#define HZ3_S222_CENTRAL_ATOMIC_FAST 1
#endif

#ifndef HZ3_S222_SC_MIN
#define HZ3_S222_SC_MIN 5
#endif

#ifndef HZ3_S222_SC_MAX
#define HZ3_S222_SC_MAX 7
#endif

#ifndef HZ3_S222_STATS
#define HZ3_S222_STATS 0
#endif

#if HZ3_S222_SC_MIN < 0
#error "HZ3_S222_SC_MIN must be >= 0"
#endif

#if HZ3_S222_SC_MAX < HZ3_S222_SC_MIN
#error "HZ3_S222_SC_MAX must be >= HZ3_S222_SC_MIN"
#endif

#if HZ3_S222_STATS < 0 || HZ3_S222_STATS > 1
#error "HZ3_S222_STATS must be 0 or 1"
#endif

// ============================================================================
// S234: Central fast partial-pop CAS (opt-in)
// ============================================================================
//
// Goal:
// - reduce S222 pop->repush fixed cost by popping only `want` nodes from fast
//   stack head (CAS head old->new) instead of `exchange(all)+repush(remainder)`.
//
// Design:
// - scoped to selected sc range (`sc=5..7` by default).
// - bounded CAS retry; fallback to existing S222 exchange path on contention.
//
#ifndef HZ3_S234_CENTRAL_FAST_PARTIAL_POP
#define HZ3_S234_CENTRAL_FAST_PARTIAL_POP 0
#endif

#ifndef HZ3_S234_SC_MIN
#define HZ3_S234_SC_MIN 5
#endif

#ifndef HZ3_S234_SC_MAX
#define HZ3_S234_SC_MAX 7
#endif

#ifndef HZ3_S234_CAS_RETRY_MAX
#define HZ3_S234_CAS_RETRY_MAX 8
#endif

#if HZ3_S234_SC_MIN < 0
#error "HZ3_S234_SC_MIN must be >= 0"
#endif

#if HZ3_S234_SC_MAX < HZ3_S234_SC_MIN
#error "HZ3_S234_SC_MAX must be >= HZ3_S234_SC_MIN"
#endif

#if HZ3_S234_CAS_RETRY_MAX < 1
#error "HZ3_S234_CAS_RETRY_MAX must be >= 1"
#endif

// ============================================================================
// S229: Medium alloc_slow central-first (archived; hard-off)
// ============================================================================
//
// NO-GO summary:
// - path engaged but source mix shifted to central-heavy, amplifying slowpath.
// - replay-stable main-lane collapse; hard-archived via hz3_config_archive.h.
//
// This block keeps archived knob metadata/validation only.
// Enabling is blocked by hz3_config_archive.h (#error).
//
#ifndef HZ3_S229_CENTRAL_FIRST
#define HZ3_S229_CENTRAL_FIRST 0
#endif

#ifndef HZ3_S229_SC_MIN
#define HZ3_S229_SC_MIN 5
#endif

#ifndef HZ3_S229_SC_MAX
#define HZ3_S229_SC_MAX 7
#endif

#ifndef HZ3_S229_PEEK_BEFORE_POP
#define HZ3_S229_PEEK_BEFORE_POP 1
#endif

#ifndef HZ3_S229_SKIP_SECOND_CENTRAL_TRY
#define HZ3_S229_SKIP_SECOND_CENTRAL_TRY 1
#endif

#if HZ3_S229_SC_MIN < 0
#error "HZ3_S229_SC_MIN must be >= 0"
#endif

#if HZ3_S229_SC_MAX < HZ3_S229_SC_MIN
#error "HZ3_S229_SC_MAX must be >= HZ3_S229_SC_MIN"
#endif

#if HZ3_S229_PEEK_BEFORE_POP < 0 || HZ3_S229_PEEK_BEFORE_POP > 1
#error "HZ3_S229_PEEK_BEFORE_POP must be 0 or 1"
#endif

#if HZ3_S229_SKIP_SECOND_CENTRAL_TRY < 0 || HZ3_S229_SKIP_SECOND_CENTRAL_TRY > 1
#error "HZ3_S229_SKIP_SECOND_CENTRAL_TRY must be 0 or 1"
#endif

// ============================================================================
// S228: Central Fast local remainder cache (opt-in)
// ============================================================================
//
// Goal:
// - reduce S222 pop->repush overhead for shard-exclusive windows by keeping
//   fast-pop remainder in TLS and serving next pop from TLS first.
//
// Design:
// - scoped to selected sc range.
// - default behavior requires live_count==1 for the shard.
// - when shard becomes shared, TLS remainder is flushed back to fast head.
//
#ifndef HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
#define HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER 0
#endif

#ifndef HZ3_S228_SC_MIN
#define HZ3_S228_SC_MIN 5
#endif

#ifndef HZ3_S228_SC_MAX
#define HZ3_S228_SC_MAX 7
#endif

#ifndef HZ3_S228_ONLY_IF_LIVECOUNT_1
#define HZ3_S228_ONLY_IF_LIVECOUNT_1 1
#endif

#ifndef HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1
#define HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1 1
#endif

#if HZ3_S228_SC_MIN < 0
#error "HZ3_S228_SC_MIN must be >= 0"
#endif

#if HZ3_S228_SC_MAX < HZ3_S228_SC_MIN
#error "HZ3_S228_SC_MAX must be >= HZ3_S228_SC_MIN"
#endif

#if HZ3_S228_ONLY_IF_LIVECOUNT_1 < 0 || HZ3_S228_ONLY_IF_LIVECOUNT_1 > 1
#error "HZ3_S228_ONLY_IF_LIVECOUNT_1 must be 0 or 1"
#endif

#if HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1 < 0 || HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1 > 1
#error "HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1 must be 0 or 1"
#endif

// ============================================================================
// S223: Medium alloc_slow Dynamic Want (opt-in)
// ============================================================================
//
// Goal:
// - reduce repeated central-miss -> segment fallback churn on medium sc range
//   by temporarily increasing refill "want" after miss streak.
//
// Design:
// - scoped to sc range (`SC_MIN..SC_MAX`), default OFF.
// - apply boost only when miss streak reaches threshold.
// - hard-cap final want to avoid oversized refill bursts.
//
#ifndef HZ3_S223_DYNAMIC_WANT
#define HZ3_S223_DYNAMIC_WANT 0
#endif

#ifndef HZ3_S223_SC_MIN
#define HZ3_S223_SC_MIN 5
#endif

#ifndef HZ3_S223_SC_MAX
#define HZ3_S223_SC_MAX 7
#endif

#ifndef HZ3_S223_MISS_STREAK_MIN
#define HZ3_S223_MISS_STREAK_MIN 3
#endif

#ifndef HZ3_S223_WANT_BONUS
#define HZ3_S223_WANT_BONUS 2
#endif

#ifndef HZ3_S223_WANT_CAP
#define HZ3_S223_WANT_CAP 8
#endif

#if HZ3_S223_SC_MIN < 0
#error "HZ3_S223_SC_MIN must be >= 0"
#endif

#if HZ3_S223_SC_MAX < HZ3_S223_SC_MIN
#error "HZ3_S223_SC_MAX must be >= HZ3_S223_SC_MIN"
#endif

#if HZ3_S223_MISS_STREAK_MIN < 1
#error "HZ3_S223_MISS_STREAK_MIN must be >= 1"
#endif

#if HZ3_S223_WANT_BONUS < 1
#error "HZ3_S223_WANT_BONUS must be >= 1"
#endif

#if HZ3_S223_WANT_CAP < 1
#error "HZ3_S223_WANT_CAP must be >= 1"
#endif

// ============================================================================
// S224: Medium remote local n==1 pair-batch (opt-in)
// ============================================================================
//
// Goal:
// - reduce n==1 inbox publish fixed cost for medium remote path by pairing two
//   consecutive n==1 objects for the same (dst,sc) on producer TLS.
//
// Design:
// - keep at most one pending object per (dst,sc) in producer TLS.
// - next n==1 for same key publishes one n==2 list immediately.
// - pending singles are flushed at sparse remote stash flush boundaries.
//
#ifndef HZ3_S224_MEDIUM_N1_PAIR_BATCH
#define HZ3_S224_MEDIUM_N1_PAIR_BATCH 0
#endif

#ifndef HZ3_S224_SC_MIN
#define HZ3_S224_SC_MIN 5
#endif

#ifndef HZ3_S224_SC_MAX
#define HZ3_S224_SC_MAX 7
#endif

// 1: bypass pair-batch for dst==my_shard (default)
// 0: allow pair-batch even for self dst
#ifndef HZ3_S224_SKIP_SELF_DST
#define HZ3_S224_SKIP_SELF_DST 1
#endif

#if HZ3_S224_SC_MIN < 0
#error "HZ3_S224_SC_MIN must be >= 0"
#endif

#if HZ3_S224_SC_MAX < HZ3_S224_SC_MIN
#error "HZ3_S224_SC_MAX must be >= HZ3_S224_SC_MIN"
#endif

#if HZ3_S224_SKIP_SELF_DST < 0 || HZ3_S224_SKIP_SELF_DST > 1
#error "HZ3_S224_SKIP_SELF_DST must be 0 or 1"
#endif

// ============================================================================
// S225: Medium remote n==1 last-key pair-batch (opt-in)
// ============================================================================
//
// Goal:
// - reduce n==1 dominant medium remote publish fixed cost with lower TLS
//   footprint than S224.
//
// Design:
// - keep one pending n==1 object with last (dst,sc) key in TLS.
// - if next n==1 hits same key, publish n==2 list immediately.
// - if key differs, flush pending single and replace pending key.
//
#ifndef HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
#define HZ3_S225_MEDIUM_N1_LASTKEY_BATCH 0
#endif

#ifndef HZ3_S225_SC_MIN
#define HZ3_S225_SC_MIN 5
#endif

#ifndef HZ3_S225_SC_MAX
#define HZ3_S225_SC_MAX 7
#endif

#ifndef HZ3_S225_SKIP_SELF_DST
#define HZ3_S225_SKIP_SELF_DST 1
#endif

#if HZ3_S225_SC_MIN < 0
#error "HZ3_S225_SC_MIN must be >= 0"
#endif

#if HZ3_S225_SC_MAX < HZ3_S225_SC_MIN
#error "HZ3_S225_SC_MAX must be >= HZ3_S225_SC_MIN"
#endif

#if HZ3_S225_SKIP_SELF_DST < 0 || HZ3_S225_SKIP_SELF_DST > 1
#error "HZ3_S225_SKIP_SELF_DST must be 0 or 1"
#endif

// ============================================================================
// S226: Medium flush-scope bucket3 (opt-in)
// ============================================================================
//
// Goal:
// - reduce medium remote publish fixed cost by grouping entries only inside one
//   remote_stash flush window, keyed by (dst, sc) for sc=5..7.
//
// Design:
// - no persistent TLS outbox state; only local (function-scope) buckets.
// - non-target bins keep existing behavior.
//
#ifndef HZ3_S226_MEDIUM_FLUSH_BUCKET3
#define HZ3_S226_MEDIUM_FLUSH_BUCKET3 0
#endif

#ifndef HZ3_S226_SC_MIN
#define HZ3_S226_SC_MIN 5
#endif

#ifndef HZ3_S226_SC_MAX
#define HZ3_S226_SC_MAX 7
#endif

#ifndef HZ3_S226_BUCKET_CHUNK_MAX
#define HZ3_S226_BUCKET_CHUNK_MAX 64
#endif

#if HZ3_S226_SC_MIN < 0
#error "HZ3_S226_SC_MIN must be >= 0"
#endif

#if HZ3_S226_SC_MAX < HZ3_S226_SC_MIN
#error "HZ3_S226_SC_MAX must be >= HZ3_S226_SC_MIN"
#endif

#if HZ3_S226_BUCKET_CHUNK_MAX < 1
#error "HZ3_S226_BUCKET_CHUNK_MAX must be >= 1"
#endif

// ============================================================================
// S230: Medium n==1 to central-fast direct (aggressive opt-in)
// ============================================================================
//
// Goal:
// - reduce medium remote publish fixed cost under n==1 dominance by routing
//   n==1 medium dispatch to central fastpath (S222), bypassing inbox publish.
//
// Scope:
// - medium only, default sc=5..7
// - default keeps this OFF (aggressive lane opt-in)
//
#ifndef HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
#define HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST 0
#endif

#ifndef HZ3_S230_SC_MIN
#define HZ3_S230_SC_MIN 5
#endif

#ifndef HZ3_S230_SC_MAX
#define HZ3_S230_SC_MAX 7
#endif

#ifndef HZ3_S230_ONLY_N_EQ_1
#define HZ3_S230_ONLY_N_EQ_1 1
#endif

#ifndef HZ3_S230_SKIP_SELF_DST
#define HZ3_S230_SKIP_SELF_DST 1
#endif

#if HZ3_S230_SC_MIN < 0
#error "HZ3_S230_SC_MIN must be >= 0"
#endif

#if HZ3_S230_SC_MAX < HZ3_S230_SC_MIN
#error "HZ3_S230_SC_MAX must be >= HZ3_S230_SC_MIN"
#endif

#if HZ3_S230_ONLY_N_EQ_1 < 0 || HZ3_S230_ONLY_N_EQ_1 > 1
#error "HZ3_S230_ONLY_N_EQ_1 must be 0 or 1"
#endif

#if HZ3_S230_SKIP_SELF_DST < 0 || HZ3_S230_SKIP_SELF_DST > 1
#error "HZ3_S230_SKIP_SELF_DST must be 0 or 1"
#endif

// ============================================================================
// S231: Medium inbox fast MPSC (aggressive opt-in)
// ============================================================================
//
// Goal:
// - reduce medium inbox fixed cost for hot remote lanes by using a simplified
//   lane-0 MPSC push path scoped to selected size classes.
//
// Scope:
// - medium only, default sc=5..7
// - default OFF (aggressive lane opt-in)
//
#ifndef HZ3_S231_INBOX_FAST_MPSC
#define HZ3_S231_INBOX_FAST_MPSC 0
#endif

#ifndef HZ3_S231_SC_MIN
#define HZ3_S231_SC_MIN 5
#endif

#ifndef HZ3_S231_SC_MAX
#define HZ3_S231_SC_MAX 7
#endif

// 1: skip inbox push-seq update on S231 path (S200 is default off)
// 0: keep seq update identical to legacy path
#ifndef HZ3_S231_SKIP_PUSH_SEQ
#define HZ3_S231_SKIP_PUSH_SEQ 1
#endif

#if HZ3_S231_SC_MIN < 0
#error "HZ3_S231_SC_MIN must be >= 0"
#endif

#if HZ3_S231_SC_MAX < HZ3_S231_SC_MIN
#error "HZ3_S231_SC_MAX must be >= HZ3_S231_SC_MIN"
#endif

#if HZ3_S231_SKIP_PUSH_SEQ < 0 || HZ3_S231_SKIP_PUSH_SEQ > 1
#error "HZ3_S231_SKIP_PUSH_SEQ must be 0 or 1"
#endif

// ============================================================================
// S236-A: Medium remote mailbox + alloc-fast pop (aggressive opt-in)
// ============================================================================
//
// Goal:
// - make n==1-dominant medium remote publish cheaper by routing singleton
//   remote objects to per-owner mailbox slots, consumable before alloc_slow.
//
// Scope:
// - medium only, default sc=5..7
// - default ON (promoted from aggressive lane A/B)
//
#ifndef HZ3_S236_MEDIUM_MAILBOX
#define HZ3_S236_MEDIUM_MAILBOX 1
#endif

#ifndef HZ3_S236_SC_MIN
#define HZ3_S236_SC_MIN 5
#endif

#ifndef HZ3_S236_SC_MAX
#define HZ3_S236_SC_MAX 7
#endif

// 1: only n==1 dispatch is mailbox-eligible.
// 0: allow any n>=1 list (v0 only supports singleton path).
#ifndef HZ3_S236_ONLY_N_EQ_1
#define HZ3_S236_ONLY_N_EQ_1 1
#endif

// 1: skip mailbox push for dst==my_shard.
// 0: allow self-dst mailbox push.
#ifndef HZ3_S236_SKIP_SELF_DST
#define HZ3_S236_SKIP_SELF_DST 1
#endif

// Mailbox slots per (owner,sc).
// Keep this small (1..4) to preserve cache locality.
#ifndef HZ3_S236_MAILBOX_SLOTS
#define HZ3_S236_MAILBOX_SLOTS 1
#endif

#if HZ3_S236_SC_MIN < 0
#error "HZ3_S236_SC_MIN must be >= 0"
#endif

#if HZ3_S236_SC_MAX < HZ3_S236_SC_MIN
#error "HZ3_S236_SC_MAX must be >= HZ3_S236_SC_MIN"
#endif

#if HZ3_S236_ONLY_N_EQ_1 < 0 || HZ3_S236_ONLY_N_EQ_1 > 1
#error "HZ3_S236_ONLY_N_EQ_1 must be 0 or 1"
#endif

#if HZ3_S236_SKIP_SELF_DST < 0 || HZ3_S236_SKIP_SELF_DST > 1
#error "HZ3_S236_SKIP_SELF_DST must be 0 or 1"
#endif

#if HZ3_S236_MAILBOX_SLOTS < 1
#error "HZ3_S236_MAILBOX_SLOTS must be >= 1"
#endif

#if HZ3_S236_MAILBOX_SLOTS > 4
#error "HZ3_S236_MAILBOX_SLOTS must be <= 4"
#endif

// ============================================================================
// S236-B: Medium mini-refill lane before full alloc_slow (aggressive opt-in)
// ============================================================================
//
// Goal:
// - reduce full alloc_slow entry frequency for medium sc by trying a bounded
//   pre-slow refill sequence (inbox / central-fast) on bin-empty.
//
// Scope:
// - medium only, default sc=5..7
// - default ON (promoted from aggressive lane A/B)
//
#ifndef HZ3_S236_MINIREFILL
#define HZ3_S236_MINIREFILL 1
#endif

#ifndef HZ3_S236_MINIREFILL_K
#define HZ3_S236_MINIREFILL_K 8
#endif

#ifndef HZ3_S236_MINIREFILL_TRY_INBOX
#define HZ3_S236_MINIREFILL_TRY_INBOX 1
#endif

#ifndef HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST
#define HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST 1
#endif

#if HZ3_S236_MINIREFILL_K < 1
#error "HZ3_S236_MINIREFILL_K must be >= 1"
#endif

#if HZ3_S236_MINIREFILL_TRY_INBOX < 0 || HZ3_S236_MINIREFILL_TRY_INBOX > 1
#error "HZ3_S236_MINIREFILL_TRY_INBOX must be 0 or 1"
#endif

#if HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST < 0 || HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST > 1
#error "HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST must be 0 or 1"
#endif

// ============================================================================
// S236-E: mini-refill central retry (aggressive opt-in)
// ============================================================================
//
// Goal:
// - keep S236-A/B behavior and add one extra bounded central-fast retry only
//   after inbox miss + first central miss.
//
// Scope:
// - medium only, reuses S236 sc range
// - default OFF (screening candidate)
//
#ifndef HZ3_S236E_MINI_CENTRAL_RETRY
#define HZ3_S236E_MINI_CENTRAL_RETRY 0
#endif

#ifndef HZ3_S236E_MINI_CENTRAL_RETRY_WANT
#define HZ3_S236E_MINI_CENTRAL_RETRY_WANT 2
#endif

// 1: retry only when central has non-empty hint.
// 0: always issue retry pop.
#ifndef HZ3_S236E_MINI_CENTRAL_RETRY_REQUIRE_SUPPLY_HINT
#define HZ3_S236E_MINI_CENTRAL_RETRY_REQUIRE_SUPPLY_HINT 1
#endif

#if HZ3_S236E_MINI_CENTRAL_RETRY < 0 || HZ3_S236E_MINI_CENTRAL_RETRY > 1
#error "HZ3_S236E_MINI_CENTRAL_RETRY must be 0 or 1"
#endif

#if HZ3_S236E_MINI_CENTRAL_RETRY_WANT < 1
#error "HZ3_S236E_MINI_CENTRAL_RETRY_WANT must be >= 1"
#endif

#if HZ3_S236E_MINI_CENTRAL_RETRY_WANT > 16
#error "HZ3_S236E_MINI_CENTRAL_RETRY_WANT must be <= 16"
#endif

#if HZ3_S236E_MINI_CENTRAL_RETRY_REQUIRE_SUPPLY_HINT < 0 || HZ3_S236E_MINI_CENTRAL_RETRY_REQUIRE_SUPPLY_HINT > 1
#error "HZ3_S236E_MINI_CENTRAL_RETRY_REQUIRE_SUPPLY_HINT must be 0 or 1"
#endif

// ============================================================================
// S236-F: mini-refill central retry with miss-streak gate (aggressive opt-in)
// ============================================================================
//
// Goal:
// - avoid S236-E-style always/mostly-skipped retry overhead by issuing retry
//   only on sustained mini-refill central misses.
//
// Scope:
// - medium only, reuses S236 sc range
// - default OFF (screening candidate)
//
#ifndef HZ3_S236F_MINI_CENTRAL_RETRY
#define HZ3_S236F_MINI_CENTRAL_RETRY 0
#endif

#ifndef HZ3_S236F_MINI_CENTRAL_RETRY_WANT
#define HZ3_S236F_MINI_CENTRAL_RETRY_WANT 1
#endif

#ifndef HZ3_S236F_MISS_STREAK_MIN
#define HZ3_S236F_MISS_STREAK_MIN 3
#endif

#ifndef HZ3_S236F_RETRY_EVERY
#define HZ3_S236F_RETRY_EVERY 8
#endif

#if HZ3_S236F_MINI_CENTRAL_RETRY < 0 || HZ3_S236F_MINI_CENTRAL_RETRY > 1
#error "HZ3_S236F_MINI_CENTRAL_RETRY must be 0 or 1"
#endif

#if HZ3_S236F_MINI_CENTRAL_RETRY_WANT < 1
#error "HZ3_S236F_MINI_CENTRAL_RETRY_WANT must be >= 1"
#endif

#if HZ3_S236F_MINI_CENTRAL_RETRY_WANT > 16
#error "HZ3_S236F_MINI_CENTRAL_RETRY_WANT must be <= 16"
#endif

#if HZ3_S236F_MISS_STREAK_MIN < 1
#error "HZ3_S236F_MISS_STREAK_MIN must be >= 1"
#endif

#if HZ3_S236F_RETRY_EVERY < 1
#error "HZ3_S236F_RETRY_EVERY must be >= 1"
#endif

// ============================================================================
// S236-G: mini-refill central hint gate (aggressive opt-in)
// ============================================================================
//
// Goal:
// - avoid useless mini-refill central pop attempts when central-fast appears
//   empty for the current (shard,sc).
//
// Scope:
// - medium only, reuses S236 sc range
// - default OFF (screening candidate)
//
#ifndef HZ3_S236G_MINI_CENTRAL_HINT_GATE
#define HZ3_S236G_MINI_CENTRAL_HINT_GATE 0
#endif

#if HZ3_S236G_MINI_CENTRAL_HINT_GATE < 0 || HZ3_S236G_MINI_CENTRAL_HINT_GATE > 1
#error "HZ3_S236G_MINI_CENTRAL_HINT_GATE must be 0 or 1"
#endif

// ============================================================================
// S236-I: mini-refill inbox second-chance (consumer-path, opt-in)
// ============================================================================
//
// Goal:
// - keep producer path unchanged and reduce full alloc_slow entries by adding
//   one cheap inbox second-chance at the tail of mini-refill.
//
// Scope:
// - medium only (reuses S236 sc range)
// - default OFF (screening candidate)
//
#ifndef HZ3_S236I_MINI_INBOX_SECOND_CHANCE
#define HZ3_S236I_MINI_INBOX_SECOND_CHANCE 0
#endif

// 1: require remote_hint before issuing second-chance inbox drain.
// 0: allow second-chance based only on backlog hint.
#ifndef HZ3_S236I_REQUIRE_REMOTE_HINT
#define HZ3_S236I_REQUIRE_REMOTE_HINT 1
#endif

// Backlog threshold for second-chance inbox drain.
#ifndef HZ3_S236I_INBOX_BACKLOG_MIN
#define HZ3_S236I_INBOX_BACKLOG_MIN 1
#endif

#if HZ3_S236I_MINI_INBOX_SECOND_CHANCE < 0 || HZ3_S236I_MINI_INBOX_SECOND_CHANCE > 1
#error "HZ3_S236I_MINI_INBOX_SECOND_CHANCE must be 0 or 1"
#endif

#if HZ3_S236I_REQUIRE_REMOTE_HINT < 0 || HZ3_S236I_REQUIRE_REMOTE_HINT > 1
#error "HZ3_S236I_REQUIRE_REMOTE_HINT must be 0 or 1"
#endif

#if HZ3_S236I_INBOX_BACKLOG_MIN < 0
#error "HZ3_S236I_INBOX_BACKLOG_MIN must be >= 0"
#endif

#if HZ3_S236I_MINI_INBOX_SECOND_CHANCE && !HZ3_S236_MINIREFILL_TRY_INBOX
#error "HZ3_S236I_MINI_INBOX_SECOND_CHANCE requires HZ3_S236_MINIREFILL_TRY_INBOX=1"
#endif

// ============================================================================
// S236-N: alloc_slow bounded bulk refill (consumer-path, opt-in)
// ============================================================================
//
// Goal:
// - when alloc_slow gets one object from inbox/central, pull a few extra
//   objects from the same source to reduce repeated slow entries.
//
// Scope:
// - medium only (reuses S236 range by default)
// - default OFF (screening candidate)
//
#ifndef HZ3_S236N_ALLOC_SLOW_BULK_REFILL
#define HZ3_S236N_ALLOC_SLOW_BULK_REFILL 1
#endif

#ifndef HZ3_S236N_SC_MIN
#define HZ3_S236N_SC_MIN HZ3_S236_SC_MIN
#endif

#ifndef HZ3_S236N_SC_MAX
#define HZ3_S236N_SC_MAX HZ3_S236_SC_MAX
#endif

#ifndef HZ3_S236N_BULK_WANT
#define HZ3_S236N_BULK_WANT 4
#endif

#if HZ3_S236N_ALLOC_SLOW_BULK_REFILL < 0 || HZ3_S236N_ALLOC_SLOW_BULK_REFILL > 1
#error "HZ3_S236N_ALLOC_SLOW_BULK_REFILL must be 0 or 1"
#endif

#if HZ3_S236N_SC_MIN < 0
#error "HZ3_S236N_SC_MIN must be >= 0"
#endif

#if HZ3_S236N_SC_MAX < HZ3_S236N_SC_MIN
#error "HZ3_S236N_SC_MAX must be >= HZ3_S236N_SC_MIN"
#endif

#if HZ3_S236N_BULK_WANT < 2
#error "HZ3_S236N_BULK_WANT must be >= 2"
#endif

#if HZ3_S236N_BULK_WANT > 8
#error "HZ3_S236N_BULK_WANT must be <= 8"
#endif

// ============================================================================
// S236-C: Pressure-aware BatchIt-lite (aggressive opt-in)
// ============================================================================
//
// Goal:
// - reduce medium remote singleton publish fixed cost by batching n==1
//   dispatches in a tiny TLS cache keyed by (dst,sc).
// - avoid starvation by bypassing batching when owner inbox backlog is low.
//
// Scope:
// - medium only, default sc=5..7
// - default OFF (aggressive lane experiment)
//
#ifndef HZ3_S236_BATCHIT_LITE
#define HZ3_S236_BATCHIT_LITE 0
#endif

#ifndef HZ3_S236_BATCH_CACHE_ENTRIES
#define HZ3_S236_BATCH_CACHE_ENTRIES 16
#endif

#ifndef HZ3_S236_BATCH_FLUSH_N
#define HZ3_S236_BATCH_FLUSH_N 8
#endif

#ifndef HZ3_S236_PRESSURE_ENABLE
#define HZ3_S236_PRESSURE_ENABLE 1
#endif

#ifndef HZ3_S236_PRESSURE_INBOX_MIN
#define HZ3_S236_PRESSURE_INBOX_MIN 8
#endif

#if HZ3_S236_BATCH_CACHE_ENTRIES < 1
#error "HZ3_S236_BATCH_CACHE_ENTRIES must be >= 1"
#endif

#if HZ3_S236_BATCH_FLUSH_N < 2
#error "HZ3_S236_BATCH_FLUSH_N must be >= 2"
#endif

#if HZ3_S236_PRESSURE_ENABLE < 0 || HZ3_S236_PRESSURE_ENABLE > 1
#error "HZ3_S236_PRESSURE_ENABLE must be 0 or 1"
#endif

#if HZ3_S236_PRESSURE_INBOX_MIN < 0
#error "HZ3_S236_PRESSURE_INBOX_MIN must be >= 0"
#endif
