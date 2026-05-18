// S65: Always-On Release (Boundary + Ledger + Coverage)
// ============================================================================
//
// - ReleaseBoundaryBox: Single entry point for free_bits updates
// - ReleaseLedgerBox: Range coalesce + deferred purge
// - CoverageBox: Unify small/sub4k/medium retire paths
// - Idle/BusyGate: Adjust purge aggressiveness based on pressure
//
// When HZ3_S65_RELEASE_BOUNDARY=1, ALL retire paths must go through
// hz3_release_range_definitely_free() - no direct hz3_bitmap_mark_free() calls.

// Master enable for ReleaseBoundaryBox
#ifndef HZ3_S65_RELEASE_BOUNDARY
#define HZ3_S65_RELEASE_BOUNDARY 0
#endif

// Enable ReleaseLedgerBox (requires BOUNDARY)
#ifndef HZ3_S65_RELEASE_LEDGER
#define HZ3_S65_RELEASE_LEDGER 0
#endif

// Enable Coverage expansion (small/sub4k/medium unified retire)
#ifndef HZ3_S65_RELEASE_COVERAGE
#define HZ3_S65_RELEASE_COVERAGE 0
#endif

// Enable Idle/Busy Gate (adjust delay/budget based on pressure)
#ifndef HZ3_S65_IDLE_GATE
#define HZ3_S65_IDLE_GATE 0
#endif

// Ledger ring buffer size (power of 2)
#ifndef HZ3_S65_LEDGER_SIZE
#define HZ3_S65_LEDGER_SIZE 256
#endif

// Max pages to purge per epoch tick
#ifndef HZ3_S65_PURGE_BUDGET_PAGES
#define HZ3_S65_PURGE_BUDGET_PAGES 512
#endif

// Max madvise calls per epoch tick
#ifndef HZ3_S65_PURGE_MAX_CALLS
#define HZ3_S65_PURGE_MAX_CALLS 64
#endif

// Delay epochs before purge (idle mode)
#ifndef HZ3_S65_DELAY_EPOCHS_IDLE
#define HZ3_S65_DELAY_EPOCHS_IDLE 0
#endif

// Delay epochs before purge (busy mode)
#ifndef HZ3_S65_DELAY_EPOCHS_BUSY
#define HZ3_S65_DELAY_EPOCHS_BUSY 4
#endif

// Stats: atexit one-shot dump
#ifndef HZ3_S65_STATS
#define HZ3_S65_STATS 0
#endif

// Coalesce scan depth (how many recent entries to check for merge)
#ifndef HZ3_S65_COALESCE_SCAN_DEPTH
#define HZ3_S65_COALESCE_SCAN_DEPTH 8
#endif
// S65: Validate pages are still free before madvise (prevents purge of live pages)
#ifndef HZ3_S65_LEDGER_VALIDATE_FREE
#define HZ3_S65_LEDGER_VALIDATE_FREE 1
#endif

// S65-2C: Medium epoch reclaim (独立フラグ)
// central から run を pop して boundary API 経由で free_bits を更新
// HZ3_S65_RELEASE_BOUNDARY=1 が前提
#ifndef HZ3_S65_MEDIUM_RECLAIM
#define HZ3_S65_MEDIUM_RECLAIM 0
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_MODE
// 0=auto (purge_only for now), 1=release to seg free_bits (original, repro only), 2=purge_only (madvise + return to central)
#define HZ3_S65_MEDIUM_RECLAIM_MODE 0
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS
#define HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS 512
#endif

// S194: MediumDemandReclaimBox (opt-in)
// Trigger S65 medium reclaim from central-miss boundary instead of only epoch.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND 0
#endif

// Demand trigger range (size-class scope)
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN 5
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX 7
#endif

// Run demand reclaim once per N central-miss events.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE 8
#endif

// Budget cap for demand reclaim (runs per trigger).
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS 64
#endif

// Optional scale by refill want: budget=max(base, want*scale). 0=disabled.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_SCALE
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_SCALE 0
#endif

// Guard: skip demand reclaim while remote-hint is active.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_SKIP_REMOTE_HINT
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_SKIP_REMOTE_HINT 1
#endif

