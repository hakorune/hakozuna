// S55-2: RetentionPolicyBox FROZEN mode (Admission Control + Open Debt)
// ============================================================================

#ifndef HZ3_S55_RETENTION_FROZEN
#define HZ3_S55_RETENTION_FROZEN 0
#endif

// S55-2: Watermark tuning
#ifndef HZ3_S55_WM_SOFT_BYTES
#define HZ3_S55_WM_SOFT_BYTES (HZ3_ARENA_SIZE / 4)
#endif
#ifndef HZ3_S55_WM_HARD_BYTES
#define HZ3_S55_WM_HARD_BYTES (HZ3_ARENA_SIZE * 35 / 100)
#endif
#ifndef HZ3_S55_WM_HYST_BYTES
#define HZ3_S55_WM_HYST_BYTES (HZ3_ARENA_SIZE / 50)
#endif

// S55-2: Debt/tries tuning
#ifndef HZ3_S55_DEBT_LIMIT_L1
#define HZ3_S55_DEBT_LIMIT_L1 8
#endif
#ifndef HZ3_S55_DEBT_LIMIT_L2
#define HZ3_S55_DEBT_LIMIT_L2 2
#endif
#ifndef HZ3_S55_PACK_TRIES_L1
#define HZ3_S55_PACK_TRIES_L1 16
#endif
#ifndef HZ3_S55_PACK_TRIES_L2
#define HZ3_S55_PACK_TRIES_L2 64
#endif

// ============================================================================
// S55-2B: ReturnPolicyBox - Epoch Repay
// ============================================================================

#ifndef HZ3_S55_REPAY_EPOCH_INTERVAL
#define HZ3_S55_REPAY_EPOCH_INTERVAL 16
#endif

#ifndef HZ3_S55_REPAY_SCAN_SLOTS_BUDGET
#define HZ3_S55_REPAY_SCAN_SLOTS_BUDGET 256
#endif

// ============================================================================
// S55-3: MediumRunSubreleaseBox
// ============================================================================

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE
#define HZ3_S55_3_MEDIUM_SUBRELEASE 0
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES
#define HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES 256
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST
#define HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST 0
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT
#define HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT 1
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS
#define HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS 32
#endif

#ifndef HZ3_S55_3_REQUIRE_GEN_DELTA0
#define HZ3_S55_3_REQUIRE_GEN_DELTA0 1
#endif

// ============================================================================
