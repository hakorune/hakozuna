#ifndef HZ6_ALLOCATOR_API_DESCRIPTOR_H
#define HZ6_ALLOCATOR_API_DESCRIPTOR_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor);

#ifdef __cplusplus
}
#endif

#endif