#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN > HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX
#error "HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN must be <= HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX"
#endif

#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE < 1
#error "HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS < 1
#error "HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS must be >= 1"
#endif

// S65-2C: Guard release mode when shards collide (multiple threads share shard id).
#ifndef HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD
#define HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD 1
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_COLLISION_FAILFAST
#define HZ3_S65_MEDIUM_RECLAIM_COLLISION_FAILFAST 0
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT
#define HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT 1
#endif

// S65 cold central quarantine for purge-only medium runs.
// Opt-in research lane:
// - purge-only reclaim can park already-purged runs outside hot central
// - allocation slow path can use cold central only after hot central misses
// - no metadata is cleared and no decommit happens here
#ifndef HZ3_S65_CENTRAL_COLD_ENABLE
#define HZ3_S65_CENTRAL_COLD_ENABLE 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_PARK_PURGED
#define HZ3_S65_CENTRAL_COLD_PARK_PURGED HZ3_S65_CENTRAL_COLD_ENABLE
#endif

#ifndef HZ3_S65_CENTRAL_COLD_POP_ON_MISS
#define HZ3_S65_CENTRAL_COLD_POP_ON_MISS HZ3_S65_CENTRAL_COLD_ENABLE
#endif

// Keep this many already-purged medium runs in hot central per shard/size-class
// before parking the rest in cold central. Default 0 preserves the pure
// quarantine behavior for repeatable A/B tests.
#ifndef HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS
#define HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS 0
#endif

// Store cold-central list links outside reusable run memory. This is a
// prerequisite for any future MEM_DECOMMIT experiment: decommitted run pages
// cannot be used as intrusive list nodes.
#ifndef HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
#define HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP
#define HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP 262144
#endif

// Narrow Windows RSS experiment: decommit only cold-central runs whose list
// metadata lives outside the run. Recommit happens before the run is returned
// from cold central. This must never be enabled with the intrusive cold list.
#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES 1
#endif

// Keep this many cold-central runs committed per shard/size-class before
// decommitting additional cold runs. This is a reuse/RSS tradeoff knob for
// Windows MEM_DECOMMIT research lanes; 0 preserves decommit-all behavior.
#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS 0
#endif

// Optional small-run override for the committed cold reserve. This lets RSS
// experiments keep cheap-to-reuse 4K/8K runs ready while still decommitting
// larger cold runs more aggressively.
#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS
#endif

#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES 2
#endif

// When the committed cold list is empty, recommit up to this many decommitted
// cold runs back into the committed cold reserve before serving allocation.
// 1 preserves the original one-run-at-a-time cold pop behavior.
#ifndef HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS
#define HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS 1
#endif

// Restrict batch recommit to cold runs with at least this many pages.
// This lets research lanes try large-run prewarm without changing 4K/8K reuse.
#ifndef HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES
#define HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES 1
#endif

// Move MEM_COMMIT for decommitted cold runs outside the cold-bin lock.
// This does not reduce the number of Windows commit calls; it tests whether
// lock hold time is the foreground throughput/tail-latency tax.
#ifndef HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK
#define HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK 0
#endif

// Route medium remote frees for dead owner shards to central instead of
// leaving them in an inbox that no live owner can drain. This is off by default
// while the RSS/speed tradeoff is measured.
#ifndef HZ3_S65_ORPHAN_INBOX_TO_CENTRAL
#define HZ3_S65_ORPHAN_INBOX_TO_CENTRAL 0
#endif

// More aggressive inbox-routing research knob. When non-negative, medium
// remote frees for owner shards with live_count <= this value are routed to
// central instead of the owner inbox. The default keeps the behavior disabled;
// `0` is equivalent to the orphan-only policy, while `1` probes owner-exit
// tail behavior where the owner is still live but unlikely to drain enough.
#ifndef HZ3_S65_INBOX_TO_CENTRAL_MAX_LIVE_COUNT
#define HZ3_S65_INBOX_TO_CENTRAL_MAX_LIVE_COUNT -1
#endif

