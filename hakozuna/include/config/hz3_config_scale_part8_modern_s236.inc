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
