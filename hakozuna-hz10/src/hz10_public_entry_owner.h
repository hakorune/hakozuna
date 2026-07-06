#ifndef HZ10_PUBLIC_ENTRY_OWNER_H
#define HZ10_PUBLIC_ENTRY_OWNER_H

#include "hz10_class_pages.h"
#include "hz10_size_class.h"

#include <stdint.h>

#ifndef HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
#define HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION 0
#endif

#if HZ10_ENABLE_FRONT_CACHE && HZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION
#error "HZ10 orphan active adoption does not yet support front cache handoff"
#endif

typedef struct Hz10ClassState {
  Hz10FreelistPage* active;
  Hz10ClassPageList list;
#if HZ10_ENABLE_TWO_SLOT_STATS
  Hz10FreelistPage* prior_active;
  uint64_t ops_since_activate;
#endif
} Hz10ClassState;

typedef struct Hz10ThreadOwner {
  Hz10ClassState classes[HZ10_CLASS_COUNT];
} Hz10ThreadOwner;

Hz10ThreadOwner* hz10_public_entry_current_owner(void);
Hz10ThreadOwner* hz10_public_entry_current_owner_if_any(void);
Hz10FreelistPage* hz10_public_entry_try_adopt_orphan_active(
    uint32_t class_id, Hz10ThreadOwner* adopter);

#endif
