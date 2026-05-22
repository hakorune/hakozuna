#include "hz5_lowpage64.h"
#include "hz5_lowpage64_p43_segment.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef HZ5_LOWPAGE64_RAW_PAGE_SIZE
#define HZ5_LOWPAGE64_RAW_PAGE_SIZE ((size_t)65536)
#endif

#ifndef HZ5_LOWPAGE64_BATCH_FLUSH_N
#ifdef BENCHLAB_HZ5_P25_BATCH_FLUSH_N
#define HZ5_LOWPAGE64_BATCH_FLUSH_N BENCHLAB_HZ5_P25_BATCH_FLUSH_N
#else
#define HZ5_LOWPAGE64_BATCH_FLUSH_N 8u
#endif
#endif

#ifndef HZ5_LOWPAGE64_GLOBAL_CAP
#ifdef BENCHLAB_HZ5_P25_GLOBAL_CAP
#define HZ5_LOWPAGE64_GLOBAL_CAP BENCHLAB_HZ5_P25_GLOBAL_CAP
#else
#define HZ5_LOWPAGE64_GLOBAL_CAP 256u
#endif
#endif

#ifndef HZ5_LOWPAGE64_SPAN_CAP
#ifdef BENCHLAB_HZ5_P25_SPAN_CAP
#define HZ5_LOWPAGE64_SPAN_CAP BENCHLAB_HZ5_P25_SPAN_CAP
#else
#define HZ5_LOWPAGE64_SPAN_CAP 32u
#endif
#endif

#ifndef HZ5_LOWPAGE64_STASH_CAP
#ifdef BENCHLAB_HZ5_P25_STASH_CAP
#define HZ5_LOWPAGE64_STASH_CAP BENCHLAB_HZ5_P25_STASH_CAP
#else
#define HZ5_LOWPAGE64_STASH_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP
#ifdef BENCHLAB_HZ5_P25_ADAPTIVE_STASH_CAP
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP \
  BENCHLAB_HZ5_P25_ADAPTIVE_STASH_CAP
#else
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER
#ifdef BENCHLAB_HZ5_P25_ADAPTIVE_STASH_TRIGGER
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER \
  BENCHLAB_HZ5_P25_ADAPTIVE_STASH_TRIGGER
#else
#define HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER 192u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_HOT_CAP
#ifdef BENCHLAB_HZ5_P37_HOT_CAP
#define HZ5_LOWPAGE64_P37_HOT_CAP BENCHLAB_HZ5_P37_HOT_CAP
#else
#define HZ5_LOWPAGE64_P37_HOT_CAP 128u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_OVERFLOW_CAP
#ifdef BENCHLAB_HZ5_P37_OVERFLOW_CAP
#define HZ5_LOWPAGE64_P37_OVERFLOW_CAP BENCHLAB_HZ5_P37_OVERFLOW_CAP
#else
#define HZ5_LOWPAGE64_P37_OVERFLOW_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME
#ifdef BENCHLAB_HZ5_P37_OVERFLOW_LIFETIME
#define HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME \
  BENCHLAB_HZ5_P37_OVERFLOW_LIFETIME
#else
#define HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_HIGH_STREAK
#ifdef BENCHLAB_HZ5_P37_HIGH_STREAK
#define HZ5_LOWPAGE64_P37_HIGH_STREAK BENCHLAB_HZ5_P37_HIGH_STREAK
#else
#define HZ5_LOWPAGE64_P37_HIGH_STREAK 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P37_HIGH_WATER
#ifdef BENCHLAB_HZ5_P37_HIGH_WATER
#define HZ5_LOWPAGE64_P37_HIGH_WATER BENCHLAB_HZ5_P37_HIGH_WATER
#else
#define HZ5_LOWPAGE64_P37_HIGH_WATER HZ5_LOWPAGE64_P37_OVERFLOW_CAP
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_HOT_CAP
#ifdef BENCHLAB_HZ5_P39_HOT_CAP
#define HZ5_LOWPAGE64_P39_HOT_CAP BENCHLAB_HZ5_P39_HOT_CAP
#else
#define HZ5_LOWPAGE64_P39_HOT_CAP 128u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_PROBATION_CAP
#ifdef BENCHLAB_HZ5_P39_PROBATION_CAP
#define HZ5_LOWPAGE64_P39_PROBATION_CAP BENCHLAB_HZ5_P39_PROBATION_CAP
#else
#define HZ5_LOWPAGE64_P39_PROBATION_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_PROBATION_LIFETIME
#ifdef BENCHLAB_HZ5_P39_PROBATION_LIFETIME
#define HZ5_LOWPAGE64_P39_PROBATION_LIFETIME \
  BENCHLAB_HZ5_P39_PROBATION_LIFETIME
#else
#define HZ5_LOWPAGE64_P39_PROBATION_LIFETIME 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P39_USE_ON_MISS
#ifdef BENCHLAB_HZ5_P39_USE_ON_MISS
#define HZ5_LOWPAGE64_P39_USE_ON_MISS BENCHLAB_HZ5_P39_USE_ON_MISS
#else
#define HZ5_LOWPAGE64_P39_USE_ON_MISS 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP
#ifdef BENCHLAB_HZ5_P40_GLOBAL_SOFT_CAP
#define HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP \
  BENCHLAB_HZ5_P40_GLOBAL_SOFT_CAP
#else
#define HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP
#ifdef BENCHLAB_HZ5_P40_GLOBAL_HARD_CAP
#define HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP \
  BENCHLAB_HZ5_P40_GLOBAL_HARD_CAP
#else
#define HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_TTL_EPOCHS
#ifdef BENCHLAB_HZ5_P40_TTL_EPOCHS
#define HZ5_LOWPAGE64_P40_TTL_EPOCHS BENCHLAB_HZ5_P40_TTL_EPOCHS
#else
#define HZ5_LOWPAGE64_P40_TTL_EPOCHS 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_RELEASE_BATCH
#ifdef BENCHLAB_HZ5_P40_RELEASE_BATCH
#define HZ5_LOWPAGE64_P40_RELEASE_BATCH BENCHLAB_HZ5_P40_RELEASE_BATCH
#else
#define HZ5_LOWPAGE64_P40_RELEASE_BATCH 16u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP
#ifdef BENCHLAB_HZ5_P40_SKIP_WHEN_P43_FREE_CAP
#define HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP \
  BENCHLAB_HZ5_P40_SKIP_WHEN_P43_FREE_CAP
#else
#define HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN
#ifdef BENCHLAB_HZ5_P43O_ADMISSION_DRYRUN
#define HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN \
  BENCHLAB_HZ5_P43O_ADMISSION_DRYRUN
#else
#define HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_ADMISSION_GATE
#ifdef BENCHLAB_HZ5_P43O_ADMISSION_GATE
#define HZ5_LOWPAGE64_P43O_ADMISSION_GATE \
  BENCHLAB_HZ5_P43O_ADMISSION_GATE
#else
#define HZ5_LOWPAGE64_P43O_ADMISSION_GATE 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
#ifdef BENCHLAB_HZ5_P43O_PROJECTED_DRYRUN
#define HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN \
  BENCHLAB_HZ5_P43O_PROJECTED_DRYRUN
#else
#define HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_PROJECTED_GATE
#ifdef BENCHLAB_HZ5_P43O_PROJECTED_GATE
#define HZ5_LOWPAGE64_P43O_PROJECTED_GATE \
  BENCHLAB_HZ5_P43O_PROJECTED_GATE
#else
#define HZ5_LOWPAGE64_P43O_PROJECTED_GATE 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_LOW_WATER
#ifdef BENCHLAB_HZ5_P43O_LOW_WATER
#define HZ5_LOWPAGE64_P43O_LOW_WATER BENCHLAB_HZ5_P43O_LOW_WATER
#else
#define HZ5_LOWPAGE64_P43O_LOW_WATER 48u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_HIGH_WATER
#ifdef BENCHLAB_HZ5_P43O_HIGH_WATER
#define HZ5_LOWPAGE64_P43O_HIGH_WATER BENCHLAB_HZ5_P43O_HIGH_WATER
#else
#define HZ5_LOWPAGE64_P43O_HIGH_WATER 80u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_HARD_WATER
#ifdef BENCHLAB_HZ5_P43O_HARD_WATER
#define HZ5_LOWPAGE64_P43O_HARD_WATER BENCHLAB_HZ5_P43O_HARD_WATER
#else
#define HZ5_LOWPAGE64_P43O_HARD_WATER 112u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS
#ifdef BENCHLAB_HZ5_P43O_COOLDOWN_EPOCHS
#define HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS \
  BENCHLAB_HZ5_P43O_COOLDOWN_EPOCHS
#else
#define HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS 2u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN
#ifdef BENCHLAB_HZ5_P43P_BRIDGE_COLD_DRYRUN
#define HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN \
  BENCHLAB_HZ5_P43P_BRIDGE_COLD_DRYRUN
#else
#define HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1
#ifdef BENCHLAB_HZ5_P43P_BRIDGE_COLD_STAGE1
#define HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1 \
  BENCHLAB_HZ5_P43P_BRIDGE_COLD_STAGE1
#else
#define HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
#ifdef BENCHLAB_HZ5_P43P_DEMOTE_BATCH
#define HZ5_LOWPAGE64_P43P_DEMOTE_BATCH \
  BENCHLAB_HZ5_P43P_DEMOTE_BATCH
#else
#define HZ5_LOWPAGE64_P43P_DEMOTE_BATCH 4u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43P_STAGE1_ACQUIRE_LIMIT
#ifdef BENCHLAB_HZ5_P43P_STAGE1_ACQUIRE_LIMIT
#define HZ5_LOWPAGE64_P43P_STAGE1_ACQUIRE_LIMIT \
  BENCHLAB_HZ5_P43P_STAGE1_ACQUIRE_LIMIT
#else
#define HZ5_LOWPAGE64_P43P_STAGE1_ACQUIRE_LIMIT 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN
#ifdef BENCHLAB_HZ5_P43P_AGE_STAGE1_DRYRUN
#define HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN \
  BENCHLAB_HZ5_P43P_AGE_STAGE1_DRYRUN
#else
#define HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P45_CONTROL_PLANE_DRYRUN
#ifdef BENCHLAB_HZ5_P45_CONTROL_PLANE_DRYRUN
#define HZ5_LOWPAGE64_P45_CONTROL_PLANE_DRYRUN \
  BENCHLAB_HZ5_P45_CONTROL_PLANE_DRYRUN
#else
#define HZ5_LOWPAGE64_P45_CONTROL_PLANE_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P45_REFINED_GATE
#ifdef BENCHLAB_HZ5_P45_REFINED_GATE
#define HZ5_LOWPAGE64_P45_REFINED_GATE \
  BENCHLAB_HZ5_P45_REFINED_GATE
#else
#define HZ5_LOWPAGE64_P45_REFINED_GATE 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN
#ifdef BENCHLAB_HZ5_P45_STAGE1_DRAIN_DRYRUN
#define HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN \
  BENCHLAB_HZ5_P45_STAGE1_DRAIN_DRYRUN
#else
#define HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P45_STAGE1_ACQUIRE_LIMIT
#ifdef BENCHLAB_HZ5_P45_STAGE1_ACQUIRE_LIMIT
#define HZ5_LOWPAGE64_P45_STAGE1_ACQUIRE_LIMIT \
  BENCHLAB_HZ5_P45_STAGE1_ACQUIRE_LIMIT
#else
#define HZ5_LOWPAGE64_P45_STAGE1_ACQUIRE_LIMIT 4u
#endif
#endif

#define HZ5_LOWPAGE64_STAGE1_ENABLED \
  (HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1 || HZ5_LOWPAGE64_P45_REFINED_GATE)

#define HZ5_LOWPAGE64_P45_ENABLED \
  (HZ5_LOWPAGE64_P45_CONTROL_PLANE_DRYRUN || \
   HZ5_LOWPAGE64_P45_REFINED_GATE || \
   HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN)

#ifndef HZ5_LOWPAGE64_P43O_ANY
#define HZ5_LOWPAGE64_P43O_ANY \
  (HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
   HZ5_LOWPAGE64_P43O_ADMISSION_GATE || \
   HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN || \
   HZ5_LOWPAGE64_P43O_PROJECTED_GATE || \
   HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN || \
   HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN || \
   HZ5_LOWPAGE64_P45_ENABLED)
#endif

#ifndef HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
#ifdef BENCHLAB_HZ5_P44_BRIDGE_TRIM_DRYRUN
#define HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN \
  BENCHLAB_HZ5_P44_BRIDGE_TRIM_DRYRUN
#else
#define HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP
#ifdef BENCHLAB_HZ5_P44_GLOBAL_SOFT_CAP
#define HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP \
  BENCHLAB_HZ5_P44_GLOBAL_SOFT_CAP
#else
#define HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP 64u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P44_STASH_SOFT_CAP
#ifdef BENCHLAB_HZ5_P44_STASH_SOFT_CAP
#define HZ5_LOWPAGE64_P44_STASH_SOFT_CAP \
  BENCHLAB_HZ5_P44_STASH_SOFT_CAP
#else
#define HZ5_LOWPAGE64_P44_STASH_SOFT_CAP 96u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP
#ifdef BENCHLAB_HZ5_P44_RELBUF_SOFT_CAP
#define HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP \
  BENCHLAB_HZ5_P44_RELBUF_SOFT_CAP
#else
#define HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP HZ5_LOWPAGE64_BATCH_FLUSH_N
#endif
#endif

#ifndef HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP
#ifdef BENCHLAB_HZ5_P44_P43_COMMITTED_FREE_SOFT_CAP
#define HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP \
  BENCHLAB_HZ5_P44_P43_COMMITTED_FREE_SOFT_CAP
#else
#define HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP 64u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P44_TRIM_BATCH
#ifdef BENCHLAB_HZ5_P44_TRIM_BATCH
#define HZ5_LOWPAGE64_P44_TRIM_BATCH BENCHLAB_HZ5_P44_TRIM_BATCH
#else
#define HZ5_LOWPAGE64_P44_TRIM_BATCH 16u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P42_VA_SOURCE
#ifdef BENCHLAB_HZ5_P42_VA_SOURCE
#define HZ5_LOWPAGE64_P42_VA_SOURCE BENCHLAB_HZ5_P42_VA_SOURCE
#else
#define HZ5_LOWPAGE64_P42_VA_SOURCE 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P42_DECOMMIT_COLD
#ifdef BENCHLAB_HZ5_P42_DECOMMIT_COLD
#define HZ5_LOWPAGE64_P42_DECOMMIT_COLD \
  BENCHLAB_HZ5_P42_DECOMMIT_COLD
#else
#define HZ5_LOWPAGE64_P42_DECOMMIT_COLD 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P42_RECOMMIT_BATCH
#ifdef BENCHLAB_HZ5_P42_RECOMMIT_BATCH
#define HZ5_LOWPAGE64_P42_RECOMMIT_BATCH \
  BENCHLAB_HZ5_P42_RECOMMIT_BATCH
#else
#define HZ5_LOWPAGE64_P42_RECOMMIT_BATCH 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_SPAN_CACHE
#ifdef BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#define HZ5_LOWPAGE64_SPAN_CACHE BENCHLAB_HZ5_P25_SPAN_CACHE64K_A8192
#else
#define HZ5_LOWPAGE64_SPAN_CACHE 0
#endif
#endif

#ifndef HZ5_DIAGNOSTIC_STATS
#define HZ5_DIAGNOSTIC_STATS 0
#endif

#ifndef HZ5_LOWPAGE64_STATS
#ifdef BENCHLAB_HZ5_P25_STATS
#define HZ5_LOWPAGE64_STATS BENCHLAB_HZ5_P25_STATS
#else
#define HZ5_LOWPAGE64_STATS HZ5_DIAGNOSTIC_STATS
#endif
#endif

#if HZ5_LOWPAGE64_STATS
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
static void hz5_lowpage64_count_max(_Atomic size_t* counter, size_t value) {
  size_t cur = atomic_load_explicit(counter, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             counter, &cur, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}
#define HZ5_LOWPAGE64_COUNT_MAX(counter, value) \
  hz5_lowpage64_count_max(&(counter), (value))
#else
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) ((void)0)
#define HZ5_LOWPAGE64_COUNT_MAX(counter, value) ((void)0)
#endif

#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P44 BridgeTrimDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P43o AdmissionDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P43o ProjectedDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P43p BridgeColdDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P45_ENABLED && !HZ5_LOWPAGE64_STATS
#error "P45 control-plane lanes require lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN && \
    !HZ5_LOWPAGE64_P45_REFINED_GATE
