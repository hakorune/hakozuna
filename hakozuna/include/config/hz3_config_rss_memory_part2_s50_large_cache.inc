
// ============================================================================
// S50: Large Cache Size-Class Box
// ============================================================================

#ifndef HZ3_S50_LARGE_SCACHE
#define HZ3_S50_LARGE_SCACHE 1
#endif

#ifndef HZ3_S50_LARGE_SCACHE_EVICT
#define HZ3_S50_LARGE_SCACHE_EVICT 0
#endif

// S51: madvise(MADV_DONTNEED) on cache push
#ifndef HZ3_S51_LARGE_MADVISE
#define HZ3_S51_LARGE_MADVISE 0
#endif

// S52: Best-fit fallback for large cache
#ifndef HZ3_S52_LARGE_BESTFIT
#define HZ3_S52_LARGE_BESTFIT 0
#endif

#ifndef HZ3_S52_BESTFIT_RANGE
#define HZ3_S52_BESTFIT_RANGE 2
#endif

// S181: lock striping for active-large hash map (insert/take/find-size path)
// 1 = legacy single lock, N(power-of-2) = striped locks.
#ifndef HZ3_S181_LARGE_MAP_LOCK_STRIPES
#define HZ3_S181_LARGE_MAP_LOCK_STRIPES 64
#endif

#if HZ3_S181_LARGE_MAP_LOCK_STRIPES < 1
#error "HZ3_S181_LARGE_MAP_LOCK_STRIPES must be >= 1"
#endif

#if (HZ3_S181_LARGE_MAP_LOCK_STRIPES & (HZ3_S181_LARGE_MAP_LOCK_STRIPES - 1)) != 0
#error "HZ3_S181_LARGE_MAP_LOCK_STRIPES must be power-of-2"
#endif

// S238: LargeDirectHeaderLookupBox (opt-in)
// 0=legacy hash-only take, 1=direct-first + safe fallback to hash walk.
#ifndef HZ3_S238_LARGE_DIRECT_LOOKUP
#define HZ3_S238_LARGE_DIRECT_LOOKUP 0
#endif

// S238 one-shot counters at exit.
#ifndef HZ3_S238_STATS
#define HZ3_S238_STATS 0
#endif

// S182: use userspace spin lock for large-cache global lock to reduce futex/kernel time
// on MT large alloc/free hot path. 0=mutex(hz3_lock_t), 1=atomic_flag spin.
#ifndef HZ3_S182_LARGE_CACHE_SPINLOCK
#define HZ3_S182_LARGE_CACHE_SPINLOCK 0
#endif

// S183: split large cache lock by size class (S50 scache path only).
// 0=single cache lock, 1=per-class list lock + global budget lock.
#ifndef HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#define HZ3_S183_LARGE_CLASS_LOCK_SPLIT 0
#endif

// S184: free-path budget precheck with atomic cached-bytes hint.
// 0=off (legacy), 1=on (skip first budget lock section when likely cacheable).
#ifndef HZ3_S184_LARGE_FREE_PRECHECK
#define HZ3_S184_LARGE_FREE_PRECHECK 0
#endif

// S240: LargeOwnerFront research lane.
// Patch 1 is metadata/counter only: owner shard, large state, and size-class
// observations are recorded without changing cache behavior.
#ifndef HZ3_S240_LARGE_OWNER_FRONT
#define HZ3_S240_LARGE_OWNER_FRONT 0
#endif

#ifndef HZ3_S240_LARGE_OWNER_FRONT_STATS
#define HZ3_S240_LARGE_OWNER_FRONT_STATS HZ3_S240_LARGE_OWNER_FRONT
#endif

// S240 Patch 2: per-thread large front cache for owner-local reuse.
// Remote owner inbox remains a later stage.
#ifndef HZ3_S240_LARGE_FRONT_CACHE
#define HZ3_S240_LARGE_FRONT_CACHE 0
#endif

#ifndef HZ3_S240_LARGE_FRONT_MAX_NODES_PER_SC
#define HZ3_S240_LARGE_FRONT_MAX_NODES_PER_SC 8
#endif

#ifndef HZ3_S240_LARGE_FRONT_MAX_BYTES_PER_THREAD
#define HZ3_S240_LARGE_FRONT_MAX_BYTES_PER_THREAD (2ULL << 20)
#endif

#ifndef HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES
#define HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES (128ULL << 10)
#endif

// S240 Patch 3: owner-shard large remote-free inbox.
// Remote frees can hand cached large blocks back to the allocation owner.
// The owner drains a small batch into the TLS front cache before touching the
// global large cache.  This is research-only and disabled by default.
#ifndef HZ3_S240_LARGE_OWNER_INBOX
#define HZ3_S240_LARGE_OWNER_INBOX 0
#endif

#ifndef HZ3_S240_LARGE_INBOX_DRAIN_BATCH
#define HZ3_S240_LARGE_INBOX_DRAIN_BATCH 8
#endif

#ifndef HZ3_S240_LARGE_INBOX_MAX_NODES_PER_SC
#define HZ3_S240_LARGE_INBOX_MAX_NODES_PER_SC 64
#endif

