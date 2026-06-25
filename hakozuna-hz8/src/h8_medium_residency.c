#include "h8_internal.h"
#include "h8_medium.h"

#include <sys/mman.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

static const size_t k_h8_medium_empty_resident_budget =
    (size_t)H8_OWNER_MAX * H8_MEDIUM_RESIDENT_BUDGET_CLASSES *
    H8_MEDIUM_RUN_BYTES;

#if defined(H8_MEDIUM_BUDGET_REJECT_LAZY_PURGE)
#if !defined(H8_MEDIUM_LAZY_PURGE_BYTES)
#define H8_MEDIUM_LAZY_PURGE_BYTES (128u * 1024u * 1024u)
#endif
static atomic_size_t h8_medium_lazy_purge_bytes_runtime;
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static pthread_mutex_t h8_medium_retention_debug_lock = PTHREAD_MUTEX_INITIALIZER;

static void h8_medium_retention_debug_lock_acquire(void) {
  pthread_mutex_lock(&h8_medium_retention_debug_lock);
}

static void h8_medium_retention_debug_lock_release(void) {
  pthread_mutex_unlock(&h8_medium_retention_debug_lock);
}
#else
static void h8_medium_retention_debug_lock_acquire(void) {
}

static void h8_medium_retention_debug_lock_release(void) {
}
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_medium_residency_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_medium_update_lazy_purge_peak(size_t value) {
  size_t cur =
      atomic_load_explicit(&h8g.medium_lazy_purge_peak, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.medium_lazy_purge_peak, &cur, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

static void h8_medium_lazy_purge_shadow_note_candidate(H8MediumRun* run) {
  if (!run || run->debug_lazy_purge_candidate) {
    return;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  run->debug_lazy_purge_candidate = true;
  H8_DEBUG_INC(medium_lazy_purge_candidate);
  size_t next = atomic_fetch_add_explicit(&h8g.medium_lazy_purge_bytes, bytes,
                                          memory_order_acq_rel) +
                bytes;
  h8_medium_update_lazy_purge_peak(next);
  if (next > 16u * 1024u * 1024u) {
    H8_DEBUG_INC(medium_lazy_purge_over_16m);
  }
  if (next > 32u * 1024u * 1024u) {
    H8_DEBUG_INC(medium_lazy_purge_over_32m);
  }
}

static bool h8_medium_lazy_purge_shadow_clear(H8MediumRun* run) {
  if (!run || !run->debug_lazy_purge_candidate) {
    return false;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  run->debug_lazy_purge_candidate = false;
  atomic_fetch_sub_explicit(&h8g.medium_lazy_purge_bytes, bytes,
                            memory_order_acq_rel);
  return true;
}
#endif

void h8_medium_lazy_purge_shadow_drop(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  (void)h8_medium_lazy_purge_shadow_clear(run);
#else
  (void)run;
#endif
#if defined(H8_MEDIUM_BUDGET_REJECT_LAZY_PURGE)
  if (run && run->lazy_purge_charge) {
    size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
    atomic_fetch_sub_explicit(&h8_medium_lazy_purge_bytes_runtime, bytes,
                              memory_order_acq_rel);
    run->lazy_purge_charge = false;
  }
#endif
}

static void h8_medium_decommit_empty_with_reason_locked_impl(
    H8MediumRun* run, H8MediumDecommitReason reason);

static void h8_medium_update_resident_peak(size_t value) {
  size_t cur =
      atomic_load_explicit(&h8g.medium_resident_empty_peak, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.medium_resident_empty_peak, &cur, value,
             memory_order_relaxed, memory_order_relaxed)) {
  }
}

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_medium_update_active_live_peak(size_t value) {
  size_t cur = atomic_load_explicit(&h8g.medium_active_live_empty_peak,
                                    memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.medium_active_live_empty_peak, &cur, value,
             memory_order_relaxed, memory_order_relaxed)) {
  }
}

static H8OwnerRecord* h8_medium_shadow_owner(H8MediumRun* run) {
  if (!run || !run->owner_attached) {
    return NULL;
  }
  uint64_t raw = atomic_load_explicit(&run->owner_word, memory_order_acquire);
  if (raw == 0) {
    return NULL;
  }
  H8OwnerWord word = h8_owner_word_unpack(raw);
  if (word.slot >= H8_OWNER_MAX) {
    return NULL;
  }
  H8OwnerRecord* owner = h8_owner_by_slot(word.slot);
  if (!owner || owner->slot != word.slot || owner->generation != word.generation) {
    return NULL;
  }
  return owner;
}

static bool h8_medium_warm2_contains(H8OwnerRecord* owner, uint32_t class_id,
                                     H8MediumRun* run) {
  return owner->medium_warm_shadow2[class_id][0] == run ||
         owner->medium_warm_shadow2[class_id][1] == run;
}

static void h8_medium_warm_shadow_install(H8MediumRun* run) {
  H8OwnerRecord* owner = h8_medium_shadow_owner(run);
  if (!owner || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  uint32_t c = run->class_id;
  if (owner->medium_warm_shadow1[c] != run) {
    if (owner->medium_warm_shadow1[c]) {
      H8_DEBUG_INC(medium_warm1_would_replace);
    } else {
      H8_DEBUG_INC(medium_warm1_would_install);
    }
    owner->medium_warm_shadow1[c] = run;
  }

  if (owner->medium_warm_shadow2[c][0] == run) {
    return;
  }
  if (owner->medium_warm_shadow2[c][1] == run) {
    owner->medium_warm_shadow2[c][1] = owner->medium_warm_shadow2[c][0];
    owner->medium_warm_shadow2[c][0] = run;
    return;
  }
  if (owner->medium_warm_shadow2[c][1]) {
    H8_DEBUG_INC(medium_warm2_would_replace);
  } else {
    H8_DEBUG_INC(medium_warm2_would_install);
  }
  owner->medium_warm_shadow2[c][1] = owner->medium_warm_shadow2[c][0];
  owner->medium_warm_shadow2[c][0] = run;
}

static void h8_medium_warm_shadow_note_budget_reject(H8MediumRun* run) {
  H8OwnerRecord* owner = h8_medium_shadow_owner(run);
  if (!owner || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  uint32_t c = run->class_id;
  if (owner->medium_warm_shadow1[c] == run) {
    H8_DEBUG_INC(medium_warm1_would_avoid_budget_reject);
  }
  if (h8_medium_warm2_contains(owner, c, run)) {
    H8_DEBUG_INC(medium_warm2_would_avoid_budget_reject);
  }
}

static void h8_medium_warm_shadow_note_alloc(H8MediumRun* run) {
  H8OwnerRecord* owner = h8_medium_shadow_owner(run);
  if (!owner || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    if (run && run->active_live_empty_charge) {
      H8_DEBUG_INC(medium_warm_reuse_distance_0);
    }
    return;
  }
  uint32_t c = run->class_id;
  bool hit1 = owner->medium_warm_shadow1[c] == run;
  if (hit1) {
    H8_DEBUG_INC(medium_warm1_reuse_hit);
    owner->medium_warm_shadow1[c] = NULL;
  }

  if (run->active_live_empty_charge) {
    H8_DEBUG_INC(medium_warm_reuse_distance_0);
  } else if (owner->medium_warm_shadow2[c][0] == run) {
    H8_DEBUG_INC(medium_warm2_reuse_hit);
    H8_DEBUG_INC(medium_warm_reuse_distance_1);
    owner->medium_warm_shadow2[c][0] = owner->medium_warm_shadow2[c][1];
    owner->medium_warm_shadow2[c][1] = NULL;
  } else if (owner->medium_warm_shadow2[c][1] == run) {
    H8_DEBUG_INC(medium_warm2_reuse_hit);
    H8_DEBUG_INC(medium_warm_reuse_distance_2);
    owner->medium_warm_shadow2[c][1] = NULL;
  } else {
    H8_DEBUG_INC(medium_warm_reuse_distance_3p);
  }
}
#endif

void h8_medium_note_active_live_empty(H8MediumRun* run) {
  if (!run || run->active_live_empty_charge) {
    return;
  }
  run->active_live_empty_charge = true;
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  size_t next = atomic_fetch_add_explicit(
                    &h8g.medium_active_live_empty_bytes, bytes,
                    memory_order_acq_rel) +
                bytes;
  h8_medium_update_active_live_peak(next);
#endif
}

void h8_medium_clear_active_live_empty(H8MediumRun* run) {
  if (!run || !run->active_live_empty_charge) {
    return;
  }
  run->active_live_empty_charge = false;
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  size_t cur = atomic_load_explicit(&h8g.medium_active_live_empty_bytes,
                                    memory_order_acquire);
  while (cur >= bytes &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.medium_active_live_empty_bytes, &cur, cur - bytes,
             memory_order_acq_rel, memory_order_acquire)) {
  }
#endif
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
#if defined(H8_ENABLE_DEBUG_STATS)
      h8_medium_warm_shadow_note_budget_reject(run);
#endif
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

#if defined(H8_MEDIUM_BUDGET_REJECT_LAZY_PURGE)
static bool h8_medium_try_lazy_purge_payload(H8MediumRun* run) {
  if (!run || run->lazy_purge_charge || !run->owner_attached) {
    return run && run->lazy_purge_charge;
  }
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  size_t cur = atomic_load_explicit(&h8_medium_lazy_purge_bytes_runtime,
                                    memory_order_relaxed);
  for (;;) {
    if (cur > H8_MEDIUM_LAZY_PURGE_BYTES ||
        bytes > H8_MEDIUM_LAZY_PURGE_BYTES - cur) {
      return false;
    }
    size_t next = cur + bytes;
    if (atomic_compare_exchange_weak_explicit(
            &h8_medium_lazy_purge_bytes_runtime, &cur, next,
            memory_order_acq_rel, memory_order_relaxed)) {
      run->lazy_purge_charge = true;
#if defined(H8_ENABLE_DEBUG_STATS)
      h8_medium_lazy_purge_shadow_note_candidate(run);
#endif
      return true;
    }
  }
}
#endif

static int h8_medium_madvise_advice(H8MediumDecommitReason reason) {
#if defined(H8_MEDIUM_BUDGET_REJECT_MADV_FREE) && defined(MADV_FREE)
  if (reason == H8_MEDIUM_DECOMMIT_BUDGET_REJECT) {
    return MADV_FREE;
  }
#else
  (void)reason;
#endif
  return MADV_DONTNEED;
}

static void h8_medium_madvise_run(H8MediumRun* run,
                                  H8MediumDecommitReason reason) {
  if (!run || !run->base || run->run_size == 0) {
    return;
  }
  H8_DEBUG_INC(medium_run_madvise_count);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_residency_now_ns();
#endif
  if (madvise(run->base, run->run_size, h8_medium_madvise_advice(reason)) != 0) {
    H8_DEBUG_INC(medium_madvise_fail_count);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_madvise_ns,
               (size_t)(h8_medium_residency_now_ns() - start));
#endif
}

static void h8_medium_decommit_empty_with_reason_locked(
    H8MediumRun* run, H8MediumDecommitReason reason) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  h8_medium_retention_debug_lock_acquire();
  h8_medium_decommit_empty_with_reason_locked_impl(run, reason);
  h8_medium_retention_debug_lock_release();
}

static void h8_medium_decommit_empty_with_reason_locked_impl(
    H8MediumRun* run, H8MediumDecommitReason reason) {
#if !defined(H8_ENABLE_DEBUG_STATS)
  (void)reason;
#endif
  h8_medium_retention_shadow_note_decommit(run, reason);
#if defined(H8_ENABLE_DEBUG_STATS)
  if (reason == H8_MEDIUM_DECOMMIT_BUDGET_REJECT) {
    h8_medium_lazy_purge_shadow_note_candidate(run);
  } else {
    (void)h8_medium_lazy_purge_shadow_clear(run);
  }
#endif
  if (run->payload_state == H8_MEDIUM_PAYLOAD_LIVE) {
    h8_medium_clear_active_live_empty(run);
  }
  h8_medium_madvise_run(run, reason);
  h8_medium_release_empty_payload(run);
  h8_medium_lazy_purge_shadow_drop(run);
#if defined(H8_MEDIUM_BUDGET_REJECT_MADV_FREE) && defined(MADV_FREE)
  if (reason == H8_MEDIUM_DECOMMIT_BUDGET_REJECT) {
    /*
     * MADV_FREE may keep pages resident until pressure; owner-exit hard drain
     * must still see this run as physically purgeable.
     */
    run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT;
    return;
  }
#endif
  run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED;
}

void h8_medium_decommit_empty_locked(H8MediumRun* run) {
  h8_medium_decommit_empty_with_reason_locked(run, H8_MEDIUM_DECOMMIT_COLD);
}

void h8_medium_decommit_empty_owner_exit_locked(H8MediumRun* run) {
  h8_medium_decommit_empty_with_reason_locked(run, H8_MEDIUM_DECOMMIT_OWNER_EXIT);
}

void h8_medium_mark_live_on_alloc(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    H8_DEBUG_INC(medium_alloc_mark_live_nonempty);
    return;
  }
  h8_medium_retention_debug_lock_acquire();
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_medium_retention_shadow_note_alloc(run);
  h8_medium_warm_shadow_note_alloc(run);
  if (h8_medium_lazy_purge_shadow_clear(run)) {
    H8_DEBUG_INC(medium_lazy_purge_reuse);
  }
#endif
  if (run->payload_state == H8_MEDIUM_PAYLOAD_LIVE) {
    if (run->active_live_empty_charge) {
      H8_DEBUG_INC(medium_alloc_mark_live_active_empty);
    } else {
      H8_DEBUG_INC(medium_alloc_mark_live_live);
    }
    h8_medium_clear_active_live_empty(run);
  }
  if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT) {
    H8_DEBUG_INC(medium_alloc_mark_live_resident);
    H8_DEBUG_INC(medium_empty_reactivate_count);
    h8_medium_release_empty_payload(run);
    h8_medium_lazy_purge_shadow_drop(run);
  }
  if (run->payload_state == H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED) {
    H8_DEBUG_INC(medium_alloc_mark_live_decommitted);
  }
  run->payload_state = H8_MEDIUM_PAYLOAD_LIVE;
  h8_medium_retention_debug_lock_release();
}

void h8_medium_mark_empty_locked(H8MediumRun* run) {
  if (!run || run->allocated_mask != 0) {
    return;
  }
  H8_DEBUG_INC(medium_empty_transition_count);
  h8_medium_retention_debug_lock_acquire();
  h8_medium_clear_active_live_empty(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  h8_medium_retention_shadow_note_empty(run);
  h8_medium_warm_shadow_install(run);
#endif
  if (run->owner_attached && h8_medium_try_reserve_empty_payload(run)) {
    h8_medium_retention_shadow_note_retain(run);
    H8_DEBUG_INC(medium_empty_retain_count);
    run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT;
    h8_medium_retention_debug_lock_release();
    return;
  }
#if defined(H8_MEDIUM_BUDGET_REJECT_LAZY_PURGE)
  if (h8_medium_try_lazy_purge_payload(run)) {
    run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_RESIDENT;
    h8_medium_retention_debug_lock_release();
    return;
  }
#endif
  h8_medium_decommit_empty_with_reason_locked_impl(
      run, H8_MEDIUM_DECOMMIT_BUDGET_REJECT);
  h8_medium_retention_debug_lock_release();
}
