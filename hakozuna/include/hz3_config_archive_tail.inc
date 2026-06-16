// ============================================================================
// S205: OwnerSleepRescueBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: detect medium inbox backlog with no owner drain progress and
// reroute remote medium list to central.
// NO-GO: RUNS=21 main lane regressed clearly (16..32768 r90: -2.344%).
// Reference: hakozuna/hz3/archive/research/s205_owner_sleep_rescue/README.md

#ifndef HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
#define HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE 0
#endif
#if HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
#error "HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE is archived (NO-GO). See hakozuna/hz3/archive/research/s205_owner_sleep_rescue/README.md"
#endif

// ============================================================================
// S206B: MediumDstMixedBatchBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce remote medium inbox atomic frequency by batching per-dst
// into mixed queues and decoding sc on owner drain.
// NO-GO: main lane throughput regressed heavily and RSS/page-faults increased.
// Reference: hakozuna/hz3/archive/research/s206b_medium_dst_mixed_batch/README.md

#ifndef HZ3_S206B_MEDIUM_DST_MIXED_BATCH
#define HZ3_S206B_MEDIUM_DST_MIXED_BATCH 0
#endif
#if HZ3_S206B_MEDIUM_DST_MIXED_BATCH
#error "HZ3_S206B_MEDIUM_DST_MIXED_BATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s206b_medium_dst_mixed_batch/README.md"
#endif

// ============================================================================
// S208: MediumCentralFloorReserveBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: on medium central-miss boundary, reserve extra segment fallback
// runs into central up to a per-(shard,sc) floor to reduce from_segment share.
// NO-GO: repeated RUNS=21 replays showed unstable sign and guard sensitivity.
// Recent deepcheck reproduced guard gate violation with identical SO hashes.
// Reference: hakozuna/hz3/archive/research/s208_medium_central_floor_reserve/README.md

#ifndef HZ3_S208_MEDIUM_CENTRAL_RESERVE
#define HZ3_S208_MEDIUM_CENTRAL_RESERVE 0
#endif
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE
#error "HZ3_S208_MEDIUM_CENTRAL_RESERVE is archived (NO-GO). See hakozuna/hz3/archive/research/s208_medium_central_floor_reserve/README.md"
#endif

// ============================================================================
// S211: MediumSegmentReserveLiteBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce medium from_segment fallback with lower fixed cost than
// S208 by sparse reserve publish at central-miss boundary.
// NO-GO: after short-sweep retune, RUNS=21 main lane regressed and RSS rose
// sharply; no stable promotion point was found for single-default.
// Reference: hakozuna/hz3/archive/research/s211_medium_segment_reserve_lite/README.md

#ifndef HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
#define HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE 0
#endif
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
#error "HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE is archived (NO-GO). See hakozuna/hz3/archive/research/s211_medium_segment_reserve_lite/README.md"
#endif

// ============================================================================
// S220: CPU-local Remote Recycle Queue (RRQ) (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: bypass medium inbox dispatch by reusing remote-freed objects via
// per-CPU queue before inbox/central path.
// NO-GO: replay-stable regression on main lane and catastrophic larson
// regression (inbox batch recycle collapse + slowpath amplification).
// Reference: hakozuna/hz3/archive/research/s220_cpu_rrq/README.md

#ifndef HZ3_S220_CPU_RRQ
#define HZ3_S220_CPU_RRQ 0
#endif
#if HZ3_S220_CPU_RRQ
#error "HZ3_S220_CPU_RRQ is archived (NO-GO). See hakozuna/hz3/archive/research/s220_cpu_rrq/README.md"
#endif

// ============================================================================
// S221: MediumDispatchScopedMinibatchBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: batch remote medium dispatch for n==1 dominant streams while
// preserving inbox recycle path and guard behavior.
// NO-GO: main improved, but guard lane failed replay-stably in RUNS=10 screen
// and recheck (`-1.233%`, `-2.486%`); no safe promotion point found.
// Reference: hakozuna/hz3/archive/research/s221_medium_dispatch_minibatch/README.md

