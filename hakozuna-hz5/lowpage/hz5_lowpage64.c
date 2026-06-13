#include "hz5_lowpage64.h"
#include "hz5_lowpage64_p43_segment.h"

#include <malloc.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#define HZ5_TLS_STATIC __declspec(thread) static
#else
#define HZ5_TLS_STATIC static __thread
void* _aligned_malloc(size_t size, size_t alignment);
void _aligned_free(void* ptr);
#endif

void* hz5_lowpage64_raw_os_alloc(size_t raw_bytes);
void hz5_lowpage64_raw_os_release(void* raw);
void* hz5_lowpage64_p42_try_recommit(size_t raw_bytes);

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

/*
 * Stage1 is the current experimental name for the HZ5 BRIDGE_COLD state.
 * It is not an OS-return path: nodes remain committed and bridge-side. P43p
 * used it as bridge-cold evidence, while P45 uses it as the refined-gate
 * target for P40 source-demotion intents that should not enter the P43 source
 * layer yet.
 */
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

#include "hz5_lowpage64_control.inc"

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

#if HZ5_LOWPAGE64_STAGE1_ENABLED && HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  size_t bridge_cold_count = 0;
#if HZ5_LOWPAGE64_P45_REFINED_GATE
  size_t stage1_limit = HZ5_LOWPAGE64_P45_STAGE1_ACQUIRE_LIMIT;
#else
  size_t stage1_limit = HZ5_LOWPAGE64_P43P_STAGE1_ACQUIRE_LIMIT;
#endif
  /*
   * Source miss bridge-cold reuse. This runs after the normal P25 bridge
   * global batch miss and before the P43 source layer allocates/commits new
   * slot state. It is reuse, not drain-to-OS.
   */
  void* bridge_cold =
      hz5_lowpage64_bridge_cold_pop_batch(stage1_limit,
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

void* hz5_lowpage64_acquire_p43_token(size_t raw_bytes,
                                      Hz5Lowpage64FreeCtx* ctx) {
  hz5_lowpage64_prepare_ctx_clear(ctx);
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  void* raw = hz5_lowpage64_p43_alloc_slot_prepared(raw_bytes, ctx);
  if (raw) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_os_allocs, 1);
  }
  return raw;
#else
  void* raw = hz5_lowpage64_acquire(raw_bytes);
  if (raw) {
    (void)hz5_lowpage64_prepare_free_raw(raw, ctx);
  }
  return raw;
#endif
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

#include "hz5_lowpage64_stats_print.inc"
