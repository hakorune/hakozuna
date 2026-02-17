#ifndef HZ3_S44_OWNER_STASH
#define HZ3_S44_OWNER_STASH 0
#endif

// Push/pop can be controlled separately for A/B
#ifndef HZ3_S44_OWNER_STASH_PUSH
#define HZ3_S44_OWNER_STASH_PUSH (HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE)
#endif

#ifndef HZ3_S44_OWNER_STASH_POP
#define HZ3_S44_OWNER_STASH_POP (HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE)
#endif

// Drain limit
#ifndef HZ3_S44_DRAIN_LIMIT
#define HZ3_S44_DRAIN_LIMIT 32
#endif

// Max objects per bin before overflow
#ifndef HZ3_S44_STASH_MAX_OBJS
#define HZ3_S44_STASH_MAX_OBJS 256
#endif

// S44-3: Owner Stash optimization flags
#ifndef HZ3_S44_OWNER_STASH_COUNT
#define HZ3_S44_OWNER_STASH_COUNT 0
#endif

#ifndef HZ3_S44_OWNER_STASH_FASTPOP
#define HZ3_S44_OWNER_STASH_FASTPOP 1
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif

// Compile-time guard: FASTPOP requires COUNT=0
#if HZ3_S44_OWNER_STASH_FASTPOP && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S44_OWNER_STASH_FASTPOP=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// S44-BD: Bounded drain (CAS partial detach instead of atomic_exchange full drain)
// Reduces cache pollution in r90-like constant-push environments
#ifndef HZ3_S44_BOUNDED_DRAIN
#define HZ3_S44_BOUNDED_DRAIN 0  // default OFF for A/B
#endif

// Fixed slack to add to need (max_take = need + BOUNDED_EXTRA, clamped to need + 64)
#ifndef HZ3_S44_BOUNDED_EXTRA
#define HZ3_S44_BOUNDED_EXTRA 32
#endif

// S44-PF: Prefetch next pointer in pop_batch walk loop
#ifndef HZ3_S44_PREFETCH
#define HZ3_S44_PREFETCH 1  // default ON (GO in perf A/B)
#endif

// S44-PF distance: how many nodes ahead to prefetch
// 1 = prefetch next only, 2 = also prefetch next->next (A/B required)
// GO (S153): DIST=2 +5.0% on xmalloc-test, 10/10 wins (2026-01-19)
#ifndef HZ3_S44_PREFETCH_DIST
#define HZ3_S44_PREFETCH_DIST 2  // was 1
#endif

// S179: Gate dist=2 prefetch by remaining need in owner stash pop walk.
// Keeps dist=1 prefetch always ON while reducing next->next prefetch when
// spill already satisfied most of `want`.
#ifndef HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE
#define HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE 0
#endif

#ifndef HZ3_S179_OWNER_STASH_PREFETCH_DIST2_MIN_NEED
#define HZ3_S179_OWNER_STASH_PREFETCH_DIST2_MIN_NEED 8
#endif

#if HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE && (HZ3_S179_OWNER_STASH_PREFETCH_DIST2_MIN_NEED < 1)
#error "HZ3_S179_OWNER_STASH_PREFETCH_DIST2_MIN_NEED must be >= 1 when HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE=1"
#endif

// S44-4: MicroOptBox candidates for hz3_owner_stash_pop_batch() (compile-time only)
// Default OFF: require A/B (must not regress r50).
#ifndef HZ3_S44_4_STATS
#define HZ3_S44_4_STATS 0
#endif
#ifndef HZ3_S44_4_WALK_NOPREV
#define HZ3_S44_4_WALK_NOPREV 0
#endif

// S163: For S112 full-drain, skip filling spill_array and keep all leftovers in spill_overflow.
// Reduces per-pop instruction count; A/B required (may affect r50).
#ifndef HZ3_S163_S112_SKIP_SPILL_ARRAY
#define HZ3_S163_S112_SKIP_SPILL_ARRAY 0
#endif

// S180: Owner stash S112 combo gate.
// Enable the paired setting only when both knobs are on:
// - S169 bounded drain under S112
// - S164 skip quick-empty check when spill already returned objects
//
// NOTE:
// - S169 alone and S164 alone both showed R90 long-run regressions in A/B.
// - The pair is promoted as a single default gate; keep explicit -D overrides available.
#ifndef HZ3_S180_S112_BOUNDED_COMBO
#define HZ3_S180_S112_BOUNDED_COMBO 1
#endif

// S169: Re-evaluate bounded drain under S112 (limit detach length).
// A/B required; keep OFF by default.
#ifndef HZ3_S169_S112_BOUNDED_DRAIN
  #if HZ3_S180_S112_BOUNDED_COMBO
    #define HZ3_S169_S112_BOUNDED_DRAIN 1
  #else
    #define HZ3_S169_S112_BOUNDED_DRAIN 0
  #endif
#endif

// S170: S112 bounded drain observation stats (atexit one-shot).
// Requires S169 to be meaningful. Default OFF (research only).
#ifndef HZ3_S170_S112_BOUNDED_STATS
#define HZ3_S170_S112_BOUNDED_STATS 0
#endif

// S165: When S163 is enabled, cap spill_overflow length to HZ3_S67_SPILL_CAP
// and push the remainder back to stash. A/B required.
#ifndef HZ3_S165_S112_OVERFLOW_CAP
#define HZ3_S165_S112_OVERFLOW_CAP 0
#endif
