#ifndef H8_MEDIUM_DOMAIN_SHADOW_H
#define H8_MEDIUM_DOMAIN_SHADOW_H

#include "h8_medium.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum H8MediumDomainKind {
  H8_MEDIUM_DOMAIN_NONE = 0,
  H8_MEDIUM_DOMAIN_RUN = 1,
  H8_MEDIUM_DOMAIN_PAGE8K = 2
} H8MediumDomainKind;

typedef struct H8MediumDomainProbe {
  H8MediumDomainKind kind;
  const void* record;
} H8MediumDomainProbe;

typedef enum H8MediumDomainAcquireResult {
  H8_MEDIUM_DOMAIN_ACQUIRE_FALLBACK = 0,
  H8_MEDIUM_DOMAIN_ACQUIRE_VALID = 1,
  H8_MEDIUM_DOMAIN_ACQUIRE_INVALID = 2
} H8MediumDomainAcquireResult;

typedef struct H8MediumDomainShadowStats {
  uint64_t lookup;
  uint64_t medium_hit;
  uint64_t page8k_hit;
  uint64_t miss;
  uint64_t kind_match;
  uint64_t kind_mismatch;
  uint64_t register_conflict;
  uint64_t medium_register;
  uint64_t medium_unregister;
  uint64_t page8k_register;
  uint64_t high_address_fallback;
  uint64_t stable_record_alloc;
  uint64_t stable_record_pool_fallback;
  uint64_t stable_live_lookup;
  uint64_t stable_closing_lookup;
  uint64_t stable_tombstone_lookup;
  uint64_t stable_backend_mismatch;
  uint64_t stable_geometry_mismatch;
  uint64_t stable_unregister_missing;
  uint64_t stable_turnover_pass;
  uint64_t stable_turnover_fail;
  uint64_t stable_record_reuse_detected;
  uint64_t stable_exact_valid;
  uint64_t stable_exact_invalid;
  uint64_t stable_exact_tombstone;
  uint64_t stable_exact_mismatch;
  uint64_t stable_slot_init;
  uint64_t stable_slot_sync;
  uint64_t stable_slot_mismatch;
  uint64_t stable_slot_bound_fallback;
  uint64_t stable_slot_final_sync;
  uint64_t stable_slot_sync_after_closing;
  uint64_t stable_slot_mirror_alloc_authority_free;
  uint64_t stable_slot_mirror_free_authority_alloc;
  uint64_t stable_slot_other_mismatch;
  uint64_t stable_slot_note_without_record;
  uint64_t stable_pending_init;
  uint64_t stable_pending_sync;
  uint64_t stable_pending_final_sync;
  uint64_t stable_pending_mismatch;
  uint64_t stable_pending_sync_after_closing;
  uint64_t stable_pending_note_without_record;
  uint64_t stable_lock_acquire;
  uint64_t stable_lock_fallback;
  uint64_t stable_unlock_mismatch;
  uint64_t stable_unregister_lock;
  uint64_t stable_pool_current_bytes;
  uint64_t stable_pool_peak_bytes;
} H8MediumDomainShadowStats;

void h8_medium_domain_shadow_register_medium(H8MediumRun* run);
void h8_medium_domain_shadow_unregister_medium(H8MediumRun* run);
void h8_medium_domain_shadow_register_page8k(void* base, const void* record);
H8MediumDomainProbe h8_medium_domain_shadow_lookup(const void* ptr);
void h8_medium_domain_stable_compare(H8MediumDomainProbe probe,
                                     H8MediumRun* authority);
bool h8_medium_domain_stable_turnover_test(size_t iterations);
bool h8_medium_domain_stable_exact_compare(H8MediumDomainProbe probe,
                                           const void* ptr,
                                           bool authority_exact);
void h8_medium_domain_stable_slot_note(H8MediumRun* run, size_t slot,
                                       uint32_t committed_state);
void h8_medium_domain_stable_pending_note(H8MediumRun* run);
bool h8_medium_domain_stable_lock(H8MediumRun* run);
bool h8_medium_domain_stable_unlock(H8MediumRun* run);
H8MediumDomainAcquireResult h8_medium_domain_stable_acquire_probe(
    H8MediumDomainProbe probe, const void* ptr, H8MediumRun** run_out);
void h8_medium_domain_stable_release_probe(H8MediumDomainProbe probe);
void h8_medium_domain_shadow_compare(H8MediumDomainProbe probe,
                                     H8MediumDomainKind expected);
H8MediumDomainShadowStats h8_medium_domain_shadow_stats(void);

#endif
