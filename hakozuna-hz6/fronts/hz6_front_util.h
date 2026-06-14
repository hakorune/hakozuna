#ifndef HZ6_FRONT_UTIL_H
#define HZ6_FRONT_UTIL_H

#include "../api/hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void* hz6_front_reuse_transfer(Hz6Allocator* allocator,
                               uint16_t front_id,
                               uint16_t class_id,
                               Hz6AllocPath* path);

/* Transfer-first reuse helper.  Returns NULL unless the allocator profile
 * allows transfer-first reuse and a matching transfer object activates.  When
 * non-NULL, out_descriptor receives the activated descriptor after any rehome. */
void* hz6_front_reuse_transfer_with_descriptor(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6AllocPath* path,
    Hz6ObjectDescriptor** out_descriptor);

void* hz6_front_reuse_cached_or_transfer(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         Hz6AllocPath* path);

void* hz6_front_reuse_transfer_or_cached(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         Hz6AllocPath* path);

void* hz6_front_reuse_transfer_or_cached_with_descriptor(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6AllocPath* path,
    Hz6ObjectDescriptor** out_descriptor);

void* hz6_front_reuse_cached_with_descriptor(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6AllocPath* path,
    Hz6ObjectDescriptor** out_descriptor);

int hz6_front_free_local_to_cache(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route,
                                  uint16_t expected_class_id);

int hz6_front_free_remote_to_transfer(Hz6Allocator* allocator,
                                      void* ptr,
                                      Hz6RouteResult route,
                                      uint16_t expected_class_id);

#ifdef __cplusplus
}
#endif

#endif
