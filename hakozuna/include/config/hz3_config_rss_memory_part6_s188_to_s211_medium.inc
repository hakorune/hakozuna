// S188: medium alloc-slow tuning (sc=5..7) for 4KB-32KB Larson triage.
// Default OFF. Enable only for A/B validation.
#ifndef HZ3_S188_MEDIUM_REFILL_TUNE
#define HZ3_S188_MEDIUM_REFILL_TUNE 0
#endif

#ifndef HZ3_S188_REFILL_SC5
#define HZ3_S188_REFILL_SC5 4
#endif
#ifndef HZ3_S188_REFILL_SC6
#define HZ3_S188_REFILL_SC6 4
#endif
#ifndef HZ3_S188_REFILL_SC7
#define HZ3_S188_REFILL_SC7 4
#endif

#ifndef HZ3_S188_BIN_TARGET_SC5
#define HZ3_S188_BIN_TARGET_SC5 24
#endif
#ifndef HZ3_S188_BIN_TARGET_SC6
#define HZ3_S188_BIN_TARGET_SC6 24
#endif
#ifndef HZ3_S188_BIN_TARGET_SC7
#define HZ3_S188_BIN_TARGET_SC7 24
#endif

// S189: MediumTransferCacheBox (sc5..7)
// Shared per-shard/sc cache between central and segment-refill path.
#ifndef HZ3_S189_MEDIUM_TRANSFERCACHE
#define HZ3_S189_MEDIUM_TRANSFERCACHE 0
#endif

#ifndef HZ3_S189_SC_MIN
#define HZ3_S189_SC_MIN 5
#endif
#ifndef HZ3_S189_SC_MAX
#define HZ3_S189_SC_MAX 7
#endif

// Max cached objects per (shard,sc) in S189 transfer cache.
#ifndef HZ3_S189_MAX_OBJS_PER_BIN
#define HZ3_S189_MAX_OBJS_PER_BIN 256
#endif

#if HZ3_S189_SC_MIN > HZ3_S189_SC_MAX
#error "HZ3_S189_SC_MIN must be <= HZ3_S189_SC_MAX"
#endif

#if HZ3_S189_MAX_OBJS_PER_BIN < 1
#error "HZ3_S189_MAX_OBJS_PER_BIN must be >= 1"
#endif

// S190: MissTriggeredRemoteFlushBox
// On central miss, force an extra remote flush budget and retry inbox/central once.
#ifndef HZ3_S190_MISS_TRIGGERED_REMOTE_FLUSH
#define HZ3_S190_MISS_TRIGGERED_REMOTE_FLUSH 1
#endif

// Budget bins used by the miss-triggered retry flush.
#ifndef HZ3_S190_FLUSH_BUDGET_BINS
#define HZ3_S190_FLUSH_BUDGET_BINS 32
#endif

#if HZ3_S190_FLUSH_BUDGET_BINS < 1
#error "HZ3_S190_FLUSH_BUDGET_BINS must be >= 1"
#endif

// S190-v3: targeted remote flush by requested size-class (opt-in).
// 0=legacy budget scan, 1=prefer target sc/bin when forcing miss flush.
#ifndef HZ3_S190_TARGETED_FLUSH
#define HZ3_S190_TARGETED_FLUSH 0
#endif

// S190-v2: optionally disable central retry after forced flush.
// 0=inbox-only retry, 1=inbox+central retry (legacy).
#ifndef HZ3_S190_RETRY_CENTRAL
#define HZ3_S190_RETRY_CENTRAL 1
#endif

// S190-v2: optionally require demand credit before firing forced flush.
// 0=hint-only gate (legacy), 1=hint+credit gate.
#ifndef HZ3_S190_REQUIRE_CREDIT
#define HZ3_S190_REQUIRE_CREDIT 1
#endif

#ifndef HZ3_S190_MIN_CREDIT
#define HZ3_S190_MIN_CREDIT 8
#endif

#ifndef HZ3_S190_CREDIT_CONSUME
#define HZ3_S190_CREDIT_CONSUME HZ3_S190_FLUSH_BUDGET_BINS
#endif

#if HZ3_S190_MIN_CREDIT < 1
#error "HZ3_S190_MIN_CREDIT must be >= 1"
#endif