// Narrower owner-exit routing probe. A shard is marked exiting at the start of
// its tcache destructor and the flag is cleared when a future thread claims the
// shard exclusively. This catches the destructor window without bypassing the
// normal inbox fast path for healthy single-owner shards.
#ifndef HZ3_S65_EXITING_INBOX_TO_CENTRAL
#define HZ3_S65_EXITING_INBOX_TO_CENTRAL 0
#endif

// Size-class gate for the owner-exit routing probe. The default covers all
// medium classes and preserves the original `exiting-central` behavior. Set
// MIN/MAX to 16 for a 64K-only diagnostic lane.
#ifndef HZ3_S65_EXITING_INBOX_TO_CENTRAL_MIN_PAGES
#define HZ3_S65_EXITING_INBOX_TO_CENTRAL_MIN_PAGES 1
#endif

#ifndef HZ3_S65_EXITING_INBOX_TO_CENTRAL_MAX_PAGES
#define HZ3_S65_EXITING_INBOX_TO_CENTRAL_MAX_PAGES 16
#endif

// Optional bitmask for page-count selection. Bit N controls N-page medium runs.
// The default enables pages 1..16. Example: 0x10002 enables 4K and 64K only.
#ifndef HZ3_S65_EXITING_INBOX_TO_CENTRAL_PAGE_MASK
#define HZ3_S65_EXITING_INBOX_TO_CENTRAL_PAGE_MASK 0x0001fffeu
#endif

#if HZ3_S65_EXITING_INBOX_TO_CENTRAL_MIN_PAGES < 1
#error "HZ3_S65_EXITING_INBOX_TO_CENTRAL_MIN_PAGES must be >= 1"
#endif

#if HZ3_S65_EXITING_INBOX_TO_CENTRAL_MAX_PAGES < HZ3_S65_EXITING_INBOX_TO_CENTRAL_MIN_PAGES
#error "HZ3_S65_EXITING_INBOX_TO_CENTRAL_MAX_PAGES must be >= MIN_PAGES"
#endif

// If enabled, mark the owner shard as exiting at the start of the destructor.
// If disabled, mark it after the destructor inbox drain/reclaim pass. The late
// mark is a narrower probe for the final orphan window.
#ifndef HZ3_S65_EXITING_MARK_AT_DTOR_START
#define HZ3_S65_EXITING_MARK_AT_DTOR_START 1
#endif

// Destructor-local final inbox drain. This preserves the normal remote-free
// inbox fast path and drains again immediately before the owner shard live
// count is decremented. It is a narrow alternative to routing every
// owner-exiting remote free through central.
#ifndef HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS
#define HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS 0
#endif

#ifndef HZ3_S65_DTOR_FINAL_RECLAIM_AFTER_DRAIN
#define HZ3_S65_DTOR_FINAL_RECLAIM_AFTER_DRAIN 0
#endif

#if HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS < 0
#error "HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS must be >= 0"
#endif

// S260: when remote frees are routed to central because the owner shard is
// dead/exiting, immediately reclaim the target owner shard and size class.
// This is narrower than increasing the global reclaim budget because the call
// follows the exact central push that would otherwise remain hot until a later
// owner-local tick.
#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM 0
#endif

#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE 1
#endif

#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS 4096
#endif

#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_ALLOW_LIVE_EXITING
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_ALLOW_LIVE_EXITING 0
#endif

#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES 1
#endif

#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES 16
#endif

// Optional bitmask for targeted reclaim selection. Bit N controls N-page
// medium runs. The default enables pages 1..16.
#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_PAGE_MASK
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_PAGE_MASK 0x0001fffeu
#endif

// Trigger only every Nth eligible event. This lets research lanes keep some
// hot-central reuse while still returning part of the dead-owner backlog.
#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE 1
#endif

// Accumulate eligible dead-owner inbox-to-central events and reclaim them as a
// larger batch. Unlike STRIDE, this preserves the amount of work while reducing
// how often the reclaim/decommit path is entered.
#ifndef HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS
#define HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS 1
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE < 1
#error "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE must be >= 1"
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS < 1
#error "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS must be >= 1"
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES < 1
#error "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES must be >= 1"
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES < HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES
#error "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES must be >= MIN_PAGES"
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE < 1
#error "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE must be >= 1"
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS < 1
#error "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS must be >= 1"
#endif