#ifndef HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
#define HZ3_S221_MEDIUM_DISPATCH_MINIBATCH 0
#endif
#if HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
#error "HZ3_S221_MEDIUM_DISPATCH_MINIBATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s221_medium_dispatch_minibatch/README.md"
#endif

// ============================================================================
// S223: Medium alloc_slow Dynamic Want (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: increase medium refill "want" after central-miss streak to reduce
// repeated central-miss -> segment fallback churn on sc=5..7.
// NO-GO: RUNS=10 screen showed unstable negative sign on main/guard and failed
// guard gate on recheck (`guard < -1.0%`); no safe promotion point found.
// Reference: hakozuna/hz3/archive/research/s223_medium_dynamic_want/README.md

#ifndef HZ3_S223_DYNAMIC_WANT
#define HZ3_S223_DYNAMIC_WANT 0
#endif
#if HZ3_S223_DYNAMIC_WANT
#error "HZ3_S223_DYNAMIC_WANT is archived (NO-GO). See hakozuna/hz3/archive/research/s223_medium_dynamic_want/README.md"
#endif

// ============================================================================
// S224: Medium Remote local n==1 pair-batch (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce n==1-dominant medium remote publish fixed cost by keeping
// one pending object per (dst,sc) and publishing pair-lists when possible.
// NO-GO: RUNS=10 screen was negative on main and failed guard gate
// (`main -1.889%`, `guard -1.513%`) despite larson positive.
// Reference: hakozuna/hz3/archive/research/s224_medium_n1_pair_batch/README.md

#ifndef HZ3_S224_MEDIUM_N1_PAIR_BATCH
#define HZ3_S224_MEDIUM_N1_PAIR_BATCH 0
#endif
#if HZ3_S224_MEDIUM_N1_PAIR_BATCH
#error "HZ3_S224_MEDIUM_N1_PAIR_BATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s224_medium_n1_pair_batch/README.md"
#endif

// ============================================================================
// S225: Medium Remote n==1 last-key pair-batch (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce n==1 medium remote publish fixed cost with one TLS pending
// slot (last dst/sc) and immediate n==2 publish on key hit.
// NO-GO: RUNS=10 screen failed main gate (`main -1.014%`) even though
// guard/larson were positive, so no safe default promotion point was found.
// Reference: hakozuna/hz3/archive/research/s225_medium_n1_lastkey_pair_batch/README.md

#ifndef HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
#define HZ3_S225_MEDIUM_N1_LASTKEY_BATCH 0
#endif
#if HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
#error "HZ3_S225_MEDIUM_N1_LASTKEY_BATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s225_medium_n1_lastkey_pair_batch/README.md"
#endif

// ============================================================================
// S226: Medium flush-scope bucket3 (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce n==1-dominant medium remote publish fixed cost by
// grouping entries only inside one flush window by (dst,sc5..7).
// NO-GO: RUNS=10 screen was only marginally positive on main while guard
// failed gate (`main +0.466%`, `guard -2.428%`), so no safe default point.
// Reference: hakozuna/hz3/archive/research/s226_medium_flush_bucket3/README.md

#ifndef HZ3_S226_MEDIUM_FLUSH_BUCKET3
#define HZ3_S226_MEDIUM_FLUSH_BUCKET3 0
#endif
#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
#error "HZ3_S226_MEDIUM_FLUSH_BUCKET3 is archived (NO-GO). See hakozuna/hz3/archive/research/s226_medium_flush_bucket3/README.md"
#endif

// ============================================================================
// S228: Central Fast local remainder cache (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce S222 fast-pop repush fixed cost by keeping remainder in
// TLS when shard is exclusive (`live_count==1`), then serving future pops from
// TLS first.
// NO-GO: mechanism worked (`pop_repush/pop_hits` dropped to ~0), but source mix
// worsened (`from_central`/`from_segment` up) and throughput regressed
// replay-stably on screen gate.
// Reference: hakozuna/hz3/archive/research/s228_central_fast_local_remainder/README.md