#ifndef HZ3_S240_LARGE_INBOX_MAX_BYTES_PER_SC
#define HZ3_S240_LARGE_INBOX_MAX_BYTES_PER_SC (8ULL << 20)
#endif

#ifndef HZ3_S240_LARGE_INBOX_DTOR_SPIN_LIMIT
#define HZ3_S240_LARGE_INBOX_DTOR_SPIN_LIMIT 100000
#endif

// S291: narrow high-band owner-front window.
//
// S252 proved that broad high-band owner handoff can recover the 256K/a4096
// row, but it also touches 65537 and wider high-band rows. S291 keeps the
// default S276 <=128KiB band, then adds a separate opt-in window so research
// lanes can target 128KiB<size<=256KiB without promoting a broad 1MiB policy.
#ifndef HZ3_S291_HIGHBAND_OWNER_WINDOW
#define HZ3_S291_HIGHBAND_OWNER_WINDOW 0
#endif

#ifndef HZ3_S291_HIGHBAND_MIN_CLASS_BYTES
#define HZ3_S291_HIGHBAND_MIN_CLASS_BYTES (128ULL << 10)
#endif

#ifndef HZ3_S291_HIGHBAND_MAX_CLASS_BYTES
#define HZ3_S291_HIGHBAND_MAX_CLASS_BYTES (256ULL << 10)
#endif

#if HZ3_S291_HIGHBAND_OWNER_WINDOW && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S291_HIGHBAND_OWNER_WINDOW requires HZ3_S240_LARGE_OWNER_FRONT"
#endif

#if HZ3_S291_HIGHBAND_OWNER_WINDOW && \
    HZ3_S291_HIGHBAND_MIN_CLASS_BYTES >= HZ3_S291_HIGHBAND_MAX_CLASS_BYTES
#error "HZ3_S291_HIGHBAND_MIN_CLASS_BYTES must be smaller than MAX"
#endif

// S292: high-band S291 RSS/route observation.
// Diagnostic-only. Keeps S291 behavior unchanged and records where the
// 128KiB<size<=256KiB retained objects accumulate before designing a cap.
#ifndef HZ3_S292_HIGHBAND_RETENTION_OBS
#define HZ3_S292_HIGHBAND_RETENTION_OBS 0
#endif

#if HZ3_S292_HIGHBAND_RETENTION_OBS && !HZ3_S291_HIGHBAND_OWNER_WINDOW
#error "HZ3_S292_HIGHBAND_RETENTION_OBS requires S291 high-band owner window"
#endif

#if HZ3_S292_HIGHBAND_RETENTION_OBS && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S292_HIGHBAND_RETENTION_OBS requires S240 owner-front stats"
#endif

// S293: cap S291-window objects admitted to the global large scache.
//
// This leaves owner-front and owner-inbox reuse untouched. It only rejects the
// final global-scache fallback once the request-size window already has enough
// retained nodes in that large-cache class.
#ifndef HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP
#define HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP 0
#endif

#ifndef HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC
#define HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC 8
#endif

#if HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP && !HZ3_S291_HIGHBAND_OWNER_WINDOW
#error "HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP requires S291 high-band owner window"
#endif

#if HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC < 1
#error "HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC must be >= 1"
#endif

// S245: bounded batch pop for owner large inbox drain.
// This keeps the same drain limit and caps as S240, but detaches up to N inbox
// nodes with one CAS instead of one CAS per object.
#ifndef HZ3_S245_LARGE_INBOX_BATCH_POP
#define HZ3_S245_LARGE_INBOX_BATCH_POP 0
#endif

#if HZ3_S245_LARGE_INBOX_BATCH_POP && !HZ3_S240_LARGE_OWNER_INBOX
#error "HZ3_S245_LARGE_INBOX_BATCH_POP requires HZ3_S240_LARGE_OWNER_INBOX"
#endif

// S246: return the first owner-inbox drain object directly to allocation.
// Extra drained nodes still refill the TLS front cache.
#ifndef HZ3_S246_LARGE_INBOX_TAKE_FIRST
#define HZ3_S246_LARGE_INBOX_TAKE_FIRST 0
#endif

#if HZ3_S246_LARGE_INBOX_TAKE_FIRST && !HZ3_S240_LARGE_OWNER_INBOX
#error "HZ3_S246_LARGE_INBOX_TAKE_FIRST requires HZ3_S240_LARGE_OWNER_INBOX"
#endif

#if HZ3_S246_LARGE_INBOX_TAKE_FIRST && HZ3_S245_LARGE_INBOX_BATCH_POP
#error "HZ3_S246_LARGE_INBOX_TAKE_FIRST and HZ3_S245_LARGE_INBOX_BATCH_POP are separate probes"
#endif

// S273: use the S245-style batch detach inside the S246 take-first path.
// The first detached node is returned directly to allocation; the remaining
// detached nodes refill the TLS front cache. This keeps S246 semantics while
// reducing repeated owner-inbox CAS work.
#ifndef HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH
#define HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH 0
#endif

#if HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH && !HZ3_S246_LARGE_INBOX_TAKE_FIRST
#error "HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH requires HZ3_S246_LARGE_INBOX_TAKE_FIRST"
#endif

