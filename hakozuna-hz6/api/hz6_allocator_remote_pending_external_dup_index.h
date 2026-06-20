#ifndef HZ6_ALLOCATOR_REMOTE_PENDING_EXTERNAL_DUP_INDEX_H
#define HZ6_ALLOCATOR_REMOTE_PENDING_EXTERNAL_DUP_INDEX_H

#include "hz6_allocator.h"

#include <stddef.h>
#include <stdint.h>

void hz6_remote_pending_external_dup_index_init(Hz6Allocator* allocator);

int hz6_remote_pending_external_dup_index_find(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation,
    uint32_t* out_ticket_index,
    size_t* out_probes);

int hz6_remote_pending_external_dup_index_insert(Hz6Allocator* allocator,
                                                 uint32_t ticket_index);

int hz6_remote_pending_external_dup_index_remove(
    Hz6Allocator* allocator,
    uint32_t ticket_index,
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation);

#endif
