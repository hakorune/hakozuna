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
  if (!run || !run->base || !ptr || run->slot_size == 0u ||
      run->slot_count == 0u) {
    return false;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t addr = (uintptr_t)ptr;
  if (addr < base) {
    return false;
  }
  uintptr_t offset = addr - base;
  size_t payload = (size_t)run->slot_size * (size_t)run->slot_count;
  uintptr_t slot_mask = ((uintptr_t)1u << run->slot_shift) - 1u;
  if (offset >= payload || (offset & slot_mask) != 0u) {
    return false;
  }
  size_t slot = (size_t)(offset >> run->slot_shift);
  if (slot >= run->slot_count) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot) {
  return h8_medium_slot_ptr_fast(run, slot);
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
  h8_medium_mark_live_on_alloc_fast(run);
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_alloc_slot_count);
  H8_DEBUG_ADD(medium_alloc_slot_ns,
               (size_t)(h8_medium_slots_now_ns() - start));
#endif
  return h8_medium_slot_ptr_fast(run, slot);
}

bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr,
                                       bool keep_empty_live) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_slots_now_ns();
#endif
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((run->allocated_mask & bit) == 0 || (run->free_mask & bit) != 0) {
    return false;
  }
#if !defined(H8_MEDIUM_LOCAL_FREE_PENDING_ELIDE) || \
    defined(H8_ENABLE_DEBUG_STATS)
  if ((atomic_load_explicit(&run->pending_bits[slot >> 6u],
                            memory_order_acquire) &
       (UINT64_C(1) << (slot & 63u))) != 0) {
    H8_DEBUG_INC(medium_local_free_pending_nonzero);
    return false;
  }
#endif
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_FREE | H8_SLOT_NONE,
                        memory_order_release);
  run->allocated_mask &= ~bit;
  run->free_mask |= bit;
  if (keep_empty_live && run->allocated_mask == 0) {
    h8_medium_note_active_live_empty(run);
  } else {
    h8_medium_mark_empty_locked(run);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_free_slot_count);
  H8_DEBUG_ADD(medium_free_slot_ns,
               (size_t)(h8_medium_slots_now_ns() - start));
#endif
  return true;
}
