#ifndef HZ10_PUBLIC_ENTRY_OWNER_H
#define HZ10_PUBLIC_ENTRY_OWNER_H

#include "hz10_class_pages.h"
#include "hz10_size_class.h"

#include <stdatomic.h>
#include <stdint.h>

#ifndef HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
#define HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION 0
#endif

#ifndef HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION
#define HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION 0
#endif

#if HZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION && !HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
#error "HZ10 partial orphan adoption requires HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION"
#endif

typedef struct Hz10ClassState {
  Hz10FreelistPage* active;
  Hz10ClassPageList list;
#if HZ10_ENABLE_TWO_SLOT_STATS
  Hz10FreelistPage* prior_active;
  uint64_t ops_since_activate;
#endif
} Hz10ClassState;

typedef struct Hz10OwnerRecord {
  _Atomic(uint32_t) state;
} Hz10OwnerRecord;

typedef struct Hz10ThreadOwner {
  Hz10OwnerRecord* record;
  Hz10ClassState classes[HZ10_CLASS_COUNT];
} Hz10ThreadOwner;

typedef struct Hz10OrphanAdoptionClassStats {
  uint64_t published_pages;
  uint64_t pop_count;
  uint64_t adopt_count;
  uint64_t reject_class_count;
  uint64_t reject_state_count;
  uint64_t reject_no_capacity_count;
  uint64_t repush_count;
  uint64_t depth;
  uint64_t max_depth;
} Hz10OrphanAdoptionClassStats;

typedef struct Hz10OrphanRegistryProbeClassStats {
  uint64_t depth;
  uint64_t idle_pages;
  uint64_t live_pinned_pages;
  uint64_t hidden_pending_slots;
  uint64_t age_lt_100ms;
  uint64_t age_lt_1s;
  uint64_t age_lt_4s;
  uint64_t age_lt_16s;
  uint64_t age_ge_16s;
} Hz10OrphanRegistryProbeClassStats;

typedef struct Hz10OrphanRegistryDrainProbeClassStats {
  uint64_t depth;
  uint64_t already_idle_pages;
  uint64_t drain_idle_pages;
  uint64_t drain_capacity_pages;
  uint64_t truly_live_pinned_pages;
  uint64_t skipped_live_owner_pages;
  uint64_t pending_before_slots;
  uint64_t merged_slots;
} Hz10OrphanRegistryDrainProbeClassStats;

#define HZ10_THREAD_OWNER_STATE_LIVE 1u
#define HZ10_THREAD_OWNER_STATE_EXITED 2u

extern __attribute__((visibility("hidden"))) _Thread_local
    Hz10ThreadOwner* hz10_current_owner;
__attribute__((visibility("hidden"))) Hz10ThreadOwner*
hz10_public_entry_current_owner_slow(void);

static inline Hz10ThreadOwner* hz10_public_entry_current_owner(void) {
  Hz10ThreadOwner* owner = hz10_current_owner;
  return owner ? owner : hz10_public_entry_current_owner_slow();
}

static inline Hz10ThreadOwner* hz10_public_entry_current_owner_if_any(void) {
  return hz10_current_owner;
}

static inline Hz10OwnerRecord* hz10_public_entry_owner_record(
    const Hz10ThreadOwner* owner) {
  return owner ? owner->record : NULL;
}

uint32_t hz10_public_entry_owner_state(const Hz10OwnerRecord* owner);
Hz10FreelistPage* hz10_public_entry_try_adopt_orphan_active(
    uint32_t class_id, Hz10ThreadOwner* adopter);
void hz10_public_entry_orphan_adoption_class_stats(
    uint32_t class_id, Hz10OrphanAdoptionClassStats* out);
void hz10_public_entry_orphan_registry_probe_class_stats(
    uint32_t class_id, uint64_t now_ns,
    Hz10OrphanRegistryProbeClassStats* out);
void hz10_public_entry_orphan_registry_drain_probe_class_stats(
    uint32_t class_id, Hz10OrphanRegistryDrainProbeClassStats* out);
void hz10_public_entry_owner_exit_flush_front_cache(void);

#endif
