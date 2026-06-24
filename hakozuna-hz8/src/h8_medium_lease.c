#include "h8_internal.h"

#include <stdlib.h>

#if defined(H8_ENABLE_DEBUG_STATS)
#define H8_MEDIUM_LEASE_REFS_MASK UINT32_C(0x7fff)
#define H8_MEDIUM_LEASE_CLOSED UINT32_C(0x8000)
#define H8_MEDIUM_LEASE_GENERATION_SHIFT 16u

static uint32_t h8_medium_lease_pack(uint16_t generation, bool closed,
                                     uint32_t refs) {
  uint32_t raw = ((uint32_t)generation << H8_MEDIUM_LEASE_GENERATION_SHIFT) |
                 (refs & H8_MEDIUM_LEASE_REFS_MASK);
  if (closed) {
    raw |= H8_MEDIUM_LEASE_CLOSED;
  }
  return raw;
}

static uint32_t h8_medium_lease_refs(uint32_t raw) {
  return raw & H8_MEDIUM_LEASE_REFS_MASK;
}

static uint16_t h8_medium_lease_generation(uint32_t raw) {
  return (uint16_t)(raw >> H8_MEDIUM_LEASE_GENERATION_SHIFT);
}
#endif

void h8_medium_owner_lease_shadow_open(H8OwnerRecord* owner,
                                       uint16_t generation) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t old =
      atomic_load_explicit(&owner->medium_publish_ctl, memory_order_acquire);
  if (h8_medium_lease_refs(old) != 0) {
    H8_DEBUG_INC(medium_owner_reuse_with_medium_refs);
  }
  atomic_store_explicit(&owner->medium_publish_ctl,
                        h8_medium_lease_pack(generation, false, 0),
                        memory_order_release);
#else
  (void)owner;
  (void)generation;
#endif
}

void h8_medium_owner_lease_shadow_close(H8OwnerRecord* owner) {
#if defined(H8_ENABLE_DEBUG_STATS)
  atomic_fetch_or_explicit(&owner->medium_publish_ctl, H8_MEDIUM_LEASE_CLOSED,
                           memory_order_acq_rel);
#else
  (void)owner;
#endif
}

bool h8_medium_owner_lease_shadow_enter(H8OwnerRecord* owner,
                                        uint16_t generation) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t cur =
      atomic_load_explicit(&owner->medium_publish_ctl, memory_order_acquire);
  for (;;) {
    if (h8_medium_lease_generation(cur) != generation) {
      return false;
    }
    if (cur & H8_MEDIUM_LEASE_CLOSED) {
      H8_DEBUG_INC(medium_lease_enter_after_close);
      return false;
    }
    uint32_t refs = h8_medium_lease_refs(cur);
    if (refs == H8_MEDIUM_LEASE_REFS_MASK) {
      abort();
    }
    uint32_t next = cur + 1u;
    if (atomic_compare_exchange_weak_explicit(
            &owner->medium_publish_ctl, &cur, next, memory_order_acq_rel,
            memory_order_acquire)) {
      return true;
    }
  }
#else
  (void)owner;
  (void)generation;
  return true;
#endif
}

void h8_medium_owner_lease_shadow_exit(H8OwnerRecord* owner) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t cur =
      atomic_load_explicit(&owner->medium_publish_ctl, memory_order_acquire);
  for (;;) {
    uint32_t refs = h8_medium_lease_refs(cur);
    if (refs == 0) {
      H8_DEBUG_INC(medium_lease_ref_underflow);
      return;
    }
    uint32_t next = cur - 1u;
    if (atomic_compare_exchange_weak_explicit(
            &owner->medium_publish_ctl, &cur, next, memory_order_acq_rel,
            memory_order_acquire)) {
      return;
    }
  }
#else
  (void)owner;
#endif
}

void h8_medium_owner_lease_shadow_check_exit(H8OwnerRecord* owner) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint32_t raw =
      atomic_load_explicit(&owner->medium_publish_ctl, memory_order_acquire);
  if (h8_medium_lease_refs(raw) != 0) {
    H8_DEBUG_INC(medium_lease_ref_nonzero_at_owner_exit);
  }
#else
  (void)owner;
#endif
}
