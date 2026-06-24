#include "h8_internal.h"
#include "h8_medium.h"

#include <sys/mman.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

static const size_t k_h8_medium_empty_resident_budget =
    (size_t)H8_OWNER_MAX * H8_MEDIUM_RESIDENT_BUDGET_CLASSES *
    H8_MEDIUM_RUN_BYTES;

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_medium_residency_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

static void h8_medium_update_resident_peak(size_t value) {
  size_t cur =
      atomic_load_explicit(&h8g.medium_resident_empty_peak, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.medium_resident_empty_peak, &cur, value,
             memory_order_relaxed, memory_order_relaxed)) {
  }
}

static bool h8_medium_try_reserve_empty_payload(H8MediumRun* run) {
  if (!run || run->resident_charge) {
    return true;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  size_t cur =
      atomic_load_explicit(&h8g.medium_resident_empty_bytes, memory_order_relaxed);
  for (;;) {
    if (cur > k_h8_medium_empty_resident_budget ||
        bytes > k_h8_medium_empty_resident_budget - cur) {
      H8_DEBUG_INC(medium_empty_budget_reject_count);
      return false;
    }
    size_t next = cur + bytes;
    if (atomic_compare_exchange_weak_explicit(
            &h8g.medium_resident_empty_bytes, &cur, next,
            memory_order_acq_rel, memory_order_relaxed)) {
      run->resident_charge = true;
      h8_medium_update_resident_peak(next);
      return true;
    }
  }
}

void h8_medium_release_empty_payload(H8MediumRun* run) {
  if (!run || !run->resident_charge) {
    return;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  atomic_fetch_sub_explicit(&h8g.medium_resident_empty_bytes, bytes,
                            memory_order_acq_rel);
  run->resident_charge = false;
}

static void h8_medium_madvise_run(H8MediumRun* run) {
  if (!run || !run->base || run->run_size == 0) {
    return;
  }
  H8_DEBUG_INC(medium_run_madvise_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_residency_now_ns();
#endif
  if (madvise(run->base, run->run_size, MADV_DONTNEED) != 0) {
    H8_DEBUG_INC(medium_madvise_fail_count);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_madvise_ns,
               (size_t)(h8_medium_residency_now_ns() - start));
#endif
}

void h8_medium_decommit_empty_locked(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  h8_medium_madvise_run(run);
  h8_medium_release_empty_payload(run);
  run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED;
}

void h8_medium_mark_live_on_alloc(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT) {
    H8_DEBUG_INC(medium_empty_reactivate_count);
    h8_medium_release_empty_payload(run);
  }
  run->payload_state = H8_MEDIUM_PAYLOAD_LIVE;
}

void h8_medium_mark_empty_locked(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  H8_DEBUG_INC(medium_empty_transition_count);
  if (run->owner_attached && h8_medium_try_reserve_empty_payload(run)) {
    H8_DEBUG_INC(medium_empty_retain_count);
    run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT;
    return;
  }
  h8_medium_decommit_empty_locked(run);
}
