#ifndef HZ6_ALLOCATOR_API_FRONT_H
#define HZ6_ALLOCATOR_API_FRONT_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int hz6_allocator_frontcache_push(Hz6Allocator* allocator,
                                  uint16_t class_id,
                                  Hz6FrontCacheEntry entry);

int hz6_allocator_frontcache_pop(Hz6Allocator* allocator,
                                 uint16_t class_id,
                                 Hz6FrontCacheEntry* out);

int hz6_allocator_frontcache_remove(Hz6Allocator* allocator,
                                    uint16_t class_id,
                                    void* ptr,
                                    void* descriptor,
                                    uint32_t generation,
                                    Hz6FrontCacheEntry* removed);

size_t hz6_allocator_frontcache_count(const Hz6Allocator* allocator,
                                      uint16_t class_id);

size_t hz6_allocator_frontcache_capacity(const Hz6Allocator* allocator,
                                         uint16_t class_id);

void hz6_allocator_note_frontcache_spill_dryrun(Hz6Allocator* allocator,
                                                uint16_t requested_class_id);

int hz6_allocator_spill_frontcache_for_descriptor(
    Hz6Allocator* allocator,
    uint16_t requested_class_id);

void hz6_allocator_note_frontcache_borrow_dryrun(Hz6Allocator* allocator,
                                                 uint16_t front_id,
                                                 uint16_t requested_class_id,
                                                 size_t requested_bytes);

void* hz6_allocator_borrow_larger_frontcache(Hz6Allocator* allocator,
                                             uint16_t front_id,
                                             uint16_t requested_class_id,
                                             size_t requested_bytes);

void hz6_allocator_note_frontcache_cap_dryrun(Hz6Allocator* allocator,
                                              uint16_t class_id);

int hz6_allocator_frontcache_should_cap_release(Hz6Allocator* allocator,
                                                uint16_t class_id);

size_t hz6_allocator_prefill_size(Hz6Allocator* allocator,
                                  size_t size,
                                  size_t count);

size_t hz6_allocator_prefill_front(Hz6Allocator* allocator,
                                   uint16_t front_id,
                                   size_t count);

size_t hz6_allocator_prefill_front_class(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         size_t count);

void hz6_allocator_note_source_alloc(Hz6Allocator* allocator);

void hz6_allocator_note_source_alloc_for_front(Hz6Allocator* allocator,
                                               uint16_t front_id);

#ifdef __cplusplus
}
#endif

#endif