#if HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH && HZ3_S245_LARGE_INBOX_BATCH_POP
#error "HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH uses the S245 batch helper; keep HZ3_S245_LARGE_INBOX_BATCH_POP disabled"
#endif

// S274: size-specific RSS drain/refill probe.
// Keep S246 take-first as the default shape, but use the S245/S273 batch detach
// only for a bounded request-size band. This is off by default and is intended
// for RSS-shape research lanes, not speed-default.
#ifndef HZ3_S274_LARGE_INBOX_SIZE_DRAIN
#define HZ3_S274_LARGE_INBOX_SIZE_DRAIN 0
#endif

#ifndef HZ3_S274_LARGE_INBOX_DRAIN_BATCH
#define HZ3_S274_LARGE_INBOX_DRAIN_BATCH 4
#endif

#ifndef HZ3_S274_LARGE_INBOX_MIN_REQUEST_BYTES
#define HZ3_S274_LARGE_INBOX_MIN_REQUEST_BYTES 8192
#endif

#ifndef HZ3_S274_LARGE_INBOX_MAX_REQUEST_BYTES
#define HZ3_S274_LARGE_INBOX_MAX_REQUEST_BYTES 8192
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN && !HZ3_S246_LARGE_INBOX_TAKE_FIRST
#error "HZ3_S274_LARGE_INBOX_SIZE_DRAIN requires HZ3_S246_LARGE_INBOX_TAKE_FIRST"
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN && HZ3_S245_LARGE_INBOX_BATCH_POP
#error "HZ3_S274_LARGE_INBOX_SIZE_DRAIN uses the S245 batch helper; keep HZ3_S245_LARGE_INBOX_BATCH_POP disabled"
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN && HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH
#error "HZ3_S274_LARGE_INBOX_SIZE_DRAIN and HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH are separate probes"
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN && HZ3_S274_LARGE_INBOX_DRAIN_BATCH < 1
#error "HZ3_S274_LARGE_INBOX_DRAIN_BATCH must be >= 1"
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN && \
    HZ3_S274_LARGE_INBOX_MIN_REQUEST_BYTES > HZ3_S274_LARGE_INBOX_MAX_REQUEST_BYTES
#error "HZ3_S274_LARGE_INBOX_MIN_REQUEST_BYTES must be <= HZ3_S274_LARGE_INBOX_MAX_REQUEST_BYTES"
#endif

// S286: focused take-first refill cap probe.
// Unlike S283, keep the global S240 drain batch at 8 and only cap the
// successful S246 take-first path's extra owner-inbox refill work.
#ifndef HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP
#define HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP 0
#endif

#ifndef HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES
#define HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES 6
#endif

#ifndef HZ3_S286_LARGE_TAKE_FIRST_MIN_REQUEST_BYTES
#define HZ3_S286_LARGE_TAKE_FIRST_MIN_REQUEST_BYTES 0
#endif

#ifndef HZ3_S286_LARGE_TAKE_FIRST_MAX_REQUEST_BYTES
#define HZ3_S286_LARGE_TAKE_FIRST_MAX_REQUEST_BYTES 18446744073709551615ULL
#endif

#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP && !HZ3_S246_LARGE_INBOX_TAKE_FIRST
#error "HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP requires S246 take-first"
#endif

#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP && HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH
#error "HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP and S273 batch take-first are separate probes"
#endif

#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP && HZ3_S274_LARGE_INBOX_SIZE_DRAIN
#error "HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP and S274 size drain are separate probes"
#endif

#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP && \
    HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES < 1
#error "HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES must be >= 1"
#endif

#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP && \
    HZ3_S286_LARGE_TAKE_FIRST_MIN_REQUEST_BYTES > \
        HZ3_S286_LARGE_TAKE_FIRST_MAX_REQUEST_BYTES
#error "HZ3_S286_LARGE_TAKE_FIRST_MIN_REQUEST_BYTES must be <= max request bytes"
#endif

// S242: shifted-pointer-safe large direct side-map prototype.
// Shadow mode keeps the existing active hash map authoritative.  The side-map
// is populated and checked only to measure hit rate and mismatch risk before
// any future mapless fast path.
#ifndef HZ3_S242_LARGE_DIRECT_SIDE_MAP
#define HZ3_S242_LARGE_DIRECT_SIDE_MAP 0
#endif

#ifndef HZ3_S242_DIRECT_SHADOW
#define HZ3_S242_DIRECT_SHADOW HZ3_S242_LARGE_DIRECT_SIDE_MAP
#endif

#ifndef HZ3_S242_DIRECT_MAPLESS
#define HZ3_S242_DIRECT_MAPLESS 0
#endif

#ifndef HZ3_S242_DIRECT_MAPLESS_MAX_CLASS_BYTES
#define HZ3_S242_DIRECT_MAPLESS_MAX_CLASS_BYTES HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES
#endif

#if HZ3_S291_HIGHBAND_OWNER_WINDOW && !HZ3_S242_DIRECT_MAPLESS
#error "HZ3_S291_HIGHBAND_OWNER_WINDOW requires HZ3_S242_DIRECT_MAPLESS"
#endif

