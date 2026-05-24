#include "hz5_ownerhub.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BENCHLAB_HZ5_LINUX_OWNERHUB_R1
#define BENCHLAB_HZ5_LINUX_OWNERHUB_R1 0
#endif

#if defined(__linux__) && BENCHLAB_HZ5_LINUX_OWNERHUB_R1

#define HZ5_OWNERHUB_SMALL_SHIFT 0u
#define HZ5_OWNERHUB_MID_SHIFT 16u
#define HZ5_OWNERHUB_LARGE_SHIFT 32u
#define HZ5_OWNERHUB_SMALL_BITS 14u
#define HZ5_OWNERHUB_MID_BITS 5u
#define HZ5_OWNERHUB_LARGE_BITS 4u

static _Atomic uint64_t g_hz5_ownerhub_pending[UINT16_MAX + 1u];

static _Atomic uint64_t g_hz5_ownerhub_publish_small;
static _Atomic uint64_t g_hz5_ownerhub_publish_mid;
static _Atomic uint64_t g_hz5_ownerhub_publish_large;
static _Atomic uint64_t g_hz5_ownerhub_alloc_miss_small;
static _Atomic uint64_t g_hz5_ownerhub_alloc_miss_mid;
static _Atomic uint64_t g_hz5_ownerhub_alloc_miss_large;
static _Atomic uint64_t g_hz5_ownerhub_alloc_miss_target_pending;
static _Atomic uint64_t g_hz5_ownerhub_alloc_miss_other_pending;
static _Atomic uint64_t g_hz5_ownerhub_alloc_miss_any_pending;

static uint64_t hz5_ownerhub_front_mask(Hz5OwnerHubFront front) {
  switch (front) {
    case HZ5_OWNERHUB_FRONT_SMALL:
      return ((UINT64_C(1) << HZ5_OWNERHUB_SMALL_BITS) - UINT64_C(1))
             << HZ5_OWNERHUB_SMALL_SHIFT;
    case HZ5_OWNERHUB_FRONT_MID:
      return ((UINT64_C(1) << HZ5_OWNERHUB_MID_BITS) - UINT64_C(1))
             << HZ5_OWNERHUB_MID_SHIFT;
    case HZ5_OWNERHUB_FRONT_LARGE:
      return ((UINT64_C(1) << HZ5_OWNERHUB_LARGE_BITS) - UINT64_C(1))
             << HZ5_OWNERHUB_LARGE_SHIFT;
    default:
      return 0;
  }
}

static uint64_t hz5_ownerhub_bit(Hz5OwnerHubFront front,
                                 uint32_t class_index) {
  switch (front) {
    case HZ5_OWNERHUB_FRONT_SMALL:
      if (class_index >= HZ5_OWNERHUB_SMALL_BITS) {
        return 0;
      }
      return UINT64_C(1) << (HZ5_OWNERHUB_SMALL_SHIFT + class_index);
    case HZ5_OWNERHUB_FRONT_MID:
      if (class_index >= HZ5_OWNERHUB_MID_BITS) {
        return 0;
      }
      return UINT64_C(1) << (HZ5_OWNERHUB_MID_SHIFT + class_index);
    case HZ5_OWNERHUB_FRONT_LARGE:
      if (class_index >= HZ5_OWNERHUB_LARGE_BITS) {
        return 0;
      }
      return UINT64_C(1) << (HZ5_OWNERHUB_LARGE_SHIFT + class_index);
    default:
      return 0;
  }
}

