#ifndef HZ6_ALLOCATOR_API_PROFILE_H
#define HZ6_ALLOCATOR_API_PROFILE_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6ProfileId hz6_allocator_profile_id(const Hz6Allocator* allocator);

int hz6_allocator_profile_transfer_first(const Hz6Allocator* allocator);

int hz6_allocator_profile_strict_owner_remote(const Hz6Allocator* allocator);

Hz6SourceKind hz6_allocator_profile_source_kind(
    const Hz6Allocator* allocator);

size_t hz6_allocator_profile_source_refill_batch(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

size_t hz6_allocator_control_source_refill_batch(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id);

size_t hz6_allocator_control_source_prefill_count(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested);

size_t hz6_allocator_profile_transfer_capacity(
    const Hz6Allocator* allocator);

const Hz6OsMemoryOps* hz6_allocator_source_ops(
    const Hz6Allocator* allocator,
    Hz6SourceKind source_kind);

#ifdef __cplusplus
}
#endif

#endif
