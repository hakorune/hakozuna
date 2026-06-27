#include "h8_internal.h"
#include "h8_medium.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_medium_slots_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}
#endif

bool h8_medium_slot_index_from_ptr_checked(const H8MediumRun* run,
                                           const void* ptr,
                                           size_t* slot_out) {
  return h8_medium_slot_index_from_ptr_checked_fast(run, ptr, slot_out);
}

void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot) {
  return h8_medium_slot_ptr_known(run, slot);
}

void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_slots_now_ns();
#endif
  if (!run) {
    return NULL;
  }
  if (atomic_load_explicit(&run->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(medium_alloc_state_check_fail);
    return NULL;
  }
  if (run->free_mask == 0) {
    H8_DEBUG_INC(medium_alloc_free_mask_zero);
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t section_start = h8_medium_slots_now_ns();
#endif
  h8_medium_mark_live_on_alloc_fast(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_alloc_mark_live_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_alloc_mask_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
#if !defined(H8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE)
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_alloc_slot_store_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
  void* ptr = h8_medium_slot_ptr_fast(run, slot);
  H8_DEBUG_ADD(medium_alloc_ptr_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  H8_DEBUG_INC(medium_alloc_scaffold_count);
  H8_DEBUG_INC(medium_alloc_slot_count);
  H8_DEBUG_ADD(medium_alloc_slot_ns,
               (size_t)(h8_medium_slots_now_ns() - start));
  return ptr;
#endif
  return h8_medium_slot_ptr_fast(run, slot);
}

bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr,
                                       bool keep_empty_live) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_slots_now_ns();
  uint64_t section_start = start;
#endif
  size_t slot = 0;
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  if (run && run->class_id == 2u) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_INC(medium_24k_local_free_decode_attempt);
    size_t generic_slot = 0u;
    bool generic_ok = h8_medium_slot_index_from_ptr_checked_fast(
        run, ptr, &generic_slot);
    bool fast_ok = h8_medium_24k_local_free_slot_index_fast(run, ptr, &slot);
    if (fast_ok) {
      if (slot == 0u) {
        H8_DEBUG_INC(medium_24k_local_free_decode_valid_slot0);
      } else if (slot == 1u) {
        H8_DEBUG_INC(medium_24k_local_free_decode_valid_slot1);
      } else {
        H8_DEBUG_INC(medium_24k_local_free_decode_invalid);
      }
    } else {
      H8_DEBUG_INC(medium_24k_local_free_decode_invalid);
    }
    if (fast_ok != generic_ok || (fast_ok && slot != generic_slot)) {
      H8_DEBUG_INC(medium_24k_local_free_decode_equiv_mismatch);
    }
    if (!fast_ok) {
      H8_DEBUG_ADD(medium_free_decode_ns,
                   (size_t)(h8_medium_slots_now_ns() - section_start));
      return false;
    }
#else
    if (!h8_medium_24k_local_free_slot_index_fast(run, ptr, &slot)) {
      return false;
    }
#endif
  } else
#endif
  if (!h8_medium_slot_index_from_ptr_checked_fast(run, ptr, &slot)) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_free_decode_ns,
                 (size_t)(h8_medium_slots_now_ns() - section_start));
#endif
    return false;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_free_decode_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
  uint64_t bit = UINT64_C(1) << slot;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) ||
      (run->allocated_mask & bit) == 0 || (run->free_mask & bit) != 0) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_free_state_ns,
                 (size_t)(h8_medium_slots_now_ns() - section_start));
#endif
    return false;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_free_state_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
#if !defined(H8_MEDIUM_LOCAL_FREE_PENDING_ELIDE) || \
    defined(H8_ENABLE_DEBUG_STATS)
  if ((atomic_load_explicit(&run->pending_bits[slot >> 6u],
                            memory_order_acquire) &
       (UINT64_C(1) << (slot & 63u))) != 0) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_free_pending_ns,
                 (size_t)(h8_medium_slots_now_ns() - section_start));
#endif
    H8_DEBUG_INC(medium_local_free_pending_nonzero);
    if (keep_empty_live) {
      H8_DEBUG_INC(medium_local_fast_pending_block);
    }
    return false;
  }
#endif
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_free_pending_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
#if defined(H8_MEDIUM_ENABLE_LOCAL_FAST_TIER)
  if (keep_empty_live &&
      h8_medium_local_fast_store_active_free(run, slot, bit)) {
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_free_slot_store_ns,
                 (size_t)(h8_medium_slots_now_ns() - section_start));
    section_start = h8_medium_slots_now_ns();
#endif
    H8_DEBUG_INC(medium_free_scaffold_count);
    H8_DEBUG_INC(medium_free_slot_count);
#if defined(H8_ENABLE_DEBUG_STATS)
    H8_DEBUG_ADD(medium_free_slot_ns,
                 (size_t)(h8_medium_slots_now_ns() - start));
#endif
    return true;
  }
#endif
  if (keep_empty_live) {
    h8_medium_debug_note_local_fast_eligible_free(run, bit);
  } else {
    H8_DEBUG_INC(medium_local_fast_not_active_run);
  }
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                        memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_free_slot_store_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
  run->allocated_mask &= ~bit;
  run->free_mask |= bit;
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_free_mask_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  section_start = h8_medium_slots_now_ns();
#endif
  if (keep_empty_live && run->allocated_mask == 0) {
    h8_medium_note_active_live_empty_fast(run);
  } else {
    h8_medium_mark_empty_locked(run);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_free_empty_ns,
               (size_t)(h8_medium_slots_now_ns() - section_start));
  H8_DEBUG_INC(medium_free_scaffold_count);
  H8_DEBUG_INC(medium_free_slot_count);
  H8_DEBUG_ADD(medium_free_slot_ns,
               (size_t)(h8_medium_slots_now_ns() - start));
#endif
  return true;
}