void hz5_ownerhub_mark_pending(Hz5OwnerToken owner,
                               Hz5OwnerHubFront front,
                               uint32_t class_index) {
  if (owner.slot == 0 || !hz5_owner_is_alive(owner)) {
    return;
  }
  uint64_t bit = hz5_ownerhub_bit(front, class_index);
  if (bit == 0) {
    return;
  }
  atomic_fetch_or_explicit(&g_hz5_ownerhub_pending[owner.slot],
                           bit,
                           memory_order_release);
  switch (front) {
    case HZ5_OWNERHUB_FRONT_SMALL:
      atomic_fetch_add_explicit(&g_hz5_ownerhub_publish_small,
                                1u,
                                memory_order_relaxed);
      break;
    case HZ5_OWNERHUB_FRONT_MID:
      atomic_fetch_add_explicit(&g_hz5_ownerhub_publish_mid,
                                1u,
                                memory_order_relaxed);
      break;
    case HZ5_OWNERHUB_FRONT_LARGE:
      atomic_fetch_add_explicit(&g_hz5_ownerhub_publish_large,
                                1u,
                                memory_order_relaxed);
      break;
    default:
      break;
  }
}

void hz5_ownerhub_clear_pending(Hz5OwnerToken owner,
                                Hz5OwnerHubFront front,
                                uint32_t class_index) {
  if (owner.slot == 0) {
    return;
  }
  uint64_t bit = hz5_ownerhub_bit(front, class_index);
  if (bit == 0) {
    return;
  }
  atomic_fetch_and_explicit(&g_hz5_ownerhub_pending[owner.slot],
                            ~bit,
                            memory_order_acq_rel);
}

void hz5_ownerhub_note_alloc_miss(Hz5OwnerToken owner,
                                  Hz5OwnerHubFront front,
                                  uint32_t class_index) {
  if (owner.slot == 0) {
    return;
  }
  uint64_t bit = hz5_ownerhub_bit(front, class_index);
  uint64_t front_mask = hz5_ownerhub_front_mask(front);
  if (bit == 0 || front_mask == 0) {
    return;
  }
  uint64_t pending = atomic_load_explicit(&g_hz5_ownerhub_pending[owner.slot],
                                          memory_order_acquire);
  switch (front) {
    case HZ5_OWNERHUB_FRONT_SMALL:
      atomic_fetch_add_explicit(&g_hz5_ownerhub_alloc_miss_small,
                                1u,
                                memory_order_relaxed);
      break;
    case HZ5_OWNERHUB_FRONT_MID:
      atomic_fetch_add_explicit(&g_hz5_ownerhub_alloc_miss_mid,
                                1u,
                                memory_order_relaxed);
      break;
    case HZ5_OWNERHUB_FRONT_LARGE:
      atomic_fetch_add_explicit(&g_hz5_ownerhub_alloc_miss_large,
                                1u,
                                memory_order_relaxed);
      break;
    default:
      break;
  }
  if (pending != 0) {
    atomic_fetch_add_explicit(&g_hz5_ownerhub_alloc_miss_any_pending,
                              1u,
                              memory_order_relaxed);
  }
  if ((pending & bit) != 0) {
    atomic_fetch_add_explicit(&g_hz5_ownerhub_alloc_miss_target_pending,
                              1u,
                              memory_order_relaxed);
  }
  if ((pending & ~front_mask) != 0) {
    atomic_fetch_add_explicit(&g_hz5_ownerhub_alloc_miss_other_pending,
                              1u,
                              memory_order_relaxed);
  }
}

void hz5_ownerhub_stats_print(void) {
  if (!getenv("HZ5_OWNERHUB_STATS")) {
    return;
  }
  fprintf(stderr,
          "[HZ5_OWNERHUB_STATS]"
          " publish_small=%llu publish_mid=%llu publish_large=%llu"
          " miss_small=%llu miss_mid=%llu miss_large=%llu"
          " miss_any_pending=%llu miss_target_pending=%llu"
          " miss_other_pending=%llu\n",
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_publish_small, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_publish_mid, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_publish_large, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_alloc_miss_small, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_alloc_miss_mid, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_alloc_miss_large, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_alloc_miss_any_pending, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_alloc_miss_target_pending,
              memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_ownerhub_alloc_miss_other_pending,
              memory_order_relaxed));
}

#else
typedef int hz5_ownerhub_translation_unit_not_empty;
#endif