#ifndef HZ3_S242_DIRECT_STATS
#define HZ3_S242_DIRECT_STATS HZ3_S242_LARGE_DIRECT_SIDE_MAP
#endif

#ifndef HZ3_S242_DIRECT_ENTRY_COUNT
#define HZ3_S242_DIRECT_ENTRY_COUNT HZ3_S242_DIRECT_STATS
#endif

#ifndef HZ3_S242_DIRECT_VALIDATE
#define HZ3_S242_DIRECT_VALIDATE 0
#endif

#ifndef HZ3_S242_DIRECT_FAILFAST
#define HZ3_S242_DIRECT_FAILFAST HZ3_S242_DIRECT_VALIDATE
#endif

#ifndef HZ3_S242_DIRECT_TAKE_BUSY_RETRIES
#define HZ3_S242_DIRECT_TAKE_BUSY_RETRIES 8u
#endif

#ifndef HZ3_S242_DIRECT_SET_BITS
#define HZ3_S242_DIRECT_SET_BITS 14
#endif

#ifndef HZ3_S242_DIRECT_WAYS
#define HZ3_S242_DIRECT_WAYS 4
#endif

#if HZ3_S242_LARGE_DIRECT_SIDE_MAP && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S242_LARGE_DIRECT_SIDE_MAP requires HZ3_S240_LARGE_OWNER_FRONT metadata"
#endif

#if HZ3_S242_DIRECT_MAPLESS && !HZ3_S242_LARGE_DIRECT_SIDE_MAP
#error "HZ3_S242_DIRECT_MAPLESS requires HZ3_S242_LARGE_DIRECT_SIDE_MAP"
#endif

#if HZ3_S242_LARGE_DIRECT_SIDE_MAP && (HZ3_S242_DIRECT_SET_BITS < 4 || HZ3_S242_DIRECT_SET_BITS > 20)
#error "HZ3_S242_DIRECT_SET_BITS must be between 4 and 20"
#endif

#if HZ3_S242_LARGE_DIRECT_SIDE_MAP && (HZ3_S242_DIRECT_WAYS < 1 || HZ3_S242_DIRECT_WAYS > 16)
#error "HZ3_S242_DIRECT_WAYS must be between 1 and 16"
#endif

#if HZ3_S242_DIRECT_VALIDATE && !HZ3_S242_LARGE_DIRECT_SIDE_MAP
#error "HZ3_S242_DIRECT_VALIDATE requires HZ3_S242_LARGE_DIRECT_SIDE_MAP"
#endif

#if HZ3_S242_DIRECT_VALIDATE && !HZ3_S242_DIRECT_STATS
#error "HZ3_S242_DIRECT_VALIDATE requires HZ3_S242_DIRECT_STATS"
#endif

#if HZ3_S242_DIRECT_VALIDATE < 0 || HZ3_S242_DIRECT_VALIDATE > 1
#error "HZ3_S242_DIRECT_VALIDATE must be 0 or 1"
#endif

#if HZ3_S242_DIRECT_FAILFAST < 0 || HZ3_S242_DIRECT_FAILFAST > 1
#error "HZ3_S242_DIRECT_FAILFAST must be 0 or 1"
#endif

#if HZ3_S242_DIRECT_TAKE_BUSY_RETRIES > 1024u
#error "HZ3_S242_DIRECT_TAKE_BUSY_RETRIES must be <= 1024"
#endif

// S243: residual large fast-path cost probe.
// Observation-only lane used after S240/S242 to split the remaining fixed cost
// into front pop, owner inbox drain/push, activation, and global fallback.
#ifndef HZ3_S243_LARGE_RESIDUAL_OBS
#define HZ3_S243_LARGE_RESIDUAL_OBS 0
#endif

#if HZ3_S243_LARGE_RESIDUAL_OBS && !HZ3_LARGE_ALIGNED_OBS
#error "HZ3_S243_LARGE_RESIDUAL_OBS requires HZ3_LARGE_ALIGNED_OBS"
#endif

// S275: activation/direct-map split counters for the large aligned path.
// Observation-only. This keeps allocator behavior unchanged and only splits the
// cached activation cost before S276R direct-retain experiments.
#ifndef HZ3_S275_LARGE_ACTIVATION_OBS
#define HZ3_S275_LARGE_ACTIVATION_OBS 0
#endif

#if HZ3_S275_LARGE_ACTIVATION_OBS && !HZ3_LARGE_ALIGNED_OBS
#error "HZ3_S275_LARGE_ACTIVATION_OBS requires HZ3_LARGE_ALIGNED_OBS"
#endif

#if HZ3_S275_LARGE_ACTIVATION_OBS && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S275_LARGE_ACTIVATION_OBS requires HZ3_S240_LARGE_OWNER_FRONT"
#endif

// S276R step 1: retain the S242 direct side-map slot while an object sits in
// the same-owner TLS front cache. This avoids direct remove/reinsert on the
// front-cache hot loop. Remote owner inbox retain is a later, separate lane.
#ifndef HZ3_S276_LARGE_DIRECT_RETAIN_FRONT
#define HZ3_S276_LARGE_DIRECT_RETAIN_FRONT 0
#endif