#error "P45 Stage1DrainDryRun requires P45 RefinedGate"
#endif

#if HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN && \
    !HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN
#error "P43p AgeStage1DryRun requires P43p BridgeColdDryRun"
#endif

static _Atomic(uintptr_t) g_hz5_lowpage64_global_batch_head;
static _Atomic size_t g_hz5_lowpage64_global_batch_count;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_min;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_max;
#if HZ5_LOWPAGE64_STAGE1_ENABLED
static _Atomic(uintptr_t) g_hz5_lowpage64_p43p_stage1_head;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_count;
#endif
#if (HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD) || \
    HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_p42_cold_count;
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

static _Atomic(uintptr_t) g_hz5_lowpage64_p42_cold_head;
#endif

#if HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_alloc_calls;
static _Atomic size_t g_hz5_lowpage64_span_hits;
static _Atomic size_t g_hz5_lowpage64_span_pushes;
static _Atomic size_t g_hz5_lowpage64_span_full;
static _Atomic size_t g_hz5_lowpage64_stash_hits;
static _Atomic size_t g_hz5_lowpage64_global_batch_hits;
static _Atomic size_t g_hz5_lowpage64_os_allocs;
static _Atomic size_t g_hz5_lowpage64_free_calls;
static _Atomic size_t g_hz5_lowpage64_relbuf_pushes;
static _Atomic size_t g_hz5_lowpage64_batch_publishes;
static _Atomic size_t g_hz5_lowpage64_batch_publish_nodes;
static _Atomic size_t g_hz5_lowpage64_global_overflow_frees;
static _Atomic size_t g_hz5_lowpage64_relbuf_count_max;
static _Atomic size_t g_hz5_lowpage64_global_count_before_max;
static _Atomic size_t g_hz5_lowpage64_global_count_after_max;
static _Atomic size_t g_hz5_lowpage64_acquired_count_total;
static _Atomic size_t g_hz5_lowpage64_acquired_count_max;
static _Atomic size_t g_hz5_lowpage64_stash_set_calls;
static _Atomic size_t g_hz5_lowpage64_stash_set_nodes_total;
static _Atomic size_t g_hz5_lowpage64_stash_set_nodes_max;
static _Atomic size_t g_hz5_lowpage64_trim_calls;
static _Atomic size_t g_hz5_lowpage64_trim_freed_nodes;
static _Atomic size_t g_hz5_lowpage64_adaptive_trigger_calls;
static _Atomic size_t g_hz5_lowpage64_acquired_le64;
static _Atomic size_t g_hz5_lowpage64_acquired_le128;
static _Atomic size_t g_hz5_lowpage64_acquired_le192;
static _Atomic size_t g_hz5_lowpage64_acquired_gt192;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_le64;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_le128;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_le192;
static _Atomic size_t g_hz5_lowpage64_stash_nodes_gt192;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_hits;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_set_nodes_total;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_set_nodes_max;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_release_calls;
static _Atomic size_t g_hz5_lowpage64_p37_overflow_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p37_high_event_releases;
static _Atomic size_t g_hz5_lowpage64_p37_lifetime_releases;
static _Atomic size_t g_hz5_lowpage64_p38_hot_hits;
static _Atomic size_t g_hz5_lowpage64_p38_global_first_hits;
static _Atomic size_t g_hz5_lowpage64_p38_set_hot_nodes_total;
static _Atomic size_t g_hz5_lowpage64_p38_set_hot_nodes_max;
static _Atomic size_t g_hz5_lowpage64_p38_global_cap_release_calls;
static _Atomic size_t g_hz5_lowpage64_p38_global_cap_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p38_stash_trim_release_calls;
static _Atomic size_t g_hz5_lowpage64_p38_stash_trim_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p38_p37_release_calls;
static _Atomic size_t g_hz5_lowpage64_p38_p37_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p39_probation_hits;
static _Atomic size_t g_hz5_lowpage64_p39_probation_set_nodes_total;
static _Atomic size_t g_hz5_lowpage64_p39_probation_set_nodes_max;
static _Atomic size_t g_hz5_lowpage64_p39_probation_release_calls;
static _Atomic size_t g_hz5_lowpage64_p39_probation_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p39_probation_lifetime_releases;
static _Atomic size_t g_hz5_lowpage64_p39_probation_replace_releases;
static _Atomic size_t g_hz5_lowpage64_p40_checkpoint_calls;
static _Atomic size_t g_hz5_lowpage64_p40_release_calls;
static _Atomic size_t g_hz5_lowpage64_p40_release_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_release_age_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_release_hard_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_keep_nodes;
static _Atomic size_t g_hz5_lowpage64_p40_p43_free_before_total;
static _Atomic size_t g_hz5_lowpage64_p40_p43_free_before_max;
static _Atomic size_t g_hz5_lowpage64_p40_p43_committed_before_total;
static _Atomic size_t g_hz5_lowpage64_p40_p43_committed_before_max;
static _Atomic size_t g_hz5_lowpage64_p40_skip_p43_free_calls;
static _Atomic size_t g_hz5_lowpage64_p40_skip_p43_free_nodes;
static _Atomic size_t g_hz5_lowpage64_p43o_epoch;
static _Atomic size_t g_hz5_lowpage64_p43o_state_open;
static _Atomic size_t g_hz5_lowpage64_p43o_state_drain;
static _Atomic size_t g_hz5_lowpage64_p43o_state_closed;
static _Atomic size_t g_hz5_lowpage64_p43o_admission_flips;
static _Atomic size_t g_hz5_lowpage64_p43o_reason_acquire_miss;
static _Atomic size_t g_hz5_lowpage64_p43o_reason_p40;
static _Atomic size_t g_hz5_lowpage64_p43o_reason_snapshot;
static _Atomic size_t g_hz5_lowpage64_p43o_p43_free_max;
static _Atomic size_t g_hz5_lowpage64_p43o_p43_free_hot_total;
static _Atomic size_t g_hz5_lowpage64_p43o_p43_free_warm_total;
static _Atomic size_t g_hz5_lowpage64_p43o_p43_free_cold_total;
static _Atomic size_t g_hz5_lowpage64_p43o_p40_release_candidates;
static _Atomic size_t g_hz5_lowpage64_p43o_p40_would_stage1;
static _Atomic size_t g_hz5_lowpage64_p43o_p40_would_demote_to_source;
static _Atomic size_t g_hz5_lowpage64_p43o_p40_would_keep_bridge;
static _Atomic size_t g_hz5_lowpage64_p43o_source_miss;
static _Atomic size_t g_hz5_lowpage64_p43o_projected_free_max;
static _Atomic size_t g_hz5_lowpage64_p43o_projected_p40_candidates;
static _Atomic size_t g_hz5_lowpage64_p43o_projected_would_stage1;
static _Atomic size_t g_hz5_lowpage64_p43o_projected_would_demote_to_source;
static _Atomic size_t g_hz5_lowpage64_p43o_projected_would_keep_bridge;
static _Atomic size_t g_hz5_lowpage64_p43o_projected_blocks;
static _Atomic size_t g_hz5_lowpage64_p43p_epoch;
static _Atomic size_t g_hz5_lowpage64_p43p_state_open;
static _Atomic size_t g_hz5_lowpage64_p43p_state_drain;
static _Atomic size_t g_hz5_lowpage64_p43p_state_closed;
static _Atomic size_t g_hz5_lowpage64_p43p_admission_flips;
static _Atomic size_t g_hz5_lowpage64_p43p_reason_p40;
static _Atomic size_t g_hz5_lowpage64_p43p_reason_snapshot;
static _Atomic size_t g_hz5_lowpage64_p43p_p40_release_candidates;
static _Atomic size_t g_hz5_lowpage64_p43p_would_stage1_bridge_cold;
static _Atomic size_t g_hz5_lowpage64_p43p_would_demote_to_source;
static _Atomic size_t g_hz5_lowpage64_p43p_would_keep_bridge;
static _Atomic size_t g_hz5_lowpage64_p43p_bridge_cold_projected_current;
static _Atomic size_t g_hz5_lowpage64_p43p_bridge_cold_projected_max;
static _Atomic size_t g_hz5_lowpage64_p43p_p43_committed_free_current;
static _Atomic size_t g_hz5_lowpage64_p43p_p43_committed_free_max;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_enqueue_calls;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_enqueue_nodes;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_acquire_hits;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_acquire_nodes;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_requeue_calls;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_requeue_nodes;
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_current_max;
static _Atomic size_t g_hz5_lowpage64_p43p_age_stage1_items_total;
static _Atomic size_t g_hz5_lowpage64_p43p_age_young_current;
static _Atomic size_t g_hz5_lowpage64_p43p_age_old_current;
static _Atomic size_t g_hz5_lowpage64_p43p_age_would_reuse_young;
static _Atomic size_t g_hz5_lowpage64_p43p_age_would_keep_old;
static _Atomic size_t g_hz5_lowpage64_p43p_age_would_demote_old_open;
static _Atomic size_t g_hz5_lowpage64_p43p_age_would_block_old_drain;
static _Atomic size_t g_hz5_lowpage64_p43p_age_would_block_old_closed;
static _Atomic size_t g_hz5_lowpage64_p43p_age_hot_reinject_avoided;
static _Atomic size_t g_hz5_lowpage64_p43p_age_pop_all_pollution_projection;
static _Atomic size_t g_hz5_lowpage64_p43p_age_admission_flips;
#if HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p43p_age_bucket0;
static _Atomic size_t g_hz5_lowpage64_p43p_age_bucket1;
static _Atomic size_t g_hz5_lowpage64_p43p_age_bucket2;
static _Atomic size_t g_hz5_lowpage64_p43p_age_bucket3;
static _Atomic size_t g_hz5_lowpage64_p43p_age_bucket4;
static _Atomic size_t g_hz5_lowpage64_p43p_age_bucket_old;
#endif
static _Atomic size_t g_hz5_lowpage64_p42_va_allocs;
static _Atomic size_t g_hz5_lowpage64_p42_va_alloc_failures;
static _Atomic size_t g_hz5_lowpage64_p42_va_releases;
static _Atomic size_t g_hz5_lowpage64_p42_decommit_calls;
static _Atomic size_t g_hz5_lowpage64_p42_decommit_failures;
static _Atomic size_t g_hz5_lowpage64_p42_recommit_calls;
static _Atomic size_t g_hz5_lowpage64_p42_recommit_failures;
static _Atomic size_t g_hz5_lowpage64_p42_cold_hits;
static _Atomic size_t g_hz5_lowpage64_p42_cold_count_max;
static _Atomic size_t g_hz5_lowpage64_p43g_prepare_calls;
static _Atomic size_t g_hz5_lowpage64_p43g_prepare_active;
static _Atomic size_t g_hz5_lowpage64_p43g_prepare_nonactive;
static _Atomic size_t g_hz5_lowpage64_p43g_prepare_miss;
static _Atomic size_t g_hz5_lowpage64_p43g_source_p25;
static _Atomic size_t g_hz5_lowpage64_p43g_source_other;
static _Atomic size_t g_hz5_lowpage64_p43g_raw_mismatch;
static _Atomic size_t g_hz5_lowpage64_p43g_release_old_path_calls;
static _Atomic size_t g_hz5_lowpage64_p43g_release_prepared_calls;
static _Atomic size_t g_hz5_lowpage64_p44_checkpoint_calls;
static _Atomic size_t g_hz5_lowpage64_p44_reason_publish;
static _Atomic size_t g_hz5_lowpage64_p44_reason_acquire_miss;
static _Atomic size_t g_hz5_lowpage64_p44_reason_p40;
static _Atomic size_t g_hz5_lowpage64_p44_reason_snapshot;
#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p44_global_observed_total;
#endif
static _Atomic size_t g_hz5_lowpage64_p44_global_observed_max;
static _Atomic size_t g_hz5_lowpage64_p44_global_candidate_total;
#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p44_global_candidate_max;
static _Atomic size_t g_hz5_lowpage64_p44_stash_observed_total;
#endif
static _Atomic size_t g_hz5_lowpage64_p44_stash_observed_max;
static _Atomic size_t g_hz5_lowpage64_p44_stash_candidate_total;
#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p44_stash_candidate_max;
static _Atomic size_t g_hz5_lowpage64_p44_relbuf_observed_total;
#endif
static _Atomic size_t g_hz5_lowpage64_p44_relbuf_observed_max;
static _Atomic size_t g_hz5_lowpage64_p44_relbuf_candidate_total;
#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p44_relbuf_candidate_max;
static _Atomic size_t g_hz5_lowpage64_p44_p43_free_observed_total;
#endif
static _Atomic size_t g_hz5_lowpage64_p44_p43_free_observed_max;
static _Atomic size_t g_hz5_lowpage64_p44_p43_free_candidate_total;
#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p44_p43_free_candidate_max;
#endif
static _Atomic size_t g_hz5_lowpage64_p44_candidate_total;
static _Atomic size_t g_hz5_lowpage64_p44_candidate_max;
static _Atomic size_t g_hz5_lowpage64_p45_epoch;
static _Atomic size_t g_hz5_lowpage64_p45_state_open;
static _Atomic size_t g_hz5_lowpage64_p45_state_drain;
static _Atomic size_t g_hz5_lowpage64_p45_state_closed;
static _Atomic size_t g_hz5_lowpage64_p45_admission_flips;
static _Atomic size_t g_hz5_lowpage64_p45_reason_publish;
static _Atomic size_t g_hz5_lowpage64_p45_reason_acquire_miss;
static _Atomic size_t g_hz5_lowpage64_p45_reason_p40;
static _Atomic size_t g_hz5_lowpage64_p45_reason_snapshot;
static _Atomic size_t g_hz5_lowpage64_p45_bridge_current_max;
static _Atomic size_t g_hz5_lowpage64_p45_bridge_pressure_total;
static _Atomic size_t g_hz5_lowpage64_p45_source_free_current_max;
static _Atomic size_t g_hz5_lowpage64_p45_source_free_pressure_total;
static _Atomic size_t g_hz5_lowpage64_p45_source_committed_current_max;
static _Atomic size_t g_hz5_lowpage64_p45_source_committed_pressure_total;
static _Atomic size_t g_hz5_lowpage64_p45_p40_demote_intent;
static _Atomic size_t g_hz5_lowpage64_p45_p40_demote_while_drain_closed;
static _Atomic size_t g_hz5_lowpage64_p45_p40_demote_while_bridge_nonempty;
static _Atomic size_t g_hz5_lowpage64_p45_would_demote_clean_open;
static _Atomic size_t g_hz5_lowpage64_p45_would_stage1_any;
static _Atomic size_t g_hz5_lowpage64_p45_would_stage1_state;
static _Atomic size_t g_hz5_lowpage64_p45_would_stage1_bridge_nonempty;
static _Atomic size_t g_hz5_lowpage64_p45_would_stage1_overlap;
static _Atomic size_t g_hz5_lowpage64_p45_bridge_residual_after_demote_max;
static _Atomic size_t g_hz5_lowpage64_p45_bridge_residual_after_demote_total;
static _Atomic size_t g_hz5_lowpage64_p45_would_demote_open_bridge_soft;
static _Atomic size_t g_hz5_lowpage64_p45_would_stage1_bridge_excess;
static _Atomic size_t g_hz5_lowpage64_p45_would_stage1_refined_any;
static _Atomic size_t g_hz5_lowpage64_p45_source_miss;
static _Atomic size_t g_hz5_lowpage64_p45rg_demote_intent;
static _Atomic size_t g_hz5_lowpage64_p45rg_allowed_open_bridge_soft;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_any;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_state;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_bridge_excess;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_overlap;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_enqueue_calls;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_enqueue_nodes;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_acquire_hits;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_acquire_nodes;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_requeue_calls;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_requeue_nodes;
static _Atomic size_t g_hz5_lowpage64_p45rg_stage1_current_max;
static _Atomic size_t g_hz5_lowpage64_p45rg_bridge_residual_max;
static _Atomic size_t g_hz5_lowpage64_p45rg_bridge_residual_total;
static _Atomic size_t g_hz5_lowpage64_p45dr_checkpoint_calls;
static _Atomic size_t g_hz5_lowpage64_p45dr_reason_p40;
static _Atomic size_t g_hz5_lowpage64_p45dr_reason_acquire_miss;
static _Atomic size_t g_hz5_lowpage64_p45dr_reason_snapshot;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_current_at_snapshot;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_current_at_snapshot_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_age_bucket0;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_age_bucket1;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_age_bucket2_4;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_age_old;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_old_current_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_stage1_oldest_age_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_would_drain_to_bridge;
static _Atomic size_t g_hz5_lowpage64_p45dr_would_keep_cold;
static _Atomic size_t g_hz5_lowpage64_p45dr_would_demote_open;
static _Atomic size_t g_hz5_lowpage64_p45dr_would_block_drain_closed;
static _Atomic size_t g_hz5_lowpage64_p45dr_would_block_bridge_excess;
static _Atomic size_t g_hz5_lowpage64_p45dr_bridge_current_hot_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_bridge_cold_current_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_bridge_total_with_cold_max;
static _Atomic size_t g_hz5_lowpage64_p45dr_balance_expected;
static _Atomic size_t g_hz5_lowpage64_p45dr_balance_actual;
static _Atomic size_t g_hz5_lowpage64_p45dr_balance_mismatch;
static _Atomic size_t g_hz5_lowpage64_p45dr_balance_mismatch_max;
#endif

