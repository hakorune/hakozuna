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

