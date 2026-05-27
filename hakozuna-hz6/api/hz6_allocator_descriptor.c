#include "hz6_allocator.h"

static Hz6OwnerToken hz6_allocator_descriptor_owner_none(void) {
  Hz6OwnerToken token;
  token.slot = 0;
  token.generation = 0;
  return token;
}

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return NULL;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    if (!allocator->descriptors[i].ptr) {
      return &allocator->descriptors[i];
    }
  }
  return NULL;
}

int hz6_allocator_activate_descriptor(Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner) {
  if (!descriptor || descriptor->state != expected) {
    return 0;
  }
  if (descriptor->ptr != ptr || descriptor->generation != generation) {
    return 0;
  }
  descriptor->state = HZ6_STATE_ACTIVE;
  descriptor->owner = owner;
  return 1;
}

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
    Hz6ObjectState state) {
  if (!allocator || !descriptor || !ptr || bytes == 0 ||
      source_kind == HZ6_SOURCE_NONE) {
    return 0;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = bytes;
  descriptor->source_ptr = source_ptr ? source_ptr : ptr;
  descriptor->source_bytes = source_bytes ? source_bytes : bytes;
  descriptor->source_block = source_block;
  descriptor->class_id = class_id;
  descriptor->source_kind = source_kind;
  descriptor->source_release = source_release;
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = state;
  return 1;
}

int hz6_allocator_cache_active_descriptor(Hz6Allocator* allocator,
                                          Hz6ObjectDescriptor* descriptor,
                                          void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
    return 0;
  }

  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = descriptor->class_id;
  entry.generation = descriptor->generation;
  if (hz6_allocator_frontcache_push(allocator, entry.class_id, entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_DEAD;
  hz6_allocator_route_unregister_exact(allocator, ptr);
  hz6_allocator_release_descriptor_source(descriptor);
  return 1;
}

int hz6_allocator_remote_free_active_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  if (hz6_allocator_profile_strict_owner_remote(allocator)) {
    if (!hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
      return 0;
    }
    descriptor->state = HZ6_STATE_REMOTE_PENDING;
    return 1;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;

  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  descriptor->owner = hz6_allocator_descriptor_owner_none();
  if (!hz6_allocator_transfer_push(allocator, object)) {
    descriptor->state = HZ6_STATE_ACTIVE;
    descriptor->owner = allocator->owner.token;
    return 0;
  }

  hz6_allocator_note_transfer_push(allocator);
  return 1;
}

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->ptr) {
    return 0;
  }

  int released = 0;
  if (descriptor->source_block) {
    released = hz6_allocator_release_source_block(descriptor->source_block);
  } else {
    void* source_ptr = descriptor->source_ptr ? descriptor->source_ptr
                                              : descriptor->ptr;
    if (descriptor->source_release) {
      released =
          descriptor->source_release(source_ptr, descriptor->source_bytes);
    } else {
      released = hz6_source_system_release(source_ptr, descriptor->bytes);
    }
  }

  descriptor->ptr = NULL;
  descriptor->bytes = 0;
  descriptor->source_ptr = NULL;
  descriptor->source_bytes = 0;
  descriptor->source_block = NULL;
  descriptor->class_id = 0;
  descriptor->source_kind = HZ6_SOURCE_NONE;
  descriptor->source_release = NULL;
  descriptor->owner = hz6_allocator_descriptor_owner_none();
  descriptor->generation = 0;
  descriptor->state = HZ6_STATE_DEAD;
  return released;
}