#if HZ5_LOWPAGE64_SPAN_CACHE
__declspec(thread) static void* g_hz5_lowpage64_span_head;
__declspec(thread) static size_t g_hz5_lowpage64_span_count;
#endif
__declspec(thread) static void* g_hz5_lowpage64_stash_head;
__declspec(thread) static size_t g_hz5_lowpage64_stash_count;
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
__declspec(thread) static void* g_hz5_lowpage64_relbuf_head;
__declspec(thread) static void* g_hz5_lowpage64_relbuf_tail;
__declspec(thread) static size_t g_hz5_lowpage64_relbuf_count;

static void hz5_lowpage64_link_next(void* raw, void* next) {
  *(void**)raw = next;
}

static void* hz5_lowpage64_link_next_load(void* raw) {
  return *(void**)raw;
}

#if HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD
static size_t hz5_lowpage64_p42_raw_bytes(void) {
  return HZ5_LOWPAGE64_RAW_PAGE_SIZE * (size_t)2u;
}

static void hz5_lowpage64_p42_cold_push(Hz5Lowpage64ColdNode* node) {
  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_p42_cold_head, memory_order_acquire);
  do {
    node->next = (Hz5Lowpage64ColdNode*)old_head;
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_p42_cold_head, &old_head, (uintptr_t)node,
      memory_order_acq_rel, memory_order_acquire));
  size_t count = atomic_fetch_add_explicit(
                     &g_hz5_lowpage64_p42_cold_count, 1,
                     memory_order_relaxed) +
                 1u;
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p42_cold_count_max, count);
  (void)count;
}

static Hz5Lowpage64ColdNode* hz5_lowpage64_p42_cold_pop(void) {
  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_p42_cold_head, memory_order_acquire);
  while (old_head) {
    Hz5Lowpage64ColdNode* node = (Hz5Lowpage64ColdNode*)old_head;
    uintptr_t next = (uintptr_t)node->next;
    if (atomic_compare_exchange_weak_explicit(
            &g_hz5_lowpage64_p42_cold_head, &old_head, next,
            memory_order_acq_rel, memory_order_acquire)) {
      atomic_fetch_sub_explicit(&g_hz5_lowpage64_p42_cold_count, 1,
                                memory_order_relaxed);
      node->next = NULL;
      return node;
    }
  }
  return NULL;
}

static void* hz5_lowpage64_p42_try_recommit(size_t raw_bytes) {
#if defined(_WIN32)
  for (size_t i = 0; i < HZ5_LOWPAGE64_P42_RECOMMIT_BATCH; i++) {
    Hz5Lowpage64ColdNode* node = hz5_lowpage64_p42_cold_pop();
    if (!node) {
      return NULL;
    }
    void* raw = VirtualAlloc(node->raw, node->bytes, MEM_COMMIT,
                             PAGE_READWRITE);
    if (raw) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_recommit_calls, 1);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_cold_hits, 1);
      free(node);
      (void)raw_bytes;
      return raw;
    }
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_recommit_failures, 1);
    VirtualFree(node->raw, 0, MEM_RELEASE);
    free(node);
  }
#else
  (void)raw_bytes;
#endif
  return NULL;
}
#endif

static void* hz5_lowpage64_raw_os_alloc(size_t raw_bytes) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#if defined(_WIN32)
  // P43 is the lower raw source for P43i/P44. The hot P25 bridge still lives
  // above this function in acquire/release_common.
  void* raw = hz5_lowpage64_p43_alloc_slot(raw_bytes);
  return raw;
#else
  return NULL;
#endif
#endif
#if HZ5_LOWPAGE64_P42_VA_SOURCE
#if defined(_WIN32)
  void* raw = VirtualAlloc(NULL, raw_bytes, MEM_RESERVE | MEM_COMMIT,
                           PAGE_READWRITE);
  if (!raw) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_alloc_failures, 1);
    return NULL;
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_allocs, 1);
  return raw;
#else
  return NULL;
#endif
#else
  return _aligned_malloc(raw_bytes, HZ5_LOWPAGE64_RAW_PAGE_SIZE);
#endif
}

static void hz5_lowpage64_raw_os_release(void* raw) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#if defined(_WIN32)
  // Only direct descriptor-release controls should commonly reach this P43
  // release path. P43i/P44 balanced lanes normally preserve release_common().
  (void)hz5_lowpage64_p43_release_slot(raw);
  return;
#endif
#endif
#if HZ5_LOWPAGE64_P42_VA_SOURCE
#if defined(_WIN32)
  if (raw) {
#if HZ5_LOWPAGE64_P42_DECOMMIT_COLD
    Hz5Lowpage64ColdNode* node =
        (Hz5Lowpage64ColdNode*)malloc(sizeof(Hz5Lowpage64ColdNode));
    if (node && VirtualFree(raw, hz5_lowpage64_p42_raw_bytes(),
                            MEM_DECOMMIT)) {
      node->raw = raw;
      node->bytes = hz5_lowpage64_p42_raw_bytes();
      node->next = NULL;
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_decommit_calls, 1);
      hz5_lowpage64_p42_cold_push(node);
      return;
    }
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_decommit_failures, 1);
    free(node);
#endif
    VirtualFree(raw, 0, MEM_RELEASE);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p42_va_releases, 1);
  }
#else
  (void)raw;
#endif
#else
  _aligned_free(raw);
#endif
}

#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_node_epoch_store(void* raw, uint32_t epoch) {
  *(uint32_t*)((char*)raw + sizeof(void*)) = epoch;
}

static uint32_t hz5_lowpage64_node_epoch_load(void* raw) {
  return *(uint32_t*)((char*)raw + sizeof(void*));
}

static void hz5_lowpage64_mark_list_epoch(void* head, uint32_t epoch) {
  while (head) {
    hz5_lowpage64_node_epoch_store(head, epoch);
    head = hz5_lowpage64_link_next_load(head);
  }
}
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP == 0 && \
    HZ5_LOWPAGE64_P39_PROBATION_CAP == 0
static size_t hz5_lowpage64_list_count(void* head) {
  size_t count = 0;
  while (head) {
    count++;
    head = hz5_lowpage64_link_next_load(head);
  }
  return count;
}
#endif

size_t hz5_lowpage64_round_raw_bytes(size_t size,
                                     size_t alignment,
                                     size_t header_size) {
  size_t raw_request = size + alignment + header_size;
  return (raw_request + HZ5_LOWPAGE64_RAW_PAGE_SIZE - 1u) &
         ~(HZ5_LOWPAGE64_RAW_PAGE_SIZE - 1u);
}

void hz5_lowpage64_note_raw_range(void* raw_ptr, size_t raw_bytes) {
  uintptr_t begin = (uintptr_t)raw_ptr;
  uintptr_t end = begin + raw_bytes;

  uintptr_t min_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_min, memory_order_relaxed);
  while ((min_cur == 0 || begin < min_cur) &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_lowpage64_raw_min, &min_cur, begin, memory_order_relaxed,
             memory_order_relaxed)) {
  }

  uintptr_t max_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_max, memory_order_relaxed);
  while (end > max_cur &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_lowpage64_raw_max, &max_cur, end, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

int hz5_lowpage64_lookup(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  return hz5_lowpage64_p43_lookup(ptr);
#else
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t min_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_min, memory_order_acquire);
  uintptr_t max_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_max, memory_order_acquire);
  return (min_cur != 0 && p >= min_cur && p < max_cur)
             ? HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE
             : HZ5_LOWPAGE64_LOOKUP_MISS;
#endif
}

static void hz5_lowpage64_prepare_ctx_clear(Hz5Lowpage64FreeCtx* ctx) {
  if (!ctx) {
    return;
  }
  ctx->lookup_kind = HZ5_LOWPAGE64_LOOKUP_MISS;
  ctx->segment_token = NULL;
  ctx->slot_base = NULL;
  ctx->slot_size = 0;
  ctx->slot_index = 0;
  ctx->flags = 0;
}

static void hz5_lowpage64_p43g_count_prepare(int lookup_kind) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_calls, 1);
  if (lookup_kind == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_active, 1);
  } else if (lookup_kind == HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_nonactive, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_prepare_miss, 1);
  }
#else
  (void)lookup_kind;
#endif
}

int hz5_lowpage64_prepare_free_user(void* ptr, Hz5Lowpage64FreeCtx* ctx) {
  hz5_lowpage64_prepare_ctx_clear(ctx);
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  int lookup_kind = hz5_lowpage64_p43_prepare_free_user(ptr, ctx);
#else
  int lookup_kind = hz5_lowpage64_lookup(ptr);
  if (ctx) {
    ctx->lookup_kind = lookup_kind;
  }
#endif
  hz5_lowpage64_p43g_count_prepare(lookup_kind);
  return lookup_kind;
}

int hz5_lowpage64_may_own(void* ptr) {
  return hz5_lowpage64_lookup(ptr) != HZ5_LOWPAGE64_LOOKUP_MISS;
}

int hz5_lowpage64_active_owns(void* ptr) {
  return hz5_lowpage64_lookup(ptr) == HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
}

void hz5_lowpage64_p43g_note_wrapper(int is_p25_source, int raw_match) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  if (is_p25_source) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_source_p25, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_source_other, 1);
  }
  if (!raw_match) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_raw_mismatch, 1);
  }
#else
  (void)is_p25_source;
  (void)raw_match;
#endif
}

void hz5_lowpage64_p43g_note_old_path(void) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_release_old_path_calls, 1);
#endif
}

void hz5_lowpage64_p43g_note_prepared_path(void) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43g_release_prepared_calls, 1);
#endif
}

#if HZ5_LOWPAGE64_STATS || HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static size_t hz5_lowpage64_p44_candidate_value(size_t observed,
                                                size_t soft_cap) {
  if (observed <= soft_cap) {
    return 0;
  }
  size_t candidate = observed - soft_cap;
  if (candidate > HZ5_LOWPAGE64_P44_TRIM_BATCH) {
    candidate = HZ5_LOWPAGE64_P44_TRIM_BATCH;
  }
  return candidate;
}
#endif

#if HZ5_LOWPAGE64_P43O_ANY
typedef enum Hz5Lowpage64P43oReason {
  HZ5_LOWPAGE64_P43O_REASON_ACQUIRE_MISS = 1,
  HZ5_LOWPAGE64_P43O_REASON_P40 = 2,
  HZ5_LOWPAGE64_P43O_REASON_SNAPSHOT = 3,
} Hz5Lowpage64P43oReason;

typedef enum Hz5Lowpage64P43oState {
  HZ5_LOWPAGE64_P43O_STATE_OPEN = 0,
  HZ5_LOWPAGE64_P43O_STATE_DRAIN = 1,
  HZ5_LOWPAGE64_P43O_STATE_CLOSED = 2,
} Hz5Lowpage64P43oState;

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
static void hz5_lowpage64_p43o_bucket_free(size_t free_current,
                                           size_t* hot,
                                           size_t* warm,
                                           size_t* cold) {
  size_t low = HZ5_LOWPAGE64_P43O_LOW_WATER;
  size_t high = HZ5_LOWPAGE64_P43O_HIGH_WATER;
  *hot = free_current < low ? free_current : low;
  if (free_current <= low) {
    *warm = 0;
  } else {
    size_t warm_limit = high > low ? high - low : 0;
    size_t warm_current = free_current - low;
    *warm = warm_current < warm_limit ? warm_current : warm_limit;
  }
  *cold = free_current > high ? free_current - high : 0;
}
#else
#define hz5_lowpage64_p43o_bucket_free(free_current, hot, warm, cold) \
  do {                                                               \
    *(hot) = 0;                                                      \
    *(warm) = 0;                                                     \
    *(cold) = 0;                                                     \
  } while (0)
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_ADMISSION_GATE || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN || \
    HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN || \
    HZ5_LOWPAGE64_P45_ENABLED
static size_t hz5_lowpage64_p43o_choose_state(size_t old_state,
                                              size_t free_current,
                                              _Atomic size_t* cooldown_ptr) {
  if (free_current >= HZ5_LOWPAGE64_P43O_HARD_WATER) {
    atomic_store_explicit(cooldown_ptr, HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS,
                          memory_order_relaxed);
    return HZ5_LOWPAGE64_P43O_STATE_CLOSED;
  }
  if (free_current >= HZ5_LOWPAGE64_P43O_HIGH_WATER) {
    atomic_store_explicit(cooldown_ptr, HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS,
                          memory_order_relaxed);
    return HZ5_LOWPAGE64_P43O_STATE_DRAIN;
  }
  if (free_current > HZ5_LOWPAGE64_P43O_LOW_WATER) {
    return old_state;
  }
  if (old_state == HZ5_LOWPAGE64_P43O_STATE_OPEN) {
    return HZ5_LOWPAGE64_P43O_STATE_OPEN;
  }
  size_t cooldown =
      atomic_load_explicit(cooldown_ptr, memory_order_relaxed);
  if (cooldown > 0) {
    atomic_store_explicit(cooldown_ptr, cooldown - 1,
                          memory_order_relaxed);
    return old_state;
  }
  return HZ5_LOWPAGE64_P43O_STATE_OPEN;
}
#endif

static size_t hz5_lowpage64_p43o_release_candidates(size_t observed_global) {
  if (observed_global <= HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    return 0;
  }
  return observed_global - HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP;
}

#if HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_GATE
static size_t hz5_lowpage64_p43o_projected_release_batch(
    size_t release_candidates) {
  return release_candidates < HZ5_LOWPAGE64_P40_RELEASE_BATCH
             ? release_candidates
             : HZ5_LOWPAGE64_P40_RELEASE_BATCH;
}

static int hz5_lowpage64_p43o_projected_blocks(size_t free_current,
                                               size_t release_candidates) {
  size_t projected_batch =
      hz5_lowpage64_p43o_projected_release_batch(release_candidates);
  if (projected_batch == 0) {
    return 0;
  }
  return free_current + projected_batch >= HZ5_LOWPAGE64_P43O_HIGH_WATER;
}
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_GATE
static int hz5_lowpage64_p43o_source_admission_open(
    const Hz5Lowpage64P43StatsSnapshot* stats) {
  size_t old_state = atomic_load_explicit(
      &g_hz5_lowpage64_p43o_runtime_state, memory_order_relaxed);
  size_t new_state = hz5_lowpage64_p43o_choose_state(
      old_state, stats ? stats->slots_committed_free_current : 0,
      &g_hz5_lowpage64_p43o_runtime_cooldown);
  atomic_store_explicit(&g_hz5_lowpage64_p43o_runtime_state, new_state,
                        memory_order_relaxed);
  return new_state == HZ5_LOWPAGE64_P43O_STATE_OPEN;
}
#else
#define hz5_lowpage64_p43o_source_admission_open(stats) (1)
#endif