#ifndef HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
#define HZ3_S276_LARGE_DIRECT_RETAIN_INBOX 0
#endif

#ifndef HZ3_S276_LARGE_DIRECT_RETAIN_STATS
#define HZ3_S276_LARGE_DIRECT_RETAIN_STATS \
    (HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX)
#endif

#if (HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX) && \
    !HZ3_S242_DIRECT_MAPLESS
#error "HZ3_S276_LARGE_DIRECT_RETAIN requires HZ3_S242_DIRECT_MAPLESS"
#endif

#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT && !HZ3_S240_LARGE_FRONT_CACHE
#error "HZ3_S276_LARGE_DIRECT_RETAIN_FRONT requires HZ3_S240_LARGE_FRONT_CACHE"
#endif

#if HZ3_S276_LARGE_DIRECT_RETAIN_INBOX && !HZ3_S240_LARGE_OWNER_INBOX
#error "HZ3_S276_LARGE_DIRECT_RETAIN_INBOX requires HZ3_S240_LARGE_OWNER_INBOX"
#endif

#if (HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX) && \
    !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S276_LARGE_DIRECT_RETAIN requires HZ3_S240_LARGE_OWNER_FRONT"
#endif

// S287: speed-only exact-retained activation fast path.
// If a cached direct-retained object is reused with the same request size and
// compatible alignment, skip the broader retained-activation validation. Any
// non-exact case falls back to the S276 full validation path.
#ifndef HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST
#define HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST 0
#endif

#ifndef HZ3_S287_LARGE_RETAINED_ACTIVATE_MIN_REQUEST_BYTES
#define HZ3_S287_LARGE_RETAINED_ACTIVATE_MIN_REQUEST_BYTES 0
#endif

#ifndef HZ3_S287_LARGE_RETAINED_ACTIVATE_MAX_REQUEST_BYTES
#define HZ3_S287_LARGE_RETAINED_ACTIVATE_MAX_REQUEST_BYTES 18446744073709551615ULL
#endif

#if HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST && \
    !(HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX)
#error "HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST requires S276 direct retain"
#endif

#if HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST && HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST is speed-only and requires S240 stats disabled"
#endif

#if HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST && HZ3_S276_LARGE_DIRECT_RETAIN_STATS
#error "HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST is speed-only and requires S276 stats disabled"
#endif

#if HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST && HZ3_S285_LARGE_FIXED_COST_OBS
#error "HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST is separate from S285 observation"
#endif

#if HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST && \
    HZ3_S287_LARGE_RETAINED_ACTIVATE_MIN_REQUEST_BYTES > \
        HZ3_S287_LARGE_RETAINED_ACTIVATE_MAX_REQUEST_BYTES
#error "HZ3_S287_LARGE_RETAINED_ACTIVATE_MIN_REQUEST_BYTES must be <= max request bytes"
#endif

// S300: route-only shadow observation for an over-aligned medium/run path.
// This does not change allocation behavior. It only records how much of the
// current large aligned route would be eligible for a future medium-run route.
#ifndef HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS
#define HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS 0
#endif

#if HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS && !HZ3_LARGE_ALIGNED_OBS
#error "HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS requires HZ3_LARGE_ALIGNED_OBS"
#endif

// S300: behavior-changing over-aligned medium/run prototype.
// Default off. When enabled, this may extend medium bin namespace/layout for
// aligned-medium runs, so keep it in dedicated research lanes until validated.
#ifndef HZ3_S300_OVERALIGNED_MEDIUM_RUNS
#define HZ3_S300_OVERALIGNED_MEDIUM_RUNS 0
#endif

#ifndef HZ3_S300_OVERALIGNED_MEDIUM_MAX_SIZE
#define HZ3_S300_OVERALIGNED_MEDIUM_MAX_SIZE 65536
#endif

#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS && \
    (HZ3_S300_OVERALIGNED_MEDIUM_MAX_SIZE < 4096 || \
     HZ3_S300_OVERALIGNED_MEDIUM_MAX_SIZE > 65536)
#error "HZ3_S300_OVERALIGNED_MEDIUM_MAX_SIZE must stay within medium size classes"
#endif

// S279: speed-only retained-direct free fast path.
// S276 direct-retain already validates the direct slot and transitions ACTIVE
// to FREEING in hz3_large_take(). In counter-free/safety-free speed lanes, the
// following S240 free-take metadata check is redundant for retained objects.
#ifndef HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META
#define HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META 0
#endif

#if HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META && \
    !(HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX)
#error "HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META requires S276 direct retain"
#endif

#if HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META && HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META is speed-only and requires S240 stats disabled"
#endif

#if HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META && HZ3_S251_LARGE_OWNER_FRONT_SAFETY
#error "HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META is speed-only and requires S251 safety disabled"
#endif

// S280: retained-direct free-route observation.
// Observation-only.  S276 made direct-retained large objects move through
// FRONT/REMOTE cached states, but the broad free_admit timer is too coarse to
// tell which route still costs us.  This splits retained free routing into
// same-owner front, remote owner inbox, and global/munmap fallback counters.
#ifndef HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS
#define HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS 0
#endif

