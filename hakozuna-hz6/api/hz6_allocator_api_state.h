#ifndef HZ6_ALLOCATOR_API_STATE_H
#define HZ6_ALLOCATOR_API_STATE_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id);

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator);

int hz6_allocator_activate_descriptor(Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner);

int hz6_allocator_prepare_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    size_t bytes,
    void* source_ptr,
    size_t source_bytes,
    Hz6SourceBlock* source_block,
    uint16_t class_id,
    Hz6SourceKind source_kind,
    int (*source_release)(void* ptr, size_t bytes),
    Hz6ObjectState state);

int hz6_allocator_cache_active_descriptor(Hz6Allocator* allocator,
                                          Hz6ObjectDescriptor* descriptor,
                                          void* ptr);

int hz6_allocator_remote_free_active_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr);

Hz6OwnerToken hz6_allocator_owner_token(const Hz6Allocator* allocator);

int hz6_allocator_debug_set_owner_slot(Hz6Allocator* allocator,
                                       uint32_t slot);

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor);

Hz6SourceBlock* hz6_allocator_create_source_block(
    Hz6Allocator* allocator,
    size_t bytes,
    const Hz6OsMemoryOps* source_ops,
    Hz6SourceKind source_kind);

int hz6_allocator_retain_source_block(Hz6SourceBlock* block);

int hz6_allocator_release_source_block(Hz6SourceBlock* block);

void hz6_allocator_mark_owner_dead(Hz6Allocator* allocator);

int hz6_allocator_release_orphan(Hz6Allocator* allocator, void* ptr);

int hz6_allocator_adopt_orphan(Hz6Allocator* adopter,
                               Hz6Allocator* source,
                               void* ptr);

#ifdef __cplusplus
}
#endif

#endif
