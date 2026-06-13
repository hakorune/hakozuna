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