#if HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS && !HZ3_LARGE_ALIGNED_OBS
#error "HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS requires HZ3_LARGE_ALIGNED_OBS"
#endif

#if HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS && \
    !(HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX)
#error "HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS requires S276 direct retain"
#endif

#if HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS requires S240 owner-front"
#endif

// S281: retained-direct free route specialization.
// Speed-only. S276 direct-retain already validates the direct slot and moves a
// retained large object from ACTIVE to FREEING inside hz3_large_take(). S281
// lets those retained frees try same-owner front or remote owner-inbox routing
// before the broad diagnostic/free metadata path.
#ifndef HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST
#define HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST 0
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && \
    !(HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX)
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST requires S276 direct retain"
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST requires S240 owner-front"
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST is speed-only and requires S240 stats disabled"
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && HZ3_S276_LARGE_DIRECT_RETAIN_STATS
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST is speed-only and requires S276 stats disabled"
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && HZ3_LARGE_CACHE_STATS
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST is speed-only and requires large cache stats disabled"
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && HZ3_LARGE_ALIGNED_OBS
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST is speed-only and requires aligned observation disabled"
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST && HZ3_S251_LARGE_OWNER_FRONT_SAFETY
#error "HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST is speed-only and requires S251 safety disabled"
#endif

// S282: owner-inbox preflight observation for S278X.
// Diagnostic-only. Adds owner-inbox CAS/depth/hit-shape counters on top of the
// current S276 front+inbox direct-retain speed baseline without changing queue
// behavior.
#ifndef HZ3_S282_LARGE_OWNER_INBOX_OBS
#define HZ3_S282_LARGE_OWNER_INBOX_OBS 0
#endif

#if HZ3_S282_LARGE_OWNER_INBOX_OBS && !HZ3_S246_LARGE_INBOX_TAKE_FIRST
#error "HZ3_S282_LARGE_OWNER_INBOX_OBS requires S246 take-first"
#endif

#if HZ3_S282_LARGE_OWNER_INBOX_OBS && !HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
#error "HZ3_S282_LARGE_OWNER_INBOX_OBS requires S276 front+inbox direct retain"
#endif

#if HZ3_S282_LARGE_OWNER_INBOX_OBS && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S282_LARGE_OWNER_INBOX_OBS requires S240 owner-front stats"
#endif

#if HZ3_S282_LARGE_OWNER_INBOX_OBS && !HZ3_S276_LARGE_DIRECT_RETAIN_STATS
#error "HZ3_S282_LARGE_OWNER_INBOX_OBS requires S276 direct-retain stats"
#endif

// S285: S276 fixed-cost observation.
// Diagnostic-only. Splits the remaining S276 direct-retain cost into direct
// slot scan/state reasons, retained activation rejection reasons, and
// take-first drain/refill shape. This is a counter lane, not a behavior change.
#ifndef HZ3_S285_LARGE_FIXED_COST_OBS
#define HZ3_S285_LARGE_FIXED_COST_OBS 0
#endif

#if HZ3_S285_LARGE_FIXED_COST_OBS && !HZ3_LARGE_ALIGNED_OBS
#error "HZ3_S285_LARGE_FIXED_COST_OBS requires HZ3_LARGE_ALIGNED_OBS"
#endif

#if HZ3_S285_LARGE_FIXED_COST_OBS && !HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
#error "HZ3_S285_LARGE_FIXED_COST_OBS requires S276 front+inbox direct retain"
#endif

#if HZ3_S285_LARGE_FIXED_COST_OBS && !HZ3_S246_LARGE_INBOX_TAKE_FIRST
#error "HZ3_S285_LARGE_FIXED_COST_OBS requires S246 take-first"
#endif

#if HZ3_S285_LARGE_FIXED_COST_OBS && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S285_LARGE_FIXED_COST_OBS requires S240 owner-front stats"
#endif

// S251: large owner-front container safety probe.
// Observation/debug lane for S250 high-band work.  It validates the durable
// container state transitions around ACTIVE/FREEING/FRONT/REMOTE/GLOBAL.
#ifndef HZ3_S251_LARGE_OWNER_FRONT_SAFETY
#define HZ3_S251_LARGE_OWNER_FRONT_SAFETY 0
#endif

#ifndef HZ3_S251_LARGE_OWNER_FRONT_FAILFAST
#define HZ3_S251_LARGE_OWNER_FRONT_FAILFAST 0
#endif

#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S251_LARGE_OWNER_FRONT_SAFETY requires HZ3_S240_LARGE_OWNER_FRONT"
#endif

#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S251_LARGE_OWNER_FRONT_SAFETY requires HZ3_S240_LARGE_OWNER_FRONT_STATS"
#endif

// S240 flush/debug validation.
//
// Diagnostic-only guard for rare teardown/flush corruption. It validates large
// owner-front and owner-inbox nodes immediately before S240-controlled munmap
// paths, and checks inbox accounting before subtracting pop counts/bytes.
#ifndef HZ3_S240_LARGE_FLUSH_VALIDATE
#define HZ3_S240_LARGE_FLUSH_VALIDATE 0
#endif