#if HZ5_LOWPAGE64_P43O_PROJECTED_GATE
static int hz5_lowpage64_p43o_projected_source_admission_open(
    const Hz5Lowpage64P43StatsSnapshot* stats,
    size_t release_candidates) {
  size_t free_current = stats ? stats->slots_committed_free_current : 0;
  return !hz5_lowpage64_p43o_projected_blocks(free_current,
                                              release_candidates);
}
#else
#define hz5_lowpage64_p43o_projected_source_admission_open(stats, candidates) \
  (1)
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
static void hz5_lowpage64_p43o_note(
    Hz5Lowpage64P43oReason reason,
    size_t observed_global,
    const Hz5Lowpage64P43StatsSnapshot* maybe_stats) {
  Hz5Lowpage64P43StatsSnapshot local_stats = {0};
  const Hz5Lowpage64P43StatsSnapshot* stats = maybe_stats;
  if (!stats) {
    hz5_lowpage64_p43_stats_snapshot(&local_stats);
    stats = &local_stats;
  }

  size_t free_current = stats->slots_committed_free_current;
  size_t old_state = atomic_load_explicit(
      &g_hz5_lowpage64_p43o_runtime_state, memory_order_relaxed);
  size_t new_state = hz5_lowpage64_p43o_choose_state(
      old_state, free_current, &g_hz5_lowpage64_p43o_runtime_cooldown);
  if (new_state != old_state) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_admission_flips, 1);
  }
  atomic_store_explicit(&g_hz5_lowpage64_p43o_runtime_state, new_state,
                        memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_epoch, 1);
  switch (new_state) {
    case HZ5_LOWPAGE64_P43O_STATE_OPEN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_state_open, 1);
      break;
    case HZ5_LOWPAGE64_P43O_STATE_DRAIN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_state_drain, 1);
      break;
    case HZ5_LOWPAGE64_P43O_STATE_CLOSED:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_state_closed, 1);
      break;
  }
  switch (reason) {
    case HZ5_LOWPAGE64_P43O_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_reason_acquire_miss, 1);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_source_miss, 1);
      break;
    case HZ5_LOWPAGE64_P43O_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_reason_p40, 1);
      break;
    case HZ5_LOWPAGE64_P43O_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_reason_snapshot, 1);
      break;
  }

  size_t hot = 0;
  size_t warm = 0;
  size_t cold = 0;
  hz5_lowpage64_p43o_bucket_free(free_current, &hot, &warm, &cold);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43o_p43_free_max,
                          free_current);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p43_free_hot_total, hot);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p43_free_warm_total, warm);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p43_free_cold_total, cold);

  size_t release_candidates = 0;
  if (reason == HZ5_LOWPAGE64_P43O_REASON_P40) {
    release_candidates =
        hz5_lowpage64_p43o_release_candidates(observed_global);
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p40_release_candidates,
                          release_candidates);
  if (release_candidates > 0) {
    if (new_state == HZ5_LOWPAGE64_P43O_STATE_OPEN) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_p40_would_demote_to_source,
          release_candidates);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p40_would_stage1,
                              release_candidates);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p40_would_keep_bridge,
                              release_candidates);
    }
  }

#if HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
  if (release_candidates > 0) {
    size_t projected_batch =
        hz5_lowpage64_p43o_projected_release_batch(release_candidates);
    size_t projected_free = free_current + projected_batch;
    int projected_blocks = hz5_lowpage64_p43o_projected_blocks(
        free_current, release_candidates);
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43o_projected_p40_candidates,
        release_candidates);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43o_projected_free_max,
                            projected_free);
    if (projected_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_projected_would_stage1,
          release_candidates);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_projected_would_keep_bridge,
          release_candidates);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_projected_blocks, 1);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_projected_would_demote_to_source,
          release_candidates);
    }
  }
#endif
}
#else
#define hz5_lowpage64_p43o_note(reason, observed_global, stats) ((void)0)
#endif
#else
#define hz5_lowpage64_p43o_bucket_free(free_current, hot, warm, cold) \
  do {                                                               \
    *(hot) = 0;                                                      \
    *(warm) = 0;                                                     \
    *(cold) = 0;                                                     \
  } while (0)
#define hz5_lowpage64_p43o_source_admission_open(stats) (1)
#define hz5_lowpage64_p43o_projected_source_admission_open(stats, candidates) \
  (1)
#define hz5_lowpage64_p43o_note(reason, observed_global, stats) ((void)0)
#endif

#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN
typedef enum Hz5Lowpage64P43pReason {
  HZ5_LOWPAGE64_P43P_REASON_P40 = 1,
  HZ5_LOWPAGE64_P43P_REASON_SNAPSHOT = 2,
} Hz5Lowpage64P43pReason;

#if HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN
static void hz5_lowpage64_p43p_age_note(size_t admission_state,
                                        size_t release_candidates) {
  size_t b0 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket0, 0, memory_order_relaxed);
  size_t b1 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket1, 0, memory_order_relaxed);
  size_t b2 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket2, 0, memory_order_relaxed);
  size_t b3 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket3, 0, memory_order_relaxed);
  size_t b4 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket4, 0, memory_order_relaxed);
  size_t old = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_age_bucket_old, memory_order_relaxed);

  old += b4;
  b4 = b3;
  b3 = b2;
  b2 = b1;
  b1 = b0;
  b0 = release_candidates;

  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_stage1_items_total,
      release_candidates);

  size_t demote_old = 0;
  if (admission_state == HZ5_LOWPAGE64_P43O_STATE_OPEN) {
    demote_old = old < HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
                     ? old
                     : HZ5_LOWPAGE64_P43P_DEMOTE_BATCH;
    old -= demote_old;
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43p_age_would_demote_old_open,
        demote_old);
  } else if (admission_state == HZ5_LOWPAGE64_P43O_STATE_DRAIN) {
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43p_age_would_block_old_drain,
        old);
  } else if (admission_state == HZ5_LOWPAGE64_P43O_STATE_CLOSED) {
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43p_age_would_block_old_closed,
        old);
  }

  size_t young = b0 + b1;
  size_t warm = b2 + b3 + b4;
  size_t hot_pollution = warm + old;
  size_t young_reuse = young < HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
                           ? young
                           : HZ5_LOWPAGE64_P43P_DEMOTE_BATCH;

  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_would_reuse_young,
      young_reuse);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_would_keep_old,
      old);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_hot_reinject_avoided,
      hot_pollution);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_pop_all_pollution_projection,
      hot_pollution);

  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket0, b0,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket1, b1,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket2, b2,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket3, b3,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket4, b4,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket_old, old,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_young_current,
                        young, memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_old_current,
                        old, memory_order_relaxed);
}
#else
#define hz5_lowpage64_p43p_age_note(admission_state, release_candidates) \
  ((void)0)
#endif

static void hz5_lowpage64_p43p_note(
    Hz5Lowpage64P43pReason reason,
    size_t observed_global,
    const Hz5Lowpage64P43StatsSnapshot* maybe_stats) {
  Hz5Lowpage64P43StatsSnapshot local_stats = {0};
  const Hz5Lowpage64P43StatsSnapshot* stats = maybe_stats;
  if (!stats) {
    hz5_lowpage64_p43_stats_snapshot(&local_stats);
    stats = &local_stats;
  }

  size_t free_current = stats->slots_committed_free_current;
  atomic_store_explicit(&g_hz5_lowpage64_p43p_p43_committed_free_current,
                        free_current, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43p_p43_committed_free_max,
                          free_current);

  size_t old_state = atomic_load_explicit(
      &g_hz5_lowpage64_p43o_runtime_state, memory_order_relaxed);
  size_t new_state = hz5_lowpage64_p43o_choose_state(
      old_state, free_current, &g_hz5_lowpage64_p43o_runtime_cooldown);
  if (new_state != old_state) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_admission_flips, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_age_admission_flips, 1);
  }
  atomic_store_explicit(&g_hz5_lowpage64_p43o_runtime_state, new_state,
                        memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_epoch, 1);

  switch (new_state) {
    case HZ5_LOWPAGE64_P43O_STATE_OPEN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_state_open, 1);
      break;
    case HZ5_LOWPAGE64_P43O_STATE_DRAIN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_state_drain, 1);
      break;
    case HZ5_LOWPAGE64_P43O_STATE_CLOSED:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_state_closed, 1);
      break;
  }

  if (reason == HZ5_LOWPAGE64_P43P_REASON_SNAPSHOT) {
    hz5_lowpage64_p43p_age_note(new_state, 0);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_reason_snapshot, 1);
    return;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_reason_p40, 1);
  size_t release_candidates =
      hz5_lowpage64_p43o_release_candidates(observed_global);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_p40_release_candidates,
                          release_candidates);
  hz5_lowpage64_p43p_age_note(new_state, release_candidates);
  if (release_candidates == 0) {
    return;
  }

  size_t old_bridge_cold = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
      memory_order_relaxed);
  size_t bridge_cold = old_bridge_cold + release_candidates;
  size_t demote = 0;
  if (new_state == HZ5_LOWPAGE64_P43O_STATE_OPEN) {
    demote = bridge_cold < HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
                 ? bridge_cold
                 : HZ5_LOWPAGE64_P43P_DEMOTE_BATCH;
    bridge_cold -= demote;
  }

  size_t demote_from_new =
      demote > old_bridge_cold ? demote - old_bridge_cold : 0;
  size_t keep_from_new =
      release_candidates > demote_from_new
          ? release_candidates - demote_from_new
          : 0;

  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_would_stage1_bridge_cold,
      release_candidates);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_would_demote_to_source,
                          demote);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_would_keep_bridge,
                          keep_from_new);
  atomic_store_explicit(
      &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
      bridge_cold, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(
      g_hz5_lowpage64_p43p_bridge_cold_projected_max,
      bridge_cold);
}
#else
#define hz5_lowpage64_p43p_note(reason, observed_global, stats) ((void)0)
#endif

#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
typedef enum Hz5Lowpage64P44Reason {
  HZ5_LOWPAGE64_P44_REASON_PUBLISH = 1,
  HZ5_LOWPAGE64_P44_REASON_ACQUIRE_MISS = 2,
  HZ5_LOWPAGE64_P44_REASON_P40 = 3,
  HZ5_LOWPAGE64_P44_REASON_SNAPSHOT = 4,
} Hz5Lowpage64P44Reason;

static void hz5_lowpage64_p44_count_pair(_Atomic size_t* observed_total,
                                         _Atomic size_t* observed_max,
                                         _Atomic size_t* candidate_total,
                                         _Atomic size_t* candidate_max,
                                         size_t observed,
                                         size_t candidate) {
  HZ5_LOWPAGE64_COUNT_ADD(*observed_total, observed);
  HZ5_LOWPAGE64_COUNT_MAX(*observed_max, observed);
  HZ5_LOWPAGE64_COUNT_ADD(*candidate_total, candidate);
  HZ5_LOWPAGE64_COUNT_MAX(*candidate_max, candidate);
}

static void hz5_lowpage64_p44_note(Hz5Lowpage64P44Reason reason) {
  Hz5Lowpage64P43StatsSnapshot p43_stats = {0};
  hz5_lowpage64_p43_stats_snapshot(&p43_stats);

  size_t global = atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                                       memory_order_acquire);
  size_t stash = g_hz5_lowpage64_stash_count;
  size_t relbuf = g_hz5_lowpage64_relbuf_count;
  size_t p43_free = p43_stats.slots_committed_free_current;

  size_t global_candidate = hz5_lowpage64_p44_candidate_value(
      global, HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP);
  size_t stash_candidate = hz5_lowpage64_p44_candidate_value(
      stash, HZ5_LOWPAGE64_P44_STASH_SOFT_CAP);
  size_t relbuf_candidate = hz5_lowpage64_p44_candidate_value(
      relbuf, HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP);
  size_t p43_candidate = hz5_lowpage64_p44_candidate_value(
      p43_free, HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP);
  size_t total_candidate = global_candidate + stash_candidate +
                           relbuf_candidate + p43_candidate;

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_checkpoint_calls, 1);
  switch (reason) {
    case HZ5_LOWPAGE64_P44_REASON_PUBLISH:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_publish, 1);
      break;
    case HZ5_LOWPAGE64_P44_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_acquire_miss, 1);
      break;
    case HZ5_LOWPAGE64_P44_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_p40, 1);
      break;
    case HZ5_LOWPAGE64_P44_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_snapshot, 1);
      break;
  }

  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_global_observed_total,
      &g_hz5_lowpage64_p44_global_observed_max,
      &g_hz5_lowpage64_p44_global_candidate_total,
      &g_hz5_lowpage64_p44_global_candidate_max, global, global_candidate);
  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_stash_observed_total,
      &g_hz5_lowpage64_p44_stash_observed_max,
      &g_hz5_lowpage64_p44_stash_candidate_total,
      &g_hz5_lowpage64_p44_stash_candidate_max, stash, stash_candidate);
  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_relbuf_observed_total,
      &g_hz5_lowpage64_p44_relbuf_observed_max,
      &g_hz5_lowpage64_p44_relbuf_candidate_total,
      &g_hz5_lowpage64_p44_relbuf_candidate_max, relbuf, relbuf_candidate);
  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_p43_free_observed_total,
      &g_hz5_lowpage64_p44_p43_free_observed_max,
      &g_hz5_lowpage64_p44_p43_free_candidate_total,
      &g_hz5_lowpage64_p44_p43_free_candidate_max, p43_free,
      p43_candidate);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_candidate_total,
                          total_candidate);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p44_candidate_max,
                          total_candidate);
}
#else
#define hz5_lowpage64_p44_note(reason) ((void)0)
#endif

#if HZ5_LOWPAGE64_P45_ENABLED
typedef enum Hz5Lowpage64P45Reason {
  HZ5_LOWPAGE64_P45_REASON_PUBLISH = 1,
  HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS = 2,
  HZ5_LOWPAGE64_P45_REASON_P40 = 3,
  HZ5_LOWPAGE64_P45_REASON_SNAPSHOT = 4,
} Hz5Lowpage64P45Reason;

typedef struct Hz5Lowpage64P45Decision {
  size_t demote_intent;
  size_t bridge_current;
  size_t bridge_residual;
  int state_blocks;
  int bridge_blocks;
  int bridge_excess_blocks;
  size_t state;
} Hz5Lowpage64P45Decision;

static size_t hz5_lowpage64_p45_bridge_current(size_t observed_global) {
  return observed_global + g_hz5_lowpage64_stash_count +
         g_hz5_lowpage64_relbuf_count;
}

static Hz5Lowpage64P45Decision hz5_lowpage64_p45_decide(
    size_t observed_global,
    size_t state) {
  Hz5Lowpage64P45Decision decision;
  decision.demote_intent =
      hz5_lowpage64_p43o_release_candidates(observed_global);
  decision.bridge_current =
      hz5_lowpage64_p45_bridge_current(observed_global);
  decision.bridge_residual =
      decision.bridge_current > decision.demote_intent
          ? decision.bridge_current - decision.demote_intent
          : 0;
  decision.state_blocks = state != HZ5_LOWPAGE64_P43O_STATE_OPEN;
  decision.bridge_blocks = decision.bridge_current > 0;
  decision.bridge_excess_blocks =
      decision.bridge_residual > HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP;
  decision.state = state;
  return decision;
}

static void hz5_lowpage64_p45_note(
    Hz5Lowpage64P45Reason reason,
    size_t observed_global,
    const Hz5Lowpage64P43StatsSnapshot* maybe_stats) {
  Hz5Lowpage64P43StatsSnapshot local_stats = {0};
  const Hz5Lowpage64P43StatsSnapshot* stats = maybe_stats;
  if (!stats) {
    hz5_lowpage64_p43_stats_snapshot(&local_stats);
    stats = &local_stats;
  }

  size_t source_free = stats->slots_committed_free_current;
  size_t source_committed = stats->slots_committed_current;
  size_t old_state = atomic_load_explicit(
      &g_hz5_lowpage64_p45_runtime_state, memory_order_relaxed);
  size_t new_state = hz5_lowpage64_p43o_choose_state(
      old_state, source_free, &g_hz5_lowpage64_p45_runtime_cooldown);
  if (new_state != old_state) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_admission_flips, 1);
  }
  atomic_store_explicit(&g_hz5_lowpage64_p45_runtime_state, new_state,
                        memory_order_relaxed);

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_epoch, 1);
  switch (new_state) {
    case HZ5_LOWPAGE64_P43O_STATE_OPEN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_state_open, 1);
      break;
    case HZ5_LOWPAGE64_P43O_STATE_DRAIN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_state_drain, 1);
      break;
    case HZ5_LOWPAGE64_P43O_STATE_CLOSED:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_state_closed, 1);
      break;
  }

  switch (reason) {
    case HZ5_LOWPAGE64_P45_REASON_PUBLISH:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_publish, 1);
      break;
    case HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_acquire_miss, 1);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_source_miss, 1);
      break;
    case HZ5_LOWPAGE64_P45_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_p40, 1);
      break;
    case HZ5_LOWPAGE64_P45_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_snapshot, 1);
      break;
  }

  Hz5Lowpage64P45Decision decision =
      hz5_lowpage64_p45_decide(observed_global, new_state);

  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45_bridge_current_max,
                          decision.bridge_current);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_bridge_pressure_total,
                          decision.bridge_current);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45_source_free_current_max,
                          source_free);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_source_free_pressure_total,
                          source_free);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45_source_committed_current_max,
                          source_committed);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p45_source_committed_pressure_total,
      source_committed);

  if (reason == HZ5_LOWPAGE64_P45_REASON_P40) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_p40_demote_intent,
                            decision.demote_intent);
    if (decision.demote_intent > 0) {
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45_bridge_residual_after_demote_max,
          decision.bridge_residual);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_bridge_residual_after_demote_total,
          decision.bridge_residual);
    }
    if (decision.demote_intent > 0 && !decision.state_blocks &&
        !decision.bridge_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_demote_clean_open,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 && !decision.state_blocks &&
        !decision.bridge_excess_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_demote_open_bridge_soft,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 &&
        (decision.state_blocks || decision.bridge_blocks)) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_any,
                              decision.demote_intent);
    }
    if (decision.demote_intent > 0 &&
        (decision.state_blocks || decision.bridge_excess_blocks)) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_refined_any,
                              decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.state_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_p40_demote_while_drain_closed,
          decision.demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_state,
                              decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.bridge_excess_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_stage1_bridge_excess,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.bridge_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_p40_demote_while_bridge_nonempty,
          decision.demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_stage1_bridge_nonempty,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.state_blocks &&
        decision.bridge_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_overlap,
                              decision.demote_intent);
    }
  }
}
#else
#define hz5_lowpage64_p45_note(reason, observed_global, stats) ((void)0)
#endif

