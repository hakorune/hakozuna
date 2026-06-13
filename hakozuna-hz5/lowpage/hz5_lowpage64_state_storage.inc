static _Atomic(uintptr_t) g_hz5_lowpage64_global_batch_head;
static _Atomic size_t g_hz5_lowpage64_global_batch_count;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_min;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_max;
#if HZ5_LOWPAGE64_STAGE1_ENABLED
/*
 * Shared BRIDGE_COLD/stage1 queue for P43p/P45 evidence lanes. The storage
 * names retain the P43p prefix for compatibility with existing diagnostics,
 * but new code should treat this as HZ5 bridge-cold state.
 *
 * The canonical speed layer is still release_common() -> relbuf -> global
 * batch -> acquired stash; this queue only holds P40 source-demotion intents
 * that the current admission experiment has decided to keep bridge-side.
 */
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static _Atomic(uintptr_t) g_hz5_lowpage64_p43p_stage1_head;
#endif
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_count;
#endif
#if (HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD) || \
    HZ5_LOWPAGE64_STATS
_Atomic size_t g_hz5_lowpage64_p42_cold_count;
#endif
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static _Atomic uint32_t g_hz5_lowpage64_p40_global_epoch;
#endif
#if HZ5_LOWPAGE64_STATS || HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_ADMISSION_GATE || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p43o_runtime_state;
static _Atomic size_t g_hz5_lowpage64_p43o_runtime_cooldown;
#endif
#if HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_p45_runtime_state;
static _Atomic size_t g_hz5_lowpage64_p45_runtime_cooldown;
#endif
#if HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD
typedef struct Hz5Lowpage64ColdNode {
  void* raw;
  size_t bytes;
  struct Hz5Lowpage64ColdNode* next;
} Hz5Lowpage64ColdNode;

_Atomic(uintptr_t) g_hz5_lowpage64_p42_cold_head;
#endif

#include "hz5_lowpage64_stats_storage.inc"

#if HZ5_LOWPAGE64_SPAN_CACHE
__declspec(thread) static void* g_hz5_lowpage64_span_head;
__declspec(thread) static size_t g_hz5_lowpage64_span_count;
#endif
HZ5_TLS_STATIC void* g_hz5_lowpage64_stash_head;
HZ5_TLS_STATIC size_t g_hz5_lowpage64_stash_count;
#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
__declspec(thread) static void* g_hz5_lowpage64_overflow_head;
__declspec(thread) static size_t g_hz5_lowpage64_overflow_count;
__declspec(thread) static uint32_t g_hz5_lowpage64_epoch;
__declspec(thread) static uint32_t g_hz5_lowpage64_overflow_epoch;
__declspec(thread) static uint8_t g_hz5_lowpage64_high_event_streak;
#endif
#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
__declspec(thread) static void* g_hz5_lowpage64_probation_head;
__declspec(thread) static size_t g_hz5_lowpage64_probation_count;
__declspec(thread) static uint32_t g_hz5_lowpage64_p39_epoch;
__declspec(thread) static uint32_t g_hz5_lowpage64_probation_epoch;
#endif
HZ5_TLS_STATIC void* g_hz5_lowpage64_relbuf_head;
HZ5_TLS_STATIC void* g_hz5_lowpage64_relbuf_tail;
HZ5_TLS_STATIC size_t g_hz5_lowpage64_relbuf_count;