#ifndef HZ3_S240_LARGE_FLUSH_FAILFAST
#define HZ3_S240_LARGE_FLUSH_FAILFAST HZ3_S240_LARGE_FLUSH_VALIDATE
#endif

// Debug-only quarantine. When enabled, S240 flush/drain fallback paths mark
// nodes as retired but intentionally skip munmap. This leaks memory by design
// and is only for isolating invalid/double munmap crashes.
#ifndef HZ3_S240_LARGE_FLUSH_QUARANTINE
#define HZ3_S240_LARGE_FLUSH_QUARANTINE 0
#endif

#if HZ3_S240_LARGE_FLUSH_VALIDATE && !HZ3_S240_LARGE_OWNER_FRONT
#error "HZ3_S240_LARGE_FLUSH_VALIDATE requires HZ3_S240_LARGE_OWNER_FRONT"
#endif

#if HZ3_S240_LARGE_FLUSH_VALIDATE && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S240_LARGE_FLUSH_VALIDATE requires HZ3_S240_LARGE_OWNER_FRONT_STATS"
#endif

#if HZ3_S240_LARGE_FLUSH_VALIDATE && !HZ3_S240_LARGE_FRONT_CACHE
#error "HZ3_S240_LARGE_FLUSH_VALIDATE requires HZ3_S240_LARGE_FRONT_CACHE"
#endif

#if HZ3_S240_LARGE_FLUSH_QUARANTINE && !HZ3_S240_LARGE_FRONT_CACHE
#error "HZ3_S240_LARGE_FLUSH_QUARANTINE requires HZ3_S240_LARGE_FRONT_CACHE"
#endif

#if HZ3_S240_LARGE_FLUSH_QUARANTINE && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S240_LARGE_FLUSH_QUARANTINE requires HZ3_S240_LARGE_OWNER_FRONT_STATS"
#endif

#if HZ3_S240_LARGE_FLUSH_VALIDATE < 0 || HZ3_S240_LARGE_FLUSH_VALIDATE > 1
#error "HZ3_S240_LARGE_FLUSH_VALIDATE must be 0 or 1"
#endif

#if HZ3_S240_LARGE_FLUSH_FAILFAST < 0 || HZ3_S240_LARGE_FLUSH_FAILFAST > 1
#error "HZ3_S240_LARGE_FLUSH_FAILFAST must be 0 or 1"
#endif

#if HZ3_S240_LARGE_FLUSH_QUARANTINE < 0 || HZ3_S240_LARGE_FLUSH_QUARANTINE > 1
#error "HZ3_S240_LARGE_FLUSH_QUARANTINE must be 0 or 1"
#endif

// S253: RSS retention observation lane for S252 high-band owner-front policy.
//
// This is diagnostic-only. It mirrors the S252 speed lane but emits retained
// byte inventory for front cache, owner inbox, global large scache, and caps.
#ifndef HZ3_S253_LARGE_RSS_RETENTION_OBS
#define HZ3_S253_LARGE_RSS_RETENTION_OBS 0
#endif

#if HZ3_S253_LARGE_RSS_RETENTION_OBS && !HZ3_S240_LARGE_OWNER_FRONT_STATS
#error "HZ3_S253_LARGE_RSS_RETENTION_OBS requires HZ3_S240_LARGE_OWNER_FRONT_STATS"
#endif

#if HZ3_S253_LARGE_RSS_RETENTION_OBS && !HZ3_S240_LARGE_FRONT_CACHE
#error "HZ3_S253_LARGE_RSS_RETENTION_OBS requires HZ3_S240_LARGE_FRONT_CACHE"
#endif

#if HZ3_S253_LARGE_RSS_RETENTION_OBS && !HZ3_S240_LARGE_OWNER_INBOX
#error "HZ3_S253_LARGE_RSS_RETENTION_OBS requires HZ3_S240_LARGE_OWNER_INBOX"
#endif

// S267: large RSS plateau observation lane.
//
// Diagnostic-only. It keeps allocator behavior unchanged and emits per-class
// large scache peaks plus deferred-unmap inventory so RSS plateau rows can be
// explained without enabling S240 owner-front machinery.
#ifndef HZ3_S267_LARGE_RSS_PLATEAU_OBS
#define HZ3_S267_LARGE_RSS_PLATEAU_OBS 0
#endif

// S185: batch munmap on large-cache eviction path.
// 0=legacy per-victim unlock/munmap/relock, 1=batch victims then munmap outside lock.
#ifndef HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
#define HZ3_S185_LARGE_EVICT_MUNMAP_BATCH 0
#endif

#ifndef HZ3_S185_EVICT_BATCH_MAX
#define HZ3_S185_EVICT_BATCH_MAX 8
#endif

#if HZ3_S185_EVICT_BATCH_MAX < 1
#error "HZ3_S185_EVICT_BATCH_MAX must be >= 1"
#endif

