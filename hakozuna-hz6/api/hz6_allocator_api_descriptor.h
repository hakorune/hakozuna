#ifndef HZ6_ALLOCATOR_API_DESCRIPTOR_H
#define HZ6_ALLOCATOR_API_DESCRIPTOR_H

#include <stddef.h>

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator);

static inline size_t hz6_allocator_descriptor_index(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor ||
      descriptor < allocator->descriptors ||
      descriptor >= allocator->descriptors + HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    return HZ6_OBJECT_DESCRIPTOR_CAPACITY;
  }
  return (size_t)(descriptor - allocator->descriptors);
}

static inline uint32_t hz6_descriptor_pack_owner16(Hz6OwnerToken owner) {
  if (owner.slot > UINT16_MAX || owner.generation > UINT16_MAX) {
    return 0;
  }
  return ((uint32_t)owner.generation << 16) | (uint32_t)owner.slot;
}

static inline Hz6OwnerToken hz6_descriptor_unpack_owner16(uint32_t packed) {
  Hz6OwnerToken owner;
  owner.slot = packed & 0xffffu;
  owner.generation = (packed >> 16) & 0xffffu;
  return owner;
}

static inline Hz6OwnerToken hz6_allocator_descriptor_owner(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1
  size_t index = hz6_allocator_descriptor_index(allocator, descriptor);
  if (index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    return (Hz6OwnerToken){0};
  }
  return hz6_descriptor_unpack_owner16(
      allocator->descriptor_side_owner16[index]);
#else
  (void)allocator;
  return descriptor ? descriptor->owner : (Hz6OwnerToken){0};
#endif
}

static inline void hz6_allocator_set_descriptor_owner(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner) {
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1
  size_t index = hz6_allocator_descriptor_index(allocator, descriptor);
  if (index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    return;
  }
  allocator->descriptor_side_owner16[index] =
      hz6_descriptor_pack_owner16(owner);
#else
  (void)allocator;
  if (descriptor) {
    descriptor->owner = owner;
  }
#endif
}

static inline int hz6_allocator_descriptor_owner_equal(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner) {
  return hz6_owner_equal(hz6_allocator_descriptor_owner(allocator, descriptor),
                         owner);
}

void hz6_allocator_reset_descriptor_available(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

int hz6_allocator_activate_descriptor(Hz6Allocator* allocator,
                                      Hz6ObjectDescriptor* descriptor,
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
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

int hz6_allocator_descriptor_source_meta(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    void** source_ptr,
    size_t* source_bytes,
    Hz6SourceReleaseFn* source_release);

int hz6_allocator_descriptor_has_source_release(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor);

int hz6_allocator_detach_descriptor_keep_source_slot(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

int hz6_allocator_reserve_descriptor_keep_source_slot(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

void hz6_allocator_note_descriptor_frontcache_reuse_dryrun(
    Hz6Allocator* allocator,
    uint16_t requested_class_id);

void hz6_allocator_note_descgov_descriptor_fail(
    Hz6Allocator* allocator,
    uint16_t requested_class_id);

int hz6_allocator_descgov_descriptor_available(
    const Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