#if HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN
static void hz5_lowpage64_p45dr_scan_stage1(size_t* bucket0,
                                            size_t* bucket1,
                                            size_t* bucket2_4,
                                            size_t* bucket_old,
                                            size_t* oldest_age) {
  *bucket0 = 0;
  *bucket1 = 0;
  *bucket2_4 = 0;
  *bucket_old = 0;
  *oldest_age = 0;
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  uint32_t current_epoch = atomic_load_explicit(
      &g_hz5_lowpage64_p40_global_epoch, memory_order_relaxed);
  void* node = (void*)atomic_load_explicit(
      &g_hz5_lowpage64_p43p_stage1_head, memory_order_acquire);
  while (node) {
    uint32_t node_epoch = hz5_lowpage64_node_epoch_load(node);
    size_t age = current_epoch >= node_epoch
                     ? (size_t)(current_epoch - node_epoch)
                     : 0;
    if (age == 0) {
      (*bucket0)++;
    } else if (age == 1) {
      (*bucket1)++;
    } else if (age <= 4u) {
      (*bucket2_4)++;
    } else {
      (*bucket_old)++;
    }
    if (age > *oldest_age) {
      *oldest_age = age;
    }
    node = hz5_lowpage64_link_next_load(node);
  }
#endif
}

static void hz5_lowpage64_p45dr_note(Hz5Lowpage64P45Reason reason) {
  size_t current = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_stage1_count, memory_order_acquire);
  size_t hot_bridge = hz5_lowpage64_p45_bridge_current(
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire));
  size_t total_bridge = hot_bridge + current;
  size_t state = atomic_load_explicit(&g_hz5_lowpage64_p45_runtime_state,
                                      memory_order_relaxed);

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_checkpoint_calls, 1);
  switch (reason) {
    case HZ5_LOWPAGE64_P45_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_reason_p40, 1);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint, current);
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint_max, current);
      break;
    case HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_reason_acquire_miss, 1);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss, current);
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss_max, current);
      break;
    case HZ5_LOWPAGE64_P45_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_reason_snapshot, 1);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45dr_stage1_current_at_snapshot, current);
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45dr_stage1_current_at_snapshot_max, current);
      break;
    case HZ5_LOWPAGE64_P45_REASON_PUBLISH:
      break;
  }

  size_t b0 = 0;
  size_t b1 = 0;
  size_t b2_4 = 0;
  size_t old = 0;
  size_t oldest = 0;
  hz5_lowpage64_p45dr_scan_stage1(&b0, &b1, &b2_4, &old, &oldest);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_bucket0, b0);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_bucket1, b1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_bucket2_4,
                          b2_4);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_old, old);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_stage1_old_current_max,
                          old);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_stage1_oldest_age_max,
                          oldest);

  size_t drain_to_bridge = 0;
  size_t keep_cold = current;
  size_t demote_open = 0;
  size_t block_state = 0;
  size_t block_bridge = 0;
  if (state != HZ5_LOWPAGE64_P43O_STATE_OPEN) {
    block_state = current;
  } else if (hot_bridge < HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    size_t bridge_capacity = HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP - hot_bridge;
    drain_to_bridge = current < bridge_capacity ? current : bridge_capacity;
    size_t remaining = current - drain_to_bridge;
    demote_open = old < remaining ? old : remaining;
    keep_cold = remaining - demote_open;
  } else {
    block_bridge = current;
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_would_drain_to_bridge,
                          drain_to_bridge);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_would_keep_cold,
                          keep_cold);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_would_demote_open,
                          demote_open);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p45dr_would_block_drain_closed, block_state);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p45dr_would_block_bridge_excess, block_bridge);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_bridge_current_hot_max,
                          hot_bridge);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_bridge_cold_current_max,
                          current);
  HZ5_LOWPAGE64_COUNT_MAX(
      g_hz5_lowpage64_p45dr_bridge_total_with_cold_max, total_bridge);

  size_t enqueue_nodes = atomic_load_explicit(
      &g_hz5_lowpage64_p45rg_stage1_enqueue_nodes, memory_order_relaxed);
  size_t acquire_nodes = atomic_load_explicit(
      &g_hz5_lowpage64_p45rg_stage1_acquire_nodes, memory_order_relaxed);
  size_t expected = enqueue_nodes > acquire_nodes
                        ? enqueue_nodes - acquire_nodes
                        : 0;
  size_t mismatch = expected > current ? expected - current : current - expected;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_balance_expected,
                          expected);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_balance_actual,
                          current);
  if (mismatch > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_balance_mismatch, 1);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_balance_mismatch_max,
                            mismatch);
  }
}
#else
#define hz5_lowpage64_p45dr_note(reason) ((void)0)
#endif

static size_t hz5_lowpage64_free_list(void* head) {
  size_t freed = 0;
  while (head) {
    void* next = hz5_lowpage64_link_next_load(head);
    hz5_lowpage64_raw_os_release(head);
    freed++;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_overflow_frees, 1);
    head = next;
  }
  return freed;
}

static size_t hz5_lowpage64_free_list_global_cap(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_cap_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_cap_release_nodes,
                            freed);
  }
  return freed;
}

static size_t hz5_lowpage64_free_list_stash_trim(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_stash_trim_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_stash_trim_release_nodes,
                            freed);
  }
  return freed;
}

static void hz5_lowpage64_count_trim(size_t freed) {
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_freed_nodes, freed);
  }
}

#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_global_push_list(void* head,
                                           void* tail,
                                           size_t count) {
  if (!head || !tail || count == 0) {
    return;
  }

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_global_batch_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_lowpage64_global_batch_count, count,
                            memory_order_relaxed);
}

#if HZ5_LOWPAGE64_STAGE1_ENABLED
static void hz5_lowpage64_p43p_stage1_push_list_impl(void* head,
                                                     void* tail,
                                                     size_t count,
                                                     int count_enqueue) {
  if (!head || !tail || count == 0) {
    return;
  }

#if HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN && \
    HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  if (count_enqueue) {
    uint32_t stage1_epoch = atomic_load_explicit(
        &g_hz5_lowpage64_p40_global_epoch, memory_order_relaxed);
    hz5_lowpage64_mark_list_epoch(head, stage1_epoch);
  }
#endif

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_stage1_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_p43p_stage1_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  size_t previous = atomic_fetch_add_explicit(
      &g_hz5_lowpage64_p43p_stage1_count, count, memory_order_relaxed);
#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1
  if (count_enqueue) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_enqueue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_enqueue_nodes, count);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_requeue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_requeue_nodes, count);
  }
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43p_stage1_current_max,
                          previous + count);
#endif
#if HZ5_LOWPAGE64_P45_REFINED_GATE
  if (count_enqueue) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_enqueue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_enqueue_nodes,
                            count);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_requeue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_requeue_nodes,
                            count);
  }
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45rg_stage1_current_max,
                          previous + count);
#endif
  (void)previous;
}

static void hz5_lowpage64_p43p_stage1_push_list(void* head,
                                                void* tail,
                                                size_t count) {
  hz5_lowpage64_p43p_stage1_push_list_impl(head, tail, count, 1);
}

static void* hz5_lowpage64_p43p_stage1_pop_batch(size_t limit,
                                                 size_t* out_count) {
  void* head =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_p43p_stage1_head, 0,
                                      memory_order_acq_rel);
  size_t count = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_stage1_count, 0, memory_order_acq_rel);
  if (!head) {
    count = 0;
  }
  if (out_count) {
    *out_count = count;
  }
  if (!head || limit == 0 || count <= limit) {
    return head;
  }

  void* batch_tail = head;
  size_t batch_count = 1;
  while (batch_count < limit && hz5_lowpage64_link_next_load(batch_tail)) {
    batch_tail = hz5_lowpage64_link_next_load(batch_tail);
    batch_count++;
  }

  void* rest_head = hz5_lowpage64_link_next_load(batch_tail);
  hz5_lowpage64_link_next(batch_tail, NULL);
  size_t rest_count = count - batch_count;
  if (rest_head) {
    void* rest_tail = rest_head;
    while (hz5_lowpage64_link_next_load(rest_tail)) {
      rest_tail = hz5_lowpage64_link_next_load(rest_tail);
    }
    hz5_lowpage64_p43p_stage1_push_list_impl(rest_head, rest_tail,
                                             rest_count, 0);
  }
  if (out_count) {
    *out_count = batch_count;
  }
  return head;
}
#endif

static size_t hz5_lowpage64_free_list_p40_global(void* head,
                                                 size_t age_nodes,
                                                 size_t hard_nodes) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_nodes, freed);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_age_nodes,
                            age_nodes);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_hard_nodes,
                            hard_nodes);
  }
  return freed;
}

static void hz5_lowpage64_p40_checkpoint(void) {
  // P44 observes candidate state at slow/checkpoint boundaries only. It must
  // not turn this P40 checkpoint into an actual trim policy by accident.
  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_P40);

  size_t observed = atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                                         memory_order_acquire);
  if (observed <= HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    return;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_checkpoint_calls, 1);
  Hz5Lowpage64P43StatsSnapshot p43_stats = {0};
  hz5_lowpage64_p43_stats_snapshot(&p43_stats);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_p43_free_before_total,
                          p43_stats.slots_committed_free_current);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p40_p43_free_before_max,
                          p43_stats.slots_committed_free_current);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_p43_committed_before_total,
                          p43_stats.slots_committed_current);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p40_p43_committed_before_max,
                          p43_stats.slots_committed_current);
  hz5_lowpage64_p43o_note(HZ5_LOWPAGE64_P43O_REASON_P40, observed,
                          &p43_stats);
  hz5_lowpage64_p43p_note(HZ5_LOWPAGE64_P43P_REASON_P40, observed,
                          &p43_stats);
  hz5_lowpage64_p45_note(HZ5_LOWPAGE64_P45_REASON_P40, observed,
                         &p43_stats);
#if HZ5_LOWPAGE64_P43O_PROJECTED_GATE
  size_t p43o_release_candidates =
      hz5_lowpage64_p43o_release_candidates(observed);
  if (!hz5_lowpage64_p43o_projected_source_admission_open(
          &p43_stats, p43o_release_candidates)) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_nodes,
                            p43_stats.slots_committed_free_current);
    return;
  }
#endif
#if HZ5_LOWPAGE64_P43O_ADMISSION_GATE
  if (!hz5_lowpage64_p43o_source_admission_open(&p43_stats)) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_nodes,
                            p43_stats.slots_committed_free_current);
    return;
  }
#endif
#if HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP > 0
  if (p43_stats.slots_committed_free_current >=
      HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_nodes,
                            p43_stats.slots_committed_free_current);
    return;
  }
#endif
  uint32_t epoch = atomic_fetch_add_explicit(
                       &g_hz5_lowpage64_p40_global_epoch, 1,
                       memory_order_relaxed) +
                   1u;

  void* head =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_global_batch_head, 0,
                                      memory_order_acq_rel);
  size_t expected_count = atomic_exchange_explicit(
      &g_hz5_lowpage64_global_batch_count, 0, memory_order_acq_rel);
  (void)expected_count;
  if (!head) {
    return;
  }

  void* keep_head = NULL;
  void* keep_tail = NULL;
  size_t keep_count = 0;
  void* release_head = NULL;
  void* release_tail = NULL;
  size_t release_count = 0;
  size_t age_nodes = 0;
  size_t hard_nodes = 0;

  void* node = head;
  while (node) {
    void* next = hz5_lowpage64_link_next_load(node);
    uint32_t node_epoch = hz5_lowpage64_node_epoch_load(node);
    uint32_t age = epoch - node_epoch;
    int over_hard =
#if HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP > 0
        observed > HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP;
#else
        0;
#endif
    int old_enough = age >= HZ5_LOWPAGE64_P40_TTL_EPOCHS;
    int should_release =
        release_count < HZ5_LOWPAGE64_P40_RELEASE_BATCH &&
        observed - release_count > HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP &&
        (old_enough || over_hard);

    if (should_release) {
      hz5_lowpage64_link_next(node, NULL);
      if (!release_head) {
        release_head = node;
      } else {
        hz5_lowpage64_link_next(release_tail, node);
      }
      release_tail = node;
      release_count++;
      if (over_hard && !old_enough) {
        hard_nodes++;
      } else {
        age_nodes++;
      }
    } else {
      hz5_lowpage64_link_next(node, NULL);
      if (!keep_head) {
        keep_head = node;
      } else {
        hz5_lowpage64_link_next(keep_tail, node);
      }
      keep_tail = node;
      keep_count++;
    }

    node = next;
  }

  if (keep_head) {
    hz5_lowpage64_global_push_list(keep_head, keep_tail, keep_count);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_keep_nodes, keep_count);
  }
#if HZ5_LOWPAGE64_STAGE1_ENABLED
#if HZ5_LOWPAGE64_P45_REFINED_GATE
  size_t p45_state = atomic_load_explicit(
      &g_hz5_lowpage64_p45_runtime_state, memory_order_relaxed);
  Hz5Lowpage64P45Decision p45_decision =
      hz5_lowpage64_p45_decide(observed, p45_state);
  int p45_stage1 =
      p45_decision.state_blocks || p45_decision.bridge_excess_blocks;
  if (release_count > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_demote_intent,
                            release_count);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45rg_bridge_residual_max,
                            p45_decision.bridge_residual);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_bridge_residual_total,
                            p45_decision.bridge_residual);
    if (p45_stage1) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_any,
                              release_count);
      if (p45_decision.state_blocks) {
        HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_state,
                                release_count);
      }
      if (p45_decision.bridge_excess_blocks) {
        HZ5_LOWPAGE64_COUNT_ADD(
            g_hz5_lowpage64_p45rg_stage1_bridge_excess, release_count);
      }
      if (p45_decision.state_blocks && p45_decision.bridge_excess_blocks) {
        HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_overlap,
                                release_count);
      }
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45rg_allowed_open_bridge_soft, release_count);
    }
  }
  if (p45_stage1) {
    hz5_lowpage64_p43p_stage1_push_list(release_head, release_tail,
                                        release_count);
    (void)age_nodes;
    (void)hard_nodes;
  } else {
    hz5_lowpage64_count_trim(
        hz5_lowpage64_free_list_p40_global(release_head, age_nodes,
                                           hard_nodes));
  }
#else
  hz5_lowpage64_p43p_stage1_push_list(release_head, release_tail,
                                      release_count);
  (void)age_nodes;
  (void)hard_nodes;
#endif
#else
  hz5_lowpage64_count_trim(
      hz5_lowpage64_free_list_p40_global(release_head, age_nodes,
                                         hard_nodes));
#endif
  hz5_lowpage64_p45dr_note(HZ5_LOWPAGE64_P45_REASON_P40);
}
#else
#define hz5_lowpage64_p40_checkpoint() ((void)0)
#define hz5_lowpage64_mark_list_epoch(head, epoch) ((void)0)
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
static size_t hz5_lowpage64_free_list_p37_overflow(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_p37_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_p37_release_nodes, freed);
  }
  return freed;
}