// S186: defer munmap from eviction path and drain by budget at free boundary.
// 0=off, 1=on.
#ifndef HZ3_S186_LARGE_UNMAP_DEFER
#define HZ3_S186_LARGE_UNMAP_DEFER 0
#endif

#ifndef HZ3_S186_UNMAP_DEFER_BUDGET
#define HZ3_S186_UNMAP_DEFER_BUDGET 4
#endif

#ifndef HZ3_S186_UNMAP_DEFER_MAX_BYTES
#define HZ3_S186_UNMAP_DEFER_MAX_BYTES (64ULL << 20)
#endif

#if HZ3_S186_UNMAP_DEFER_BUDGET < 1
#error "HZ3_S186_UNMAP_DEFER_BUDGET must be >= 1"
#endif

// S191: keep cache lock held on large_free fast path when madvise is not needed.
// 0=legacy unlock/relock sequence, 1=direct insert under first lock.
#ifndef HZ3_S191_LARGE_FREE_FAST_INSERT
#define HZ3_S191_LARGE_FREE_FAST_INSERT 0
#endif

// S192: soft purge advice mode for large-cache madvise path.
// 0=legacy behavior, 1=MADV_FREE, 2=MADV_DONTNEED.
#ifndef HZ3_S192_SOFT_PURGE_MODE
#define HZ3_S192_SOFT_PURGE_MODE 0
#endif

#if HZ3_S192_SOFT_PURGE_MODE < 0 || HZ3_S192_SOFT_PURGE_MODE > 2
#error "HZ3_S192_SOFT_PURGE_MODE must be 0..2"
#endif

// S193: demand-driven large scavenge on mmap miss.
// 0=off (legacy free-side purge), 1=on (purge only on miss, budgeted).
#ifndef HZ3_S193_DEMAND_SCAVENGE
#define HZ3_S193_DEMAND_SCAVENGE 1
#endif

// Run demand scavenge once per N mmap misses (>=1).
#ifndef HZ3_S193_SCAVENGE_MISS_STRIDE
#define HZ3_S193_SCAVENGE_MISS_STRIDE 16
#endif

// Maximum pages to purge in one demand-scavenge run.
#ifndef HZ3_S193_SCAVENGE_BUDGET_PAGES
#define HZ3_S193_SCAVENGE_BUDGET_PAGES 16
#endif

#if HZ3_S193_SCAVENGE_MISS_STRIDE < 1
#error "HZ3_S193_SCAVENGE_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S193_SCAVENGE_BUDGET_PAGES < 1
#error "HZ3_S193_SCAVENGE_BUDGET_PAGES must be >= 1"
#endif

// S212: LargeUnmapDeferPlusBox
// Extend deferred-unmap usage from eviction-only to free-path fallback and
// add bounded demand-drain on mmap miss to avoid long defer backlog.
#ifndef HZ3_S212_LARGE_UNMAP_DEFER_PLUS
#define HZ3_S212_LARGE_UNMAP_DEFER_PLUS 1
#endif

// S212 deferred queue upper bound (bytes).
#ifndef HZ3_S212_UNMAP_DEFER_MAX_BYTES
#define HZ3_S212_UNMAP_DEFER_MAX_BYTES (64ULL << 20)
#endif

// S212 free-boundary drain budget.
#ifndef HZ3_S212_UNMAP_DEFER_FREE_BUDGET
#define HZ3_S212_UNMAP_DEFER_FREE_BUDGET 1
#endif

// If enabled, `do_munmap` path first tries to enqueue to S186 defer queue.
#ifndef HZ3_S212_DEFER_MUNMAP_ON_FREE_PATH
#define HZ3_S212_DEFER_MUNMAP_ON_FREE_PATH 1
#endif

// If enabled, alloc-miss path can drain part of deferred queue by bounded budget.
#ifndef HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS
#define HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS 1
#endif

// Run miss-triggered drain once per N alloc-cache misses.
#ifndef HZ3_S212_DEFER_DRAIN_MISS_STRIDE
#define HZ3_S212_DEFER_DRAIN_MISS_STRIDE 16
#endif

// Number of deferred victims to munmap in one miss-triggered drain.
#ifndef HZ3_S212_DEFER_DRAIN_MISS_BUDGET
#define HZ3_S212_DEFER_DRAIN_MISS_BUDGET 1
#endif

// Trigger alloc-miss drain only if deferred queue bytes exceed this threshold.
#ifndef HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES
#define HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES 33554432ULL
#endif

#if HZ3_S212_UNMAP_DEFER_FREE_BUDGET < 1
#error "HZ3_S212_UNMAP_DEFER_FREE_BUDGET must be >= 1"
#endif

#if HZ3_S212_UNMAP_DEFER_MAX_BYTES < (1ULL << 20)
#error "HZ3_S212_UNMAP_DEFER_MAX_BYTES must be >= 1MiB"
#endif

#if HZ3_S212_DEFER_DRAIN_MISS_STRIDE < 1
#error "HZ3_S212_DEFER_DRAIN_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S212_DEFER_DRAIN_MISS_BUDGET < 1
#error "HZ3_S212_DEFER_DRAIN_MISS_BUDGET must be >= 1"
#endif