// Experimental ready-cache policy for cold medium runs. The old reserve policy
// keeps the first committed runs it sees; this mode keeps newly parked cold runs
// ready at the MRU head and evicts old ready runs from the LRU tail.
#ifndef HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
#define HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS
#define HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS
#endif

#ifndef HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS
#define HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS
#endif

#ifndef HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS
#define HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS 16
#endif

#ifndef HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES
#define HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES 16
#endif

// Optional MRU last-resort rewarm for small/medium cold runs. The first MRU
// design intentionally avoids foreground recommit for 4K/8K, but diagnostics
// showed those misses can fall through to fresh segment allocation. This flag
// allows an A/B lane that recommits a tiny batch before allocating new segment
// pages.
#ifndef HZ3_S65_CENTRAL_COLD_SMALL_REWARM_ON_MISS
#define HZ3_S65_CENTRAL_COLD_SMALL_REWARM_ON_MISS 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_SMALL_REWARM_BATCH_RUNS
#define HZ3_S65_CENTRAL_COLD_SMALL_REWARM_BATCH_RUNS 1
#endif

#ifndef HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES
#define HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES 1
#endif

#ifndef HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES
#define HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES 2
#endif

#ifndef HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST
#define HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST 1
#endif

// Coalesce adjacent decommitted cold runs during MRU rewarm so one
// VirtualAlloc(MEM_COMMIT) can cover multiple contiguous runs. This is a
// research knob for reducing Windows VM syscall count on large medium classes.
#ifndef HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
#define HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE 0
#endif

// Observation-only probe for future coalesced MEM_DECOMMIT. This keeps the
// current per-run decommit behavior but records how many VirtualFree calls
// could be saved if adjacent cold runs were decommitted as sorted ranges.
#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS 64
#endif

// Actual coalesced MEM_DECOMMIT experiment. This detaches a batch of old
// COLD_READY runs from the MRU/LRU ready deque, sorts by address, and
// decommits adjacent runs as ranges. It is intentionally gated behind the MRU
// cold-ready policy because the LRU tail gives a clean eviction batch.
#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE 0
#endif

#ifndef HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS
#define HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS 64
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && !HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE requires HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES < 1
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS < 1
#error "HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES < 1
#error "HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS < 0
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS must be >= 0"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES < 1
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK && !HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#error "HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK requires HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE && !HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#error "HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE requires HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS < 0
#error "HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS must be >= 0"
#endif

#if HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS < 0
#error "HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS must be >= 0"
#endif

#if HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS < 0
#error "HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS must be >= 0"
#endif

#if HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES < 1
#error "HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_ON_MISS && !HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
#error "HZ3_S65_CENTRAL_COLD_SMALL_REWARM_ON_MISS requires HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_BATCH_RUNS < 1
#error "HZ3_S65_CENTRAL_COLD_SMALL_REWARM_BATCH_RUNS must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES < 1
#error "HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES < 1
#error "HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES < HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES
#error "HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES must be >= HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES"
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE && !HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
#error "HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE requires HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE && !HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE requires HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS < 1
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS > 256
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS must be <= 256"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE && !HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE requires HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS < 1
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS must be >= 1"
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS > 256
#error "HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS must be <= 256"
#endif

// S80: Medium reclaim runtime gate (env: HZ3_S80_MEDIUM_RECLAIM=0/1)
#ifndef HZ3_S80_MEDIUM_RECLAIM_GATE
#define HZ3_S80_MEDIUM_RECLAIM_GATE 0
#endif

#ifndef HZ3_S80_MEDIUM_RECLAIM_DEFAULT
#define HZ3_S80_MEDIUM_RECLAIM_DEFAULT 1
#endif

#ifndef HZ3_S80_MEDIUM_RECLAIM_LOG
#define HZ3_S80_MEDIUM_RECLAIM_LOG 1
#endif

#ifndef HZ3_OS_PURGE_STATS
#define HZ3_OS_PURGE_STATS 0
#endif