static void hz5_lowpage64_p37_release_overflow(int high_event) {
  void* head = g_hz5_lowpage64_overflow_head;
  size_t count = g_hz5_lowpage64_overflow_count;
  g_hz5_lowpage64_overflow_head = NULL;
  g_hz5_lowpage64_overflow_count = 0;
  g_hz5_lowpage64_high_event_streak = 0;
  if (!head) {
    return;
  }

  size_t freed = hz5_lowpage64_free_list_p37_overflow(head);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_release_calls, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_release_nodes,
                          freed);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_calls, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_freed_nodes, freed);
  if (high_event) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_high_event_releases, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_lifetime_releases, 1);
  }
  (void)freed;
  (void)count;
}

static void hz5_lowpage64_p37_checkpoint(void) {
  g_hz5_lowpage64_epoch++;
  if (g_hz5_lowpage64_overflow_count == 0) {
    g_hz5_lowpage64_high_event_streak = 0;
    return;
  }

#if HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME > 0
  if (g_hz5_lowpage64_epoch >
      g_hz5_lowpage64_overflow_epoch +
          (uint32_t)HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME) {
    hz5_lowpage64_p37_release_overflow(0);
    return;
  }
#endif

#if HZ5_LOWPAGE64_P37_HIGH_STREAK > 0
  if (g_hz5_lowpage64_stash_count >= HZ5_LOWPAGE64_P37_HOT_CAP &&
      g_hz5_lowpage64_overflow_count >= HZ5_LOWPAGE64_P37_HIGH_WATER) {
    if (g_hz5_lowpage64_high_event_streak < UINT8_MAX) {
      g_hz5_lowpage64_high_event_streak++;
    }
    if (g_hz5_lowpage64_high_event_streak >=
        HZ5_LOWPAGE64_P37_HIGH_STREAK) {
      hz5_lowpage64_p37_release_overflow(1);
      return;
    }
  } else {
    g_hz5_lowpage64_high_event_streak = 0;
  }
#endif
}
#else
#define hz5_lowpage64_p37_checkpoint() ((void)0)
#endif

#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
static size_t hz5_lowpage64_free_list_p39_probation(void* head,
                                                    int lifetime_release) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_release_nodes,
                            freed);
    if (lifetime_release) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p39_probation_lifetime_releases, 1);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p39_probation_replace_releases, 1);
    }
  }
  return freed;
}

static void hz5_lowpage64_p39_release_probation(int lifetime_release) {
  void* head = g_hz5_lowpage64_probation_head;
  g_hz5_lowpage64_probation_head = NULL;
  g_hz5_lowpage64_probation_count = 0;
  hz5_lowpage64_count_trim(
      hz5_lowpage64_free_list_p39_probation(head, lifetime_release));
}

static void hz5_lowpage64_p39_checkpoint(void) {
  g_hz5_lowpage64_p39_epoch++;
  if (!g_hz5_lowpage64_probation_head) {
    return;
  }

#if HZ5_LOWPAGE64_P39_PROBATION_LIFETIME > 0
  if (g_hz5_lowpage64_p39_epoch >
      g_hz5_lowpage64_probation_epoch +
          (uint32_t)HZ5_LOWPAGE64_P39_PROBATION_LIFETIME) {
    hz5_lowpage64_p39_release_probation(1);
  }
#endif
}

#if HZ5_LOWPAGE64_P39_USE_ON_MISS
static void* hz5_lowpage64_p39_pop_probation(void) {
  if (!g_hz5_lowpage64_probation_head) {
    return NULL;
  }
  void* raw = g_hz5_lowpage64_probation_head;
  g_hz5_lowpage64_probation_head = hz5_lowpage64_link_next_load(raw);
  g_hz5_lowpage64_probation_count--;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_hits, 1);
  return raw;
}
#endif
#else
#define hz5_lowpage64_p39_checkpoint() ((void)0)
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP == 0 && \
    HZ5_LOWPAGE64_P39_PROBATION_CAP == 0
static size_t hz5_lowpage64_stash_cap_for_batch(size_t acquired_count) {
#if HZ5_LOWPAGE64_STASH_CAP > 0
  (void)acquired_count;
  return HZ5_LOWPAGE64_STASH_CAP;
#elif HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP > 0
  if (acquired_count > HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_adaptive_trigger_calls, 1);
    return HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP;
  }
  return 0;
#else
  (void)acquired_count;
  return 0;
#endif
}
#endif

#if HZ5_LOWPAGE64_STATS
static void hz5_lowpage64_count_acquired_bucket(size_t count) {
  if (count <= 64u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le64, 1);
  } else if (count <= 128u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le128, 1);
  } else if (count <= 192u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le192, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_gt192, 1);
  }
}

static void hz5_lowpage64_count_stash_bucket(size_t count) {
  if (count <= 64u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le64, 1);
  } else if (count <= 128u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le128, 1);
  } else if (count <= 192u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le192, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_gt192, 1);
  }
}
#else
#define hz5_lowpage64_count_acquired_bucket(count) ((void)0)
#define hz5_lowpage64_count_stash_bucket(count) ((void)0)
#endif

static void hz5_lowpage64_set_stash_from_list(void* head,
                                              size_t acquired_count) {
#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
  (void)acquired_count;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);

  if (g_hz5_lowpage64_probation_head) {
    hz5_lowpage64_p39_release_probation(0);
  }

  void* hot_head = head;
  void* hot_tail = NULL;
  size_t hot_kept = 0;
  void* node = head;
  while (node && hot_kept < HZ5_LOWPAGE64_P39_HOT_CAP) {
    hot_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    hot_kept++;
  }
  if (hot_tail) {
    hz5_lowpage64_link_next(hot_tail, NULL);
  }

  void* probation_head = node;
  void* probation_tail = NULL;
  size_t probation_kept = 0;
  while (node && probation_kept < HZ5_LOWPAGE64_P39_PROBATION_CAP) {
    probation_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    probation_kept++;
  }
  if (probation_tail) {
    hz5_lowpage64_link_next(probation_tail, NULL);
  }

  g_hz5_lowpage64_stash_head = hot_head;
  g_hz5_lowpage64_stash_count = hot_kept;
  g_hz5_lowpage64_probation_head = probation_head;
  g_hz5_lowpage64_probation_count = probation_kept;
  if (probation_kept > 0) {
    g_hz5_lowpage64_probation_epoch = g_hz5_lowpage64_p39_epoch;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, hot_kept);
  hz5_lowpage64_count_stash_bucket(hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p39_probation_set_nodes_total, probation_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p39_probation_set_nodes_max,
                          probation_kept);

  hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
  return;
#elif HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
  (void)acquired_count;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);

  void* hot_head = head;
  void* hot_tail = NULL;
  size_t hot_kept = 0;
  void* node = head;
  while (node && hot_kept < HZ5_LOWPAGE64_P37_HOT_CAP) {
    hot_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    hot_kept++;
  }
  if (hot_tail) {
    hz5_lowpage64_link_next(hot_tail, NULL);
  }

  void* overflow_head = node;
  void* overflow_tail = NULL;
  size_t overflow_kept = 0;
  while (node && overflow_kept < HZ5_LOWPAGE64_P37_OVERFLOW_CAP) {
    overflow_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    overflow_kept++;
  }
  if (overflow_tail) {
    hz5_lowpage64_link_next(overflow_tail, NULL);
  }

  g_hz5_lowpage64_stash_head = hot_head;
  g_hz5_lowpage64_stash_count = hot_kept;
  g_hz5_lowpage64_overflow_head = overflow_head;
  g_hz5_lowpage64_overflow_count = overflow_kept;
  if (overflow_kept > 0) {
    g_hz5_lowpage64_overflow_epoch = g_hz5_lowpage64_epoch;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, hot_kept);
  hz5_lowpage64_count_stash_bucket(hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p37_overflow_set_nodes_total, overflow_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p37_overflow_set_nodes_max,
                          overflow_kept);

  hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
  return;
#else
  size_t cap = hz5_lowpage64_stash_cap_for_batch(acquired_count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);
  if (cap > 0) {
    void* keep_head = head;
    void* keep_tail = NULL;
    size_t kept = 0;
    void* node = head;
    while (node && kept < cap) {
      keep_tail = node;
      node = hz5_lowpage64_link_next_load(node);
      kept++;
    }
    if (keep_tail) {
      hz5_lowpage64_link_next(keep_tail, NULL);
    }
    g_hz5_lowpage64_stash_head = keep_head;
    g_hz5_lowpage64_stash_count = kept;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total, kept);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, kept);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total, kept);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, kept);
    hz5_lowpage64_count_stash_bucket(kept);
    hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
    return;
  }

  g_hz5_lowpage64_stash_head = head;
  g_hz5_lowpage64_stash_count = hz5_lowpage64_list_count(head);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max,
                          g_hz5_lowpage64_stash_count);
  hz5_lowpage64_count_stash_bucket(g_hz5_lowpage64_stash_count);
#endif
}

static void hz5_lowpage64_publish_relbuf(void) {
  if (!g_hz5_lowpage64_relbuf_head || !g_hz5_lowpage64_relbuf_tail ||
      g_hz5_lowpage64_relbuf_count == 0) {
    return;
  }

  void* head = g_hz5_lowpage64_relbuf_head;
  void* tail = g_hz5_lowpage64_relbuf_tail;
  size_t count = g_hz5_lowpage64_relbuf_count;
  g_hz5_lowpage64_relbuf_head = NULL;
  g_hz5_lowpage64_relbuf_tail = NULL;
  g_hz5_lowpage64_relbuf_count = 0;

  size_t global_count = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_count, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_relbuf_count_max, count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_global_count_before_max,
                          global_count);
  if (global_count + count > HZ5_LOWPAGE64_GLOBAL_CAP) {
    (void)hz5_lowpage64_free_list_global_cap(head);
    return;
  }

  uint32_t publish_epoch = 0;
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  publish_epoch = atomic_load_explicit(&g_hz5_lowpage64_p40_global_epoch,
                                       memory_order_relaxed);
  hz5_lowpage64_mark_list_epoch(head, publish_epoch);
#endif

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_global_batch_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_lowpage64_global_batch_count, count,
                            memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_global_count_after_max,
                          global_count + count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publishes, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publish_nodes, count);
  (void)publish_epoch;
  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_PUBLISH);
  hz5_lowpage64_p45_note(HZ5_LOWPAGE64_P45_REASON_PUBLISH,
                         global_count + count, NULL);
  hz5_lowpage64_p40_checkpoint();
}

void* hz5_lowpage64_acquire(size_t raw_bytes) {
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_alloc_calls, 1);

#if HZ5_LOWPAGE64_SPAN_CACHE
  if (g_hz5_lowpage64_span_head) {
    void* raw = g_hz5_lowpage64_span_head;
    g_hz5_lowpage64_span_head = hz5_lowpage64_link_next_load(raw);
    g_hz5_lowpage64_span_count--;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_span_hits, 1);
    return raw;
  }
#endif

  if (g_hz5_lowpage64_stash_head) {
    void* raw = g_hz5_lowpage64_stash_head;
    g_hz5_lowpage64_stash_head = hz5_lowpage64_link_next_load(raw);
    g_hz5_lowpage64_stash_count--;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_hot_hits, 1);
    return raw;
  }

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
  if (g_hz5_lowpage64_overflow_head) {
    void* raw = g_hz5_lowpage64_overflow_head;
    g_hz5_lowpage64_overflow_head = hz5_lowpage64_link_next_load(raw);
    g_hz5_lowpage64_overflow_count--;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_hits, 1);
    return raw;
  }
#endif

  hz5_lowpage64_p37_checkpoint();
  hz5_lowpage64_p39_checkpoint();

  void* global =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_global_batch_head, 0,
                                      memory_order_acq_rel);
  if (global) {
    size_t acquired_count = atomic_exchange_explicit(
        &g_hz5_lowpage64_global_batch_count, 0, memory_order_acq_rel);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_count_total,
                            acquired_count);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_acquired_count_max,
                            acquired_count);
    hz5_lowpage64_count_acquired_bucket(acquired_count);
    void* raw = global;
    void* stash = hz5_lowpage64_link_next_load(raw);
    hz5_lowpage64_link_next(raw, NULL);
    hz5_lowpage64_set_stash_from_list(stash, acquired_count);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_batch_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_first_hits, 1);
    return raw;
  }

#if HZ5_LOWPAGE64_STAGE1_ENABLED
  size_t bridge_cold_count = 0;
#if HZ5_LOWPAGE64_P45_REFINED_GATE
  size_t stage1_limit = HZ5_LOWPAGE64_P45_STAGE1_ACQUIRE_LIMIT;
#else
  size_t stage1_limit = HZ5_LOWPAGE64_P43P_STAGE1_ACQUIRE_LIMIT;
#endif
  void* bridge_cold =
      hz5_lowpage64_p43p_stage1_pop_batch(stage1_limit,
                                          &bridge_cold_count);
  if (bridge_cold) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_count_total,
                            bridge_cold_count);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_acquired_count_max,
                            bridge_cold_count);
    hz5_lowpage64_count_acquired_bucket(bridge_cold_count);
    void* raw = bridge_cold;
    void* stash = hz5_lowpage64_link_next_load(raw);
    hz5_lowpage64_link_next(raw, NULL);
    hz5_lowpage64_set_stash_from_list(stash, bridge_cold_count);
#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_acquire_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_acquire_nodes,
                            bridge_cold_count);
#endif
#if HZ5_LOWPAGE64_P45_REFINED_GATE
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_acquire_hits, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_acquire_nodes,
                            bridge_cold_count);
#endif
    hz5_lowpage64_p45dr_note(HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS);
    return raw;
  }
#endif

#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0 && HZ5_LOWPAGE64_P39_USE_ON_MISS
  void* probation = hz5_lowpage64_p39_pop_probation();
  if (probation) {
    return probation;
  }
#endif

#if HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD
  void* cold = hz5_lowpage64_p42_try_recommit(raw_bytes);
  if (cold) {
    return cold;
  }
#endif

  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_ACQUIRE_MISS);
  hz5_lowpage64_p43o_note(HZ5_LOWPAGE64_P43O_REASON_ACQUIRE_MISS,
                          atomic_load_explicit(
                              &g_hz5_lowpage64_global_batch_count,
                              memory_order_acquire),
                          NULL);
  hz5_lowpage64_p45_note(
      HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      NULL);
  hz5_lowpage64_p45dr_note(HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_os_allocs, 1);
  return hz5_lowpage64_raw_os_alloc(raw_bytes);
}

static void hz5_lowpage64_release_common(void* raw) {
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_free_calls, 1);
  if (!raw) {
    return;
  }

#if HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS
  // P43b.1 dry run: inactive slot ownership is kept in the segment
  // descriptor. Do not enqueue freed slots into data-page intrusive lists.
  hz5_lowpage64_raw_os_release(raw);
  return;
#endif

#if HZ5_LOWPAGE64_SPAN_CACHE
  if (g_hz5_lowpage64_span_count < HZ5_LOWPAGE64_SPAN_CAP) {
    hz5_lowpage64_link_next(raw, g_hz5_lowpage64_span_head);
    g_hz5_lowpage64_span_head = raw;
    g_hz5_lowpage64_span_count++;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_span_pushes, 1);
    return;
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_span_full, 1);
#endif

  // This relbuf -> global batch -> acquired stash path is the "P25 bridge" used
  // by P25a/P33 and intentionally preserved by P43i/P44 balanced lanes.
  hz5_lowpage64_link_next(raw, NULL);
  if (!g_hz5_lowpage64_relbuf_head) {
    g_hz5_lowpage64_relbuf_head = raw;
    g_hz5_lowpage64_relbuf_tail = raw;
  } else {
    hz5_lowpage64_link_next(g_hz5_lowpage64_relbuf_tail, raw);
    g_hz5_lowpage64_relbuf_tail = raw;
  }
  g_hz5_lowpage64_relbuf_count++;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_relbuf_pushes, 1);

  if (g_hz5_lowpage64_relbuf_count >= HZ5_LOWPAGE64_BATCH_FLUSH_N) {
    hz5_lowpage64_publish_relbuf();
  }
}

void hz5_lowpage64_release(void* raw) {
  hz5_lowpage64_release_common(raw);
}

int hz5_lowpage64_release_prepared(const Hz5Lowpage64FreeCtx* ctx,
                                   void* raw) {
#if HZ5_LOWPAGE64_P43_PREPARED_ANY
  if (!ctx || ctx->lookup_kind != HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE ||
      !raw) {
    return 0;
  }
  if (ctx->slot_base && ctx->slot_base != raw) {
    return 0;
  }
  hz5_lowpage64_p43g_note_prepared_path();
#if HZ5_LOWPAGE64_P43_DIRECT_PREPARED_RELEASE
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_free_calls, 1);
  if (hz5_lowpage64_p43_release_prepared_slot(ctx)) {
    return 1;
  }
  hz5_lowpage64_raw_os_release(raw);
  return 1;