#ifndef HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
#define HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER 0
#endif
#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
#error "HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER is archived (NO-GO). See hakozuna/hz3/archive/research/s228_central_fast_local_remainder/README.md"
#endif

// ============================================================================
// S229: Medium alloc_slow central-first (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: for medium hot classes (`sc=5..7`), try central-fast before
// inbox drain to reduce inbox-side fixed cost.
// NO-GO: path engaged but source mix shifted to central-heavy and slowpath
// amplification caused main-lane collapse replay-stably.
// Reference: hakozuna/hz3/archive/research/s229_central_first/README.md

#ifndef HZ3_S229_CENTRAL_FIRST
#define HZ3_S229_CENTRAL_FIRST 0
#endif
#if HZ3_S229_CENTRAL_FIRST
#error "HZ3_S229_CENTRAL_FIRST is archived (NO-GO). See hakozuna/hz3/archive/research/s229_central_first/README.md"
#endif

// ============================================================================
// S230: Medium n==1 -> central-fast direct (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: route medium n==1 remote dispatch directly to central-fast
// (S222) to reduce inbox publish fixed cost under n==1-dominant streams.
// NO-GO: mechanism engaged strongly (`to_central_s230` jumped), but source mix
// shifted to central-heavy + slowpath amplification and main throughput
// collapsed replay-stably on screen gate.
// Reference: hakozuna/hz3/archive/research/s230_medium_n1_to_central_fast/README.md

#ifndef HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
#define HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST 0
#endif
#if HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
#error "HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST is archived (NO-GO). See hakozuna/hz3/archive/research/s230_medium_n1_to_central_fast/README.md"
#endif

// ============================================================================
// S231: Medium inbox fast MPSC (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce medium inbox fixed cost by forcing lane=0 push/drain and
// skipping push-seq updates on the same path (`sc=5..7`).
// NO-GO: mechanism did not change the dominant dispatch/source mix and failed
// replay gate on guard (RUNS=10), so no promotion point was found.
// Reference: hakozuna/hz3/archive/research/s231_medium_inbox_fast_mpsc/README.md

#ifndef HZ3_S231_INBOX_FAST_MPSC
#define HZ3_S231_INBOX_FAST_MPSC 0
#endif
#if HZ3_S231_INBOX_FAST_MPSC
#error "HZ3_S231_INBOX_FAST_MPSC is archived (NO-GO). See hakozuna/hz3/archive/research/s231_medium_inbox_fast_mpsc/README.md"
#endif

// ============================================================================
// S232: Large aggressive retain preset (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce hz3 large-path mmap/munmap churn on 64KB..128KB by
// forcing larger retain budget caps.
// NO-GO: large-only lane regressed strongly (`-22.73%`) and churn counters
// worsened (`alloc_cache_misses/free_munmap` increased), so promotion point
// was not found.
// Reference: hakozuna/hz3/archive/research/s232_large_aggressive/README.md

#ifndef HZ3_S232_LARGE_AGGRESSIVE
#define HZ3_S232_LARGE_AGGRESSIVE 0
#endif
#if HZ3_S232_LARGE_AGGRESSIVE
#error "HZ3_S232_LARGE_AGGRESSIVE is archived (NO-GO). See hakozuna/hz3/archive/research/s232_large_aggressive/README.md"
#endif

// ============================================================================
// S234: Central fast partial-pop CAS (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce S222 pop->repush fixed cost by partial CAS pop of only
// `want` nodes from fast head.
// NO-GO: mechanism engaged (`pop_repush=0`, `s234_hits` high) but replay-stable
// regressions on both main and guard lanes.
// Reference: hakozuna/hz3/archive/research/s234_central_fast_partial_pop/README.md

#ifndef HZ3_S234_CENTRAL_FAST_PARTIAL_POP
#define HZ3_S234_CENTRAL_FAST_PARTIAL_POP 0
#endif
#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
#error "HZ3_S234_CENTRAL_FAST_PARTIAL_POP is archived (NO-GO). See hakozuna/hz3/archive/research/s234_central_fast_partial_pop/README.md"
#endif

