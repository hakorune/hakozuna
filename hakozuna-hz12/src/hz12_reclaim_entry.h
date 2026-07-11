#ifndef HZ12_RECLAIM_ENTRY_H
#define HZ12_RECLAIM_ENTRY_H

#include "hz12_owner_registry.h"

void h12_reclaim_entry_reset(void);
int h12_reclaim_entry_begin(H12OwnerToken owner);
int h12_reclaim_entry_end(H12OwnerToken owner);

#endif /* HZ12_RECLAIM_ENTRY_H */