#else
  // PreparedBridge spends the active lookup result, then deliberately returns
  // to release_common() so P43i keeps the P25 hot batching topology.
  hz5_lowpage64_release_common(raw);
  return 1;
#endif
#else
  (void)ctx;
  (void)raw;
  return 0;
#endif
}

#if HZ5_LOWPAGE64_STATS
void hz5_lowpage64_print_snapshot(const char* label) {
  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_SNAPSHOT);

  Hz5Lowpage64P43StatsSnapshot p43_stats = {0};
  hz5_lowpage64_p43_stats_snapshot(&p43_stats);
  hz5_lowpage64_p43o_note(
      HZ5_LOWPAGE64_P43O_REASON_SNAPSHOT,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      &p43_stats);
  hz5_lowpage64_p43p_note(
      HZ5_LOWPAGE64_P43P_REASON_SNAPSHOT,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      &p43_stats);
  hz5_lowpage64_p45_note(
      HZ5_LOWPAGE64_P45_REASON_SNAPSHOT,
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire),
      &p43_stats);
  hz5_lowpage64_p45dr_note(HZ5_LOWPAGE64_P45_REASON_SNAPSHOT);
  size_t p44_global_current = hz5_lowpage64_p44_candidate_value(
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_relaxed),
      HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP);
  size_t p44_stash_current = hz5_lowpage64_p44_candidate_value(
      g_hz5_lowpage64_stash_count, HZ5_LOWPAGE64_P44_STASH_SOFT_CAP);
  size_t p44_relbuf_current = hz5_lowpage64_p44_candidate_value(
      g_hz5_lowpage64_relbuf_count, HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP);
  size_t p44_p43_free_current = hz5_lowpage64_p44_candidate_value(
      p43_stats.slots_committed_free_current,
      HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP);
  size_t p44_current_total = p44_global_current + p44_stash_current +
                             p44_relbuf_current + p44_p43_free_current;
#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
  size_t p43o_hot_current = 0;
  size_t p43o_warm_current = 0;
  size_t p43o_cold_current = 0;
  hz5_lowpage64_p43o_bucket_free(p43_stats.slots_committed_free_current,
                                 &p43o_hot_current, &p43o_warm_current,
                                 &p43o_cold_current);
#else
  size_t p43o_hot_current = 0;
  size_t p43o_warm_current = 0;
  size_t p43o_cold_current = 0;
#endif

  fprintf(stderr,
          "[HZ5_LOWPAGE64_SNAPSHOT] label=%s "
          "tls_stash_count=%zu relbuf_count=%zu "
          "global_batch_count=%zu acquired_count_max=%zu "
          "stash_set_nodes_max=%zu p43_segments_current=%zu "
          "p43_slots_total_current=%zu p43_slots_active_current=%zu "
          "p43_slots_committed_current=%zu "
          "p43_slots_committed_free_current=%zu "
          "p43_slots_global_retained_current=%zu "
          "p43_slots_tls_retained_current=%zu "
          "p43_slots_cold_current=%zu "
          "p43_slots_uncommitted_current=%zu "
          "p43_contract_lockless=%u p43_lockless_lookup=%u "
          "p43_slot_decommit=%u "
          "p43_page_noaccess=%u p43_runtime_segment_release=%u "
          "p43g_prepare_calls=%zu p43g_prepare_active=%zu "
          "p43g_prepare_nonactive=%zu p43g_prepare_miss=%zu "
          "p43g_source_p25=%zu p43g_source_other=%zu "
          "p43g_raw_mismatch=%zu "
          "p43g_release_old_path_calls=%zu "
          "p43g_release_prepared_calls=%zu "
          "p43_segments_reserved=%zu p43_segments_released=%zu "
          "p43_slot_commits=%zu p43_slot_decommits=%zu "
          "p43_tls_hits=%zu p43_global_hits=%zu "
          "p43_free_slot_hits=%zu p43_source_alloc_calls=%zu "
          "p43_source_cold_segment_scans_total=%zu "
          "p43_source_cold_segment_scans_max=%zu "
          "p43_source_free_segment_scans_total=%zu "
          "p43_source_free_segment_scans_max=%zu "
          "p43_source_new_segment_scan_total=%zu "
          "p43_source_new_segment_scan_max=%zu p43_lookup_calls=%zu "
          "p43_lookup_active=%zu p43_lookup_nonactive=%zu "
          "p43_lookup_miss=%zu p43_lookup_lockless_hits=%zu "
          "p43_lookup_lockless_misses=%zu "
          "p40_p43_free_before_max=%zu "
          "p40_skip_p43_free_calls=%zu "
          "p40_skip_p43_free_nodes=%zu "
          "p44_checkpoint_calls=%zu p44_candidate_total=%zu "
          "p44_candidate_max=%zu p44_global_candidate_total=%zu "
          "p44_stash_candidate_total=%zu "
          "p44_relbuf_candidate_total=%zu "
          "p44_p43_free_candidate_total=%zu "
          "p44_reason_publish=%zu p44_reason_acquire_miss=%zu "
          "p44_reason_p40=%zu p44_reason_snapshot=%zu "
          "p44_candidate_current=%zu "
          "p44_global_candidate_current=%zu "
          "p44_stash_candidate_current=%zu "
          "p44_relbuf_candidate_current=%zu "
          "p44_p43_free_candidate_current=%zu "
          "p43o_epoch=%zu p43o_state=%zu "
          "p43o_cooldown=%zu p43o_state_open=%zu "
          "p43o_state_drain=%zu p43o_state_closed=%zu "
          "p43o_admission_flips=%zu "
          "p43o_reason_acquire_miss=%zu p43o_reason_p40=%zu "
          "p43o_reason_snapshot=%zu p43o_p43_free_max=%zu "
          "p43o_p43_free_hot_current=%zu "
          "p43o_p43_free_warm_current=%zu "
          "p43o_p43_free_cold_current=%zu "
          "p43o_p43_free_hot_total=%zu "
          "p43o_p43_free_warm_total=%zu "
          "p43o_p43_free_cold_total=%zu "
          "p43o_p40_release_candidates=%zu "
          "p43o_p40_would_stage1=%zu "
          "p43o_p40_would_demote_to_source=%zu "
          "p43o_p40_would_keep_bridge=%zu "
          "p43o_source_miss=%zu "
          "p43o_projected_free_max=%zu "
          "p43o_projected_p40_candidates=%zu "
          "p43o_projected_would_stage1=%zu "
          "p43o_projected_would_demote_to_source=%zu "
          "p43o_projected_would_keep_bridge=%zu "
          "p43o_projected_blocks=%zu "
          "p43p_epoch=%zu p43p_state_open=%zu "
          "p43p_state_drain=%zu p43p_state_closed=%zu "
          "p43p_admission_flips=%zu "
          "p43p_reason_p40=%zu p43p_reason_snapshot=%zu "
          "p43p_p40_release_candidates=%zu "
          "p43p_would_stage1_bridge_cold=%zu "
          "p43p_would_demote_to_source=%zu "
          "p43p_would_keep_bridge=%zu "
          "p43p_bridge_cold_projected_current=%zu "
          "p43p_bridge_cold_projected_max=%zu "
          "p43p_p43_committed_free_current=%zu "
          "p43p_p43_committed_free_max=%zu "
          "p43p_stage1_current=%zu "
          "p43p_stage1_enqueue_calls=%zu "
          "p43p_stage1_enqueue_nodes=%zu "
          "p43p_stage1_acquire_hits=%zu "
          "p43p_stage1_acquire_nodes=%zu "
          "p43p_stage1_requeue_calls=%zu "
          "p43p_stage1_requeue_nodes=%zu "
          "p43p_stage1_current_max=%zu "
          "p43p_age_stage1_items_total=%zu "
          "p43p_age_young_current=%zu "
          "p43p_age_old_current=%zu "
          "p43p_age_would_reuse_young=%zu "
          "p43p_age_would_keep_old=%zu "
          "p43p_age_would_demote_old_open=%zu "
          "p43p_age_would_block_old_drain=%zu "
          "p43p_age_would_block_old_closed=%zu "
          "p43p_age_hot_reinject_avoided=%zu "
          "p43p_age_pop_all_pollution_projection=%zu "
          "p43p_age_admission_flips=%zu "
          "p45_epoch=%zu p45_state=%zu p45_cooldown=%zu "
          "p45_state_open=%zu p45_state_drain=%zu "
          "p45_state_closed=%zu p45_admission_flips=%zu "
          "p45_reason_publish=%zu p45_reason_acquire_miss=%zu "
          "p45_reason_p40=%zu p45_reason_snapshot=%zu "
          "p45_bridge_current=%zu p45_bridge_current_max=%zu "
          "p45_bridge_pressure_total=%zu "
          "p45_source_free_current=%zu "
          "p45_source_free_current_max=%zu "
          "p45_source_free_pressure_total=%zu "
          "p45_source_committed_current=%zu "
          "p45_source_committed_current_max=%zu "
          "p45_source_committed_pressure_total=%zu "
          "p45_p40_demote_intent=%zu "
          "p45_p40_demote_while_drain_closed=%zu "
          "p45_p40_demote_while_bridge_nonempty=%zu "
          "p45_would_demote_clean_open=%zu "
          "p45_would_stage1_any=%zu "
          "p45_would_stage1_state=%zu "
          "p45_would_stage1_bridge_nonempty=%zu "
          "p45_would_stage1_overlap=%zu "
          "p45_bridge_residual_after_demote_max=%zu "
          "p45_bridge_residual_after_demote_total=%zu "
          "p45_would_demote_open_bridge_soft=%zu "
          "p45_would_stage1_bridge_excess=%zu "
          "p45_would_stage1_refined_any=%zu "
          "p45_source_miss=%zu "
          "p45rg_demote_intent=%zu "
          "p45rg_allowed_open_bridge_soft=%zu "
          "p45rg_stage1_any=%zu "
          "p45rg_stage1_state=%zu "
          "p45rg_stage1_bridge_excess=%zu "
          "p45rg_stage1_overlap=%zu "
          "p45rg_stage1_current=%zu "
          "p45rg_stage1_enqueue_calls=%zu "
          "p45rg_stage1_enqueue_nodes=%zu "
          "p45rg_stage1_acquire_hits=%zu "
          "p45rg_stage1_acquire_nodes=%zu "
          "p45rg_stage1_requeue_calls=%zu "
          "p45rg_stage1_requeue_nodes=%zu "
          "p45rg_stage1_current_max=%zu "
          "p45rg_bridge_residual_max=%zu "
          "p45rg_bridge_residual_total=%zu "
          "p45dr_checkpoint_calls=%zu "
          "p45dr_reason_p40=%zu "
          "p45dr_reason_acquire_miss=%zu "
          "p45dr_reason_snapshot=%zu "
          "p45dr_stage1_current_at_checkpoint=%zu "
          "p45dr_stage1_current_at_checkpoint_max=%zu "
          "p45dr_stage1_current_at_acquire_miss=%zu "
          "p45dr_stage1_current_at_acquire_miss_max=%zu "
          "p45dr_stage1_current_at_snapshot=%zu "
          "p45dr_stage1_current_at_snapshot_max=%zu "
          "p45dr_stage1_age_bucket0=%zu "
          "p45dr_stage1_age_bucket1=%zu "
          "p45dr_stage1_age_bucket2_4=%zu "
          "p45dr_stage1_age_old=%zu "
          "p45dr_stage1_old_current_max=%zu "
          "p45dr_stage1_oldest_age_max=%zu "
          "p45dr_would_drain_to_bridge=%zu "
          "p45dr_would_keep_cold=%zu "
          "p45dr_would_demote_open=%zu "
          "p45dr_would_block_drain_closed=%zu "
          "p45dr_would_block_bridge_excess=%zu "
          "p45dr_bridge_current_hot_max=%zu "
          "p45dr_bridge_cold_current_max=%zu "
          "p45dr_bridge_total_with_cold_max=%zu "
          "p45dr_balance_expected=%zu "
          "p45dr_balance_actual=%zu "
          "p45dr_balance_mismatch=%zu "
          "p45dr_balance_mismatch_max=%zu\n",
          label ? label : "",
          g_hz5_lowpage64_stash_count,
          g_hz5_lowpage64_relbuf_count,
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_max,
                               memory_order_relaxed),
          p43_stats.segments_current,
          p43_stats.slots_total_current,
          p43_stats.slots_active_current,
          p43_stats.slots_committed_current,
          p43_stats.slots_committed_free_current,
          p43_stats.slots_global_retained_current,
          p43_stats.slots_tls_retained_current,
          p43_stats.slots_cold_current,
          p43_stats.slots_uncommitted_current,
          p43_stats.contract_lockless_enabled,
          p43_stats.lockless_lookup_enabled,
          p43_stats.slot_decommit_enabled,
          p43_stats.page_noaccess_enabled,
          p43_stats.runtime_segment_release_enabled,
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_active,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_nonactive,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_source_p25,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_source_other,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_raw_mismatch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_release_old_path_calls,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43g_release_prepared_calls,
              memory_order_relaxed),
          p43_stats.segments_reserved,
          p43_stats.segments_released,
          p43_stats.slot_commits,
          p43_stats.slot_decommits,
          p43_stats.tls_hits,
          p43_stats.global_hits,
          p43_stats.free_slot_hits,
          p43_stats.source_alloc_calls,
          p43_stats.source_cold_segment_scans_total,
          p43_stats.source_cold_segment_scans_max,
          p43_stats.source_free_segment_scans_total,
          p43_stats.source_free_segment_scans_max,
          p43_stats.source_new_segment_scan_total,
          p43_stats.source_new_segment_scan_max,
          p43_stats.lookup_calls,
          p43_stats.lookup_active,
          p43_stats.lookup_nonactive,
          p43_stats.lookup_miss,
          p43_stats.lookup_lockless_hits,
          p43_stats.lookup_lockless_misses,
          atomic_load_explicit(&g_hz5_lowpage64_p40_p43_free_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_global_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_stash_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_relbuf_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_p43_free_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_publish,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_snapshot,
                               memory_order_relaxed),
          p44_current_total,
          p44_global_current,
          p44_stash_current,
          p44_relbuf_current,
          p44_p43_free_current,
          atomic_load_explicit(&g_hz5_lowpage64_p43o_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_runtime_state,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_runtime_cooldown,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_max,
                               memory_order_relaxed),
          p43o_hot_current,
          p43o_warm_current,
          p43o_cold_current,
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_hot_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_warm_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_cold_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_release_candidates,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_would_stage1,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_p40_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_would_keep_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_source_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_projected_free_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_p40_candidates,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_stage1,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_keep_bridge,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_projected_blocks,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p40_release_candidates,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_would_stage1_bridge_cold,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_would_keep_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_bridge_cold_projected_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p43_committed_free_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p43_committed_free_max,
              memory_order_relaxed),
#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1
          atomic_load_explicit(&g_hz5_lowpage64_p43p_stage1_count,
                               memory_order_relaxed),
#else
          (size_t)0,
#endif
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_enqueue_calls,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_enqueue_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_acquire_hits,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_acquire_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_requeue_calls,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_requeue_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_current_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_stage1_items_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_young_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_old_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_reuse_young,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_keep_old,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_demote_old_open,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_block_old_drain,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_block_old_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_hot_reinject_avoided,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_pop_all_pollution_projection,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_admission_flips,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_runtime_state,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_runtime_cooldown,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_publish,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed) +
              g_hz5_lowpage64_stash_count + g_hz5_lowpage64_relbuf_count,
          atomic_load_explicit(&g_hz5_lowpage64_p45_bridge_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_bridge_pressure_total,
                               memory_order_relaxed),
          p43_stats.slots_committed_free_current,
          atomic_load_explicit(&g_hz5_lowpage64_p45_source_free_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_source_free_pressure_total,
              memory_order_relaxed),
          p43_stats.slots_committed_current,
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_source_committed_current_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_source_committed_pressure_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_p40_demote_intent,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_p40_demote_while_drain_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_p40_demote_while_bridge_nonempty,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_demote_clean_open,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_any,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_state,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_stage1_bridge_nonempty,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_overlap,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_bridge_residual_after_demote_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_bridge_residual_after_demote_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_demote_open_bridge_soft,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45_would_stage1_bridge_excess,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_would_stage1_refined_any,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45_source_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_demote_intent,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45rg_allowed_open_bridge_soft,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_any,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_state,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45rg_stage1_bridge_excess,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_overlap,
                               memory_order_relaxed),