// ============================================================================
// S236-C: Pressure-aware BatchIt-lite (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: producer-side pressure-aware batching for medium n==1 stream.
// NO-GO: mechanism engaged but throughput screen gate was not met.
// Reference: hakozuna/hz3/archive/research/s236c_medium_batchit_lite/README.md

#ifndef HZ3_S236_BATCHIT_LITE
#define HZ3_S236_BATCHIT_LITE 0
#endif
#if HZ3_S236_BATCHIT_LITE
#error "HZ3_S236_BATCHIT_LITE is archived (NO-GO). See hakozuna/hz3/archive/research/s236c_medium_batchit_lite/README.md"
#endif

// ============================================================================
// S236-D: Medium mailbox multi-slot (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce mailbox overflow fallback by increasing mailbox slots.
// NO-GO: fallback reduction was real but main throughput gain was too small.
// Reference: hakozuna/hz3/archive/research/s236d_medium_mailbox_multislot/README.md

#ifndef HZ3_S236_MAILBOX_SLOTS
#define HZ3_S236_MAILBOX_SLOTS 1
#endif
#if HZ3_S236_MAILBOX_SLOTS > 1
#error "HZ3_S236_MAILBOX_SLOTS>1 is archived (NO-GO). See hakozuna/hz3/archive/research/s236d_medium_mailbox_multislot/README.md"
#endif

// ============================================================================
// S236-E/F/G: mini central retry/hint series (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: improve mini-refill central capture with retry/hint gates.
// NO-GO: retries had no effective hits and hint gate had no throughput upside.
// Reference:
//   - hakozuna/hz3/archive/research/s236e_mini_central_retry/README.md
//   - hakozuna/hz3/archive/research/s236f_mini_central_retry_streak/README.md
//   - hakozuna/hz3/archive/research/s236g_mini_central_hint_gate/README.md

#ifndef HZ3_S236E_MINI_CENTRAL_RETRY
#define HZ3_S236E_MINI_CENTRAL_RETRY 0
#endif
#if HZ3_S236E_MINI_CENTRAL_RETRY
#error "HZ3_S236E_MINI_CENTRAL_RETRY is archived (NO-GO). See hakozuna/hz3/archive/research/s236e_mini_central_retry/README.md"
#endif

#ifndef HZ3_S236F_MINI_CENTRAL_RETRY
#define HZ3_S236F_MINI_CENTRAL_RETRY 0
#endif
#if HZ3_S236F_MINI_CENTRAL_RETRY
#error "HZ3_S236F_MINI_CENTRAL_RETRY is archived (NO-GO). See hakozuna/hz3/archive/research/s236f_mini_central_retry_streak/README.md"
#endif

#ifndef HZ3_S236G_MINI_CENTRAL_HINT_GATE
#define HZ3_S236G_MINI_CENTRAL_HINT_GATE 0
#endif
#if HZ3_S236G_MINI_CENTRAL_HINT_GATE
#error "HZ3_S236G_MINI_CENTRAL_HINT_GATE is archived (NO-GO). See hakozuna/hz3/archive/research/s236g_mini_central_hint_gate/README.md"
#endif

// ============================================================================
// S236-I: mini inbox second-chance (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: add one extra inbox probe in mini-refill miss path.
// NO-GO: mechanism was almost never engaged; guard/larson gates failed.
// Reference: hakozuna/hz3/archive/research/s236i_mini_inbox_second_chance/README.md

#ifndef HZ3_S236I_MINI_INBOX_SECOND_CHANCE
#define HZ3_S236I_MINI_INBOX_SECOND_CHANCE 0
#endif
#if HZ3_S236I_MINI_INBOX_SECOND_CHANCE
#error "HZ3_S236I_MINI_INBOX_SECOND_CHANCE is archived (NO-GO). See hakozuna/hz3/archive/research/s236i_mini_inbox_second_chance/README.md"
#endif
