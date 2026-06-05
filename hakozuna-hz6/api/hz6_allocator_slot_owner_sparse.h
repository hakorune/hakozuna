#ifndef HZ6_ALLOCATOR_SLOT_OWNER_SPARSE_H
#define HZ6_ALLOCATOR_SLOT_OWNER_SPARSE_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_allocator_elastic_slot_owner_sparse_note(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor);

void hz6_allocator_elastic_slot_owner_consumer_dryrun(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken expected_owner,
    int existing_equal);

int hz6_allocator_elastic_slot_owner_logical_owner_match(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken expected_owner);

#ifdef __cplusplus
}
#endif

#endif
