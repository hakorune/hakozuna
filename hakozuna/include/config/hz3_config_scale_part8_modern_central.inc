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