#if HZ5_LOWPAGE64_STAGE1_ENABLED
          atomic_load_explicit(&g_hz5_lowpage64_p43p_stage1_count,
                               memory_order_relaxed),
#else
          (size_t)0,
#endif
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_enqueue_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_enqueue_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_acquire_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_acquire_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_requeue_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_requeue_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_stage1_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_bridge_residual_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45rg_bridge_residual_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_snapshot,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_stage1_current_at_snapshot_max,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_bucket0,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_bucket1,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_bucket2_4,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_age_old,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_old_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_stage1_oldest_age_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_would_drain_to_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_would_keep_cold,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_would_demote_open,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_would_block_drain_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_would_block_bridge_excess,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_bridge_current_hot_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_bridge_cold_current_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p45dr_bridge_total_with_cold_max,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_expected,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_actual,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_mismatch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p45dr_balance_mismatch_max,
                               memory_order_relaxed));
}
#else
void hz5_lowpage64_print_snapshot(const char* label) {
  (void)label;
}
#endif

#if HZ5_LOWPAGE64_STATS
static void hz5_lowpage64_print_once(void) {
  static atomic_flag printed = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set_explicit(&printed, memory_order_acq_rel)) {
    return;
  }

  Hz5Lowpage64P43StatsSnapshot p43_stats = {0};
  hz5_lowpage64_p43_stats_snapshot(&p43_stats);
  size_t p44_global_current = hz5_lowpage64_p44_candidate_value(
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_relaxed),
      HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP);
  size_t p44_stash_current = hz5_lowpage64_p44_candidate_value(
      g_hz5_lowpage64_stash_count, HZ5_LOWPAGE64_P44_STASH_SOFT_CAP);
  size_t p44_relbuf_current = hz5_lowpage64_p44_candidate_value(
      g_hz5_lowpage64_relbuf_count, HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP);
  size_t p44_p43_free_current = hz5_lowpage64_p44_candidate_value(
      p43_stats.slots_committed_free_current,
      HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP);
  size_t p44_current_total = p44_global_current + p44_stash_current +
                             p44_relbuf_current + p44_p43_free_current;
#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
  size_t p43o_hot_current = 0;
  size_t p43o_warm_current = 0;
  size_t p43o_cold_current = 0;
  hz5_lowpage64_p43o_bucket_free(p43_stats.slots_committed_free_current,
                                 &p43o_hot_current, &p43o_warm_current,
                                 &p43o_cold_current);
#else
  size_t p43o_hot_current = 0;
  size_t p43o_warm_current = 0;
  size_t p43o_cold_current = 0;
#endif

  fprintf(stderr,
          "[HZ5_LOWPAGE64] alloc_calls=%zu span_hits=%zu "
          "span_pushes=%zu span_full=%zu stash_hits=%zu "
          "global_batch_hits=%zu os_allocs=%zu free_calls=%zu "
          "relbuf_pushes=%zu batch_publishes=%zu batch_publish_nodes=%zu "
          "global_overflow_frees=%zu global_batch_count=%zu "
          "relbuf_count_max=%zu global_count_before_max=%zu "
          "global_count_after_max=%zu acquired_count_total=%zu "
          "acquired_count_max=%zu stash_set_calls=%zu "
          "stash_set_nodes_total=%zu stash_set_nodes_max=%zu "
          "trim_calls=%zu trim_freed_nodes=%zu "
          "adaptive_trigger_calls=%zu acquired_le64=%zu "
          "acquired_le128=%zu acquired_le192=%zu acquired_gt192=%zu "
          "stash_nodes_le64=%zu stash_nodes_le128=%zu "
          "stash_nodes_le192=%zu stash_nodes_gt192=%zu "
          "p37_overflow_hits=%zu p37_overflow_set_nodes_total=%zu "
          "p37_overflow_set_nodes_max=%zu "
          "p37_overflow_release_calls=%zu "
          "p37_overflow_release_nodes=%zu "
          "p37_high_event_releases=%zu p37_lifetime_releases=%zu "
          "p38_hot_hits=%zu p38_global_first_hits=%zu "
          "p38_set_hot_nodes_total=%zu p38_set_hot_nodes_max=%zu "
          "p38_global_cap_release_calls=%zu "
          "p38_global_cap_release_nodes=%zu "
          "p38_stash_trim_release_calls=%zu "
          "p38_stash_trim_release_nodes=%zu "
          "p38_p37_release_calls=%zu p38_p37_release_nodes=%zu "
          "p39_probation_hits=%zu "
          "p39_probation_set_nodes_total=%zu "
          "p39_probation_set_nodes_max=%zu "
          "p39_probation_release_calls=%zu "
          "p39_probation_release_nodes=%zu "
          "p39_probation_lifetime_releases=%zu "
          "p39_probation_replace_releases=%zu "
          "p40_checkpoint_calls=%zu p40_release_calls=%zu "
          "p40_release_nodes=%zu p40_release_age_nodes=%zu "
          "p40_release_hard_nodes=%zu p40_keep_nodes=%zu "
          "p40_p43_free_before_total=%zu p40_p43_free_before_max=%zu "
          "p40_p43_committed_before_total=%zu "
          "p40_p43_committed_before_max=%zu "
          "p40_skip_p43_free_calls=%zu p40_skip_p43_free_nodes=%zu "
          "p42_va_allocs=%zu p42_va_alloc_failures=%zu "
          "p42_va_releases=%zu p42_decommit_calls=%zu "
          "p42_decommit_failures=%zu p42_recommit_calls=%zu "
          "p42_recommit_failures=%zu p42_cold_hits=%zu "
          "p42_cold_count=%zu p42_cold_count_max=%zu "
          "p43_segments_reserved=%zu p43_segments_released=%zu "
          "p43_slot_commits=%zu p43_slot_commit_failures=%zu "
          "p43_slot_decommits=%zu p43_slot_decommit_failures=%zu "
          "p43_tls_hits=%zu p43_tls_pushes=%zu "
          "p43_tls_overflow_pushes=%zu p43_global_hits=%zu "
          "p43_global_pushes=%zu p43_cold_hits=%zu "
          "p43_free_slot_hits=%zu p43_source_alloc_calls=%zu "
          "p43_source_cold_segment_scans_total=%zu "
          "p43_source_cold_segment_scans_max=%zu "
          "p43_source_free_segment_scans_total=%zu "
          "p43_source_free_segment_scans_max=%zu "
          "p43_source_new_segment_scan_total=%zu "
          "p43_source_new_segment_scan_max=%zu p43_lookup_calls=%zu "
          "p43_lookup_active=%zu p43_lookup_nonactive=%zu "
          "p43_lookup_miss=%zu "
          "p43_lookup_segments_scanned_total=%zu "
          "p43_lookup_segments_scanned_max=%zu "
          "p43_lookup_fast_hits=%zu p43_lookup_fast_misses=%zu "
          "p43_lookup_lockless_hits=%zu "
          "p43_lookup_lockless_misses=%zu "
          "p43_find_fast_hits=%zu "
          "p43_find_segments_scanned_total=%zu "
          "p43_find_segments_scanned_max=%zu "
          "p43g_prepare_calls=%zu p43g_prepare_active=%zu "
          "p43g_prepare_nonactive=%zu p43g_prepare_miss=%zu "
          "p43g_source_p25=%zu p43g_source_other=%zu "
          "p43g_raw_mismatch=%zu "
          "p43g_release_old_path_calls=%zu "
          "p43g_release_prepared_calls=%zu "
          "p44_checkpoint_calls=%zu p44_candidate_total=%zu "
          "p44_candidate_max=%zu p44_global_observed_max=%zu "
          "p44_global_candidate_total=%zu "
          "p44_stash_observed_max=%zu "
          "p44_stash_candidate_total=%zu "
          "p44_relbuf_observed_max=%zu "
          "p44_relbuf_candidate_total=%zu "
          "p44_p43_free_observed_max=%zu "
          "p44_p43_free_candidate_total=%zu "
          "p44_reason_publish=%zu p44_reason_acquire_miss=%zu "
          "p44_reason_p40=%zu p44_reason_snapshot=%zu "
          "p44_candidate_current=%zu "
          "p44_global_candidate_current=%zu "
          "p44_stash_candidate_current=%zu "
          "p44_relbuf_candidate_current=%zu "
          "p44_p43_free_candidate_current=%zu "
          "p43o_epoch=%zu p43o_state=%zu "
          "p43o_cooldown=%zu p43o_state_open=%zu "
          "p43o_state_drain=%zu p43o_state_closed=%zu "
          "p43o_admission_flips=%zu "
          "p43o_reason_acquire_miss=%zu p43o_reason_p40=%zu "
          "p43o_reason_snapshot=%zu p43o_p43_free_max=%zu "
          "p43o_p43_free_hot_current=%zu "
          "p43o_p43_free_warm_current=%zu "
          "p43o_p43_free_cold_current=%zu "
          "p43o_p43_free_hot_total=%zu "
          "p43o_p43_free_warm_total=%zu "
          "p43o_p43_free_cold_total=%zu "
          "p43o_p40_release_candidates=%zu "
          "p43o_p40_would_stage1=%zu "
          "p43o_p40_would_demote_to_source=%zu "
          "p43o_p40_would_keep_bridge=%zu "
          "p43o_source_miss=%zu "
          "p43o_projected_free_max=%zu "
          "p43o_projected_p40_candidates=%zu "
          "p43o_projected_would_stage1=%zu "
          "p43o_projected_would_demote_to_source=%zu "
          "p43o_projected_would_keep_bridge=%zu "
          "p43o_projected_blocks=%zu "
          "p43p_epoch=%zu p43p_state_open=%zu "
          "p43p_state_drain=%zu p43p_state_closed=%zu "
          "p43p_admission_flips=%zu "
          "p43p_reason_p40=%zu p43p_reason_snapshot=%zu "
          "p43p_p40_release_candidates=%zu "
          "p43p_would_stage1_bridge_cold=%zu "
          "p43p_would_demote_to_source=%zu "
          "p43p_would_keep_bridge=%zu "
          "p43p_bridge_cold_projected_current=%zu "
          "p43p_bridge_cold_projected_max=%zu "
          "p43p_p43_committed_free_current=%zu "
          "p43p_p43_committed_free_max=%zu "
          "p43p_stage1_current=%zu "
          "p43p_stage1_enqueue_calls=%zu "
          "p43p_stage1_enqueue_nodes=%zu "
          "p43p_stage1_acquire_hits=%zu "
          "p43p_stage1_acquire_nodes=%zu "
          "p43p_stage1_requeue_calls=%zu "
          "p43p_stage1_requeue_nodes=%zu "
          "p43p_stage1_current_max=%zu "
          "p43p_age_stage1_items_total=%zu "
          "p43p_age_young_current=%zu "
          "p43p_age_old_current=%zu "
          "p43p_age_would_reuse_young=%zu "
          "p43p_age_would_keep_old=%zu "
          "p43p_age_would_demote_old_open=%zu "
          "p43p_age_would_block_old_drain=%zu "
          "p43p_age_would_block_old_closed=%zu "
          "p43p_age_hot_reinject_avoided=%zu "
          "p43p_age_pop_all_pollution_projection=%zu "
          "p43p_age_admission_flips=%zu\n",
          atomic_load_explicit(&g_hz5_lowpage64_alloc_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_pushes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_span_full,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_os_allocs,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_relbuf_pushes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_batch_publishes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_batch_publish_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_overflow_frees,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_relbuf_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_count_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_global_count_after_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_count_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_trim_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_trim_freed_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_adaptive_trigger_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le64,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le128,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_le192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_acquired_gt192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le64,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le128,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_le192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_stash_nodes_gt192,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_hits,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p37_overflow_set_nodes_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_overflow_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_high_event_releases,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p37_lifetime_releases,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_hot_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_first_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_set_hot_nodes_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_set_hot_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_cap_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_global_cap_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_stash_trim_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_stash_trim_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_p37_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p38_p37_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_hits,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_set_nodes_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_set_nodes_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p39_probation_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_lifetime_releases,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p39_probation_replace_releases,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_age_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_release_hard_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_keep_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_p43_free_before_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_p43_free_before_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p40_p43_committed_before_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p40_p43_committed_before_max,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p40_skip_p43_free_nodes,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_allocs,
                               memory_order_relaxed) +
              p43_stats.va_allocs,
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_alloc_failures,
                               memory_order_relaxed) +
              p43_stats.va_alloc_failures,
          atomic_load_explicit(&g_hz5_lowpage64_p42_va_releases,
                               memory_order_relaxed) +
              p43_stats.va_releases,
          atomic_load_explicit(&g_hz5_lowpage64_p42_decommit_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_decommit_failures,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_recommit_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_recommit_failures,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_cold_hits,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_cold_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p42_cold_count_max,
                               memory_order_relaxed),
          p43_stats.segments_reserved,
          p43_stats.segments_released,
          p43_stats.slot_commits,
          p43_stats.slot_commit_failures,
          p43_stats.slot_decommits,
          p43_stats.slot_decommit_failures,
          p43_stats.tls_hits,
          p43_stats.tls_pushes,
          p43_stats.tls_overflow_pushes,
          p43_stats.global_hits,
          p43_stats.global_pushes,
          p43_stats.cold_hits,
          p43_stats.free_slot_hits,
          p43_stats.source_alloc_calls,
          p43_stats.source_cold_segment_scans_total,
          p43_stats.source_cold_segment_scans_max,
          p43_stats.source_free_segment_scans_total,
          p43_stats.source_free_segment_scans_max,
          p43_stats.source_new_segment_scan_total,
          p43_stats.source_new_segment_scan_max,
          p43_stats.lookup_calls,
          p43_stats.lookup_active,
          p43_stats.lookup_nonactive,
          p43_stats.lookup_miss,
          p43_stats.lookup_segments_scanned_total,
          p43_stats.lookup_segments_scanned_max,
          p43_stats.lookup_fast_hits,
          p43_stats.lookup_fast_misses,
          p43_stats.lookup_lockless_hits,
          p43_stats.lookup_lockless_misses,
          p43_stats.find_fast_hits,
          p43_stats.find_segments_scanned_total,
          p43_stats.find_segments_scanned_max,
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_active,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_nonactive,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_prepare_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_source_p25,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_source_other,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_raw_mismatch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43g_release_old_path_calls,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43g_release_prepared_calls,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_checkpoint_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_candidate_max,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_global_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_global_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_stash_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_stash_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_relbuf_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_relbuf_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_p43_free_observed_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p44_p43_free_candidate_total,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_publish,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p44_reason_snapshot,
                               memory_order_relaxed),
          p44_current_total,
          p44_global_current,
          p44_stash_current,
          p44_relbuf_current,
          p44_p43_free_current,
          atomic_load_explicit(&g_hz5_lowpage64_p43o_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_runtime_state,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_runtime_cooldown,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_acquire_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_max,
                               memory_order_relaxed),
          p43o_hot_current,
          p43o_warm_current,
          p43o_cold_current,
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_hot_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_warm_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p43_free_cold_total,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_release_candidates,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_would_stage1,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_p40_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_p40_would_keep_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_source_miss,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_projected_free_max,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_p40_candidates,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_stage1,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43o_projected_would_keep_bridge,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43o_projected_blocks,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_epoch,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_open,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_drain,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_state_closed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_admission_flips,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_reason_p40,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_reason_snapshot,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p40_release_candidates,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_would_stage1_bridge_cold,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_would_demote_to_source,
              memory_order_relaxed),
          atomic_load_explicit(&g_hz5_lowpage64_p43p_would_keep_bridge,
                               memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_bridge_cold_projected_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p43_committed_free_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_p43_committed_free_max,
              memory_order_relaxed),
#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1
          atomic_load_explicit(&g_hz5_lowpage64_p43p_stage1_count,
                               memory_order_relaxed),
#else
          (size_t)0,
#endif
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_enqueue_calls,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_enqueue_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_acquire_hits,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_acquire_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_requeue_calls,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_requeue_nodes,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_stage1_current_max,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_stage1_items_total,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_young_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_old_current,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_reuse_young,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_keep_old,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_demote_old_open,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_block_old_drain,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_would_block_old_closed,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_hot_reinject_avoided,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_pop_all_pollution_projection,
              memory_order_relaxed),
          atomic_load_explicit(
              &g_hz5_lowpage64_p43p_age_admission_flips,
              memory_order_relaxed));
}
#endif

void hz5_lowpage64_register_stats_once(void) {
#if HZ5_LOWPAGE64_STATS
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_lowpage64_print_once);
  }
#endif
}
