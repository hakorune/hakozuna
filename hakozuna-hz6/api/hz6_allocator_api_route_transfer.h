#ifndef HZ6_ALLOCATOR_API_ROUTE_TRANSFER_H
#define HZ6_ALLOCATOR_API_ROUTE_TRANSFER_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr);

void hz6_allocator_route_visibility_register(Hz6Allocator* allocator);

void hz6_allocator_route_visibility_unregister(Hz6Allocator* allocator);

Hz6RouteResult hz6_allocator_route_lookup_visible(
    Hz6Allocator* allocator,
    const void* ptr);

Hz6RouteResult hz6_allocator_route_lookup_visible_only(
    Hz6Allocator* allocator,
    const void* ptr);

Hz6RouteResult hz6_allocator_route_lookup_visible_after_local_miss(
    Hz6Allocator* allocator,
    const void* ptr);

int hz6_allocator_route_negative_filter_skip_local(
    Hz6Allocator* allocator,
    const void* ptr);

void hz6_allocator_route_shared_directory_dryrun(Hz6Allocator* allocator,
                                                 const void* ptr);

Hz6RouteResult hz6_allocator_route_shared_directory_lookup_foreign(
    Hz6Allocator* allocator,
    const void* ptr);

int hz6_allocator_route_rehome_exact(Hz6Allocator* allocator,
                                     const Hz6RouteResult* route);

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr);

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor);

int hz6_allocator_source_block_register_invalid_range(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    uint16_t front_id,
    uint16_t class_id);

Hz6RouteBackendKind hz6_allocator_route_backend_kind(
    const Hz6Allocator* allocator);

size_t hz6_allocator_route_page_granularity(const Hz6Allocator* allocator);

Hz6TransferBackendKind hz6_allocator_transfer_backend_kind(
    const Hz6Allocator* allocator);

size_t hz6_allocator_transfer_capacity(const Hz6Allocator* allocator);

size_t hz6_allocator_transfer_count(const Hz6Allocator* allocator);

size_t hz6_allocator_transfer_count_class(const Hz6Allocator* allocator,
                                          uint16_t class_id);

size_t hz6_allocator_transfer_shard_count_at(const Hz6Allocator* allocator,
                                             size_t shard_index);

size_t hz6_allocator_transfer_shard_capacity_at(
    const Hz6Allocator* allocator,
    size_t shard_index);

int hz6_allocator_transfer_push(Hz6Allocator* allocator,
                                Hz6TransferObject object);

int hz6_allocator_transfer_pop(Hz6Allocator* allocator,
                               uint16_t class_id,
                               Hz6TransferObject* out);

void hz6_allocator_note_transfer_push(Hz6Allocator* allocator);

void hz6_allocator_note_transfer_pop(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