#if HZ3_S190_CREDIT_CONSUME < 1
#error "HZ3_S190_CREDIT_CONSUME must be >= 1"
#endif

// S190-v2: size-class scope gate for forced flush path.
// Default keeps legacy behavior for all medium slow-path classes.
#ifndef HZ3_S190_SC_MIN
#define HZ3_S190_SC_MIN 5
#endif

#ifndef HZ3_S190_SC_MAX
#define HZ3_S190_SC_MAX 7
#endif

#if HZ3_S190_SC_MIN > HZ3_S190_SC_MAX
#error "HZ3_S190_SC_MIN must be <= HZ3_S190_SC_MAX"
#endif

// S208: MediumCentralFloorReserveBox (opt-in)
// On central miss, segment fallback reserves a bounded number of extra runs
// into central until per-(shard,sc) central floor is reached.
#ifndef HZ3_S208_MEDIUM_CENTRAL_RESERVE
#define HZ3_S208_MEDIUM_CENTRAL_RESERVE 0
#endif

#ifndef HZ3_S208_SC_MIN
#define HZ3_S208_SC_MIN 5
#endif

#ifndef HZ3_S208_SC_MAX
#define HZ3_S208_SC_MAX 7
#endif

#ifndef HZ3_S208_CENTRAL_FLOOR_OBJS
#define HZ3_S208_CENTRAL_FLOOR_OBJS 32
#endif

#ifndef HZ3_S208_RESERVE_MAX_OBJS
#define HZ3_S208_RESERVE_MAX_OBJS 4
#endif

#ifndef HZ3_S208_MISS_STRIDE
#define HZ3_S208_MISS_STRIDE 1
#endif

#ifndef HZ3_S208_MISS_STREAK_MIN
#define HZ3_S208_MISS_STREAK_MIN 2
#endif

#if HZ3_S208_SC_MIN > HZ3_S208_SC_MAX
#error "HZ3_S208_SC_MIN must be <= HZ3_S208_SC_MAX"
#endif

#if HZ3_S208_CENTRAL_FLOOR_OBJS < 1
#error "HZ3_S208_CENTRAL_FLOOR_OBJS must be >= 1"
#endif

#if HZ3_S208_RESERVE_MAX_OBJS < 1
#error "HZ3_S208_RESERVE_MAX_OBJS must be >= 1"
#endif

#if HZ3_S208_MISS_STRIDE < 1
#error "HZ3_S208_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S208_MISS_STREAK_MIN < 1
#error "HZ3_S208_MISS_STREAK_MIN must be >= 1"
#endif

// S211: MediumSegmentReserveLiteBox (opt-in)
// On sparse central-miss boundaries, reserve a tiny number of extra segment
// runs to central only after keeping enough runs locally.
#ifndef HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
#define HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE 0
#endif

#ifndef HZ3_S211_SC_MIN
#define HZ3_S211_SC_MIN 5
#endif

#ifndef HZ3_S211_SC_MAX
#define HZ3_S211_SC_MAX 7
#endif

#ifndef HZ3_S211_MISS_STRIDE
#define HZ3_S211_MISS_STRIDE 8
#endif

#ifndef HZ3_S211_RESERVE_MAX_OBJS
#define HZ3_S211_RESERVE_MAX_OBJS 2
#endif

#ifndef HZ3_S211_LOCAL_KEEP_MIN
#define HZ3_S211_LOCAL_KEEP_MIN 6
#endif

#if HZ3_S211_SC_MIN > HZ3_S211_SC_MAX
#error "HZ3_S211_SC_MIN must be <= HZ3_S211_SC_MAX"
#endif

#if HZ3_S211_MISS_STRIDE < 1
#error "HZ3_S211_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S211_RESERVE_MAX_OBJS < 1
#error "HZ3_S211_RESERVE_MAX_OBJS must be >= 1"
#endif

#if HZ3_S211_LOCAL_KEEP_MIN < 0
#error "HZ3_S211_LOCAL_KEEP_MIN must be >= 0"
#endif

#if HZ3_S208_MEDIUM_CENTRAL_RESERVE && HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
#error "HZ3_S208_MEDIUM_CENTRAL_RESERVE and HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE are mutually exclusive"
#endif
