#ifndef HZ6_FRONT_UTIL_H
#define HZ6_FRONT_UTIL_H

#include "../api/hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void* hz6_front_reuse_or_source(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t bytes);

void* hz6_front_reuse_or_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind);

void* hz6_front_reuse_or_prefill_source_kind(Hz6Allocator* allocator,
                                             uint16_t front_id,
                                             uint16_t class_id,
                                             size_t bytes,
                                             Hz6SourceKind source_kind,
                                             size_t count);

void* hz6_front_reuse_or_source_ops(Hz6Allocator* allocator,
                                    uint16_t front_id,
                                    uint16_t class_id,
                                    size_t bytes,
                                    const Hz6OsMemoryOps* source_ops,
                                    Hz6SourceKind source_kind);

void* hz6_front_reuse_cached_or_transfer(Hz6Allocator* allocator,
                                         uint16_t class_id);

void* hz6_front_reuse_transfer_or_cached(Hz6Allocator* allocator,
                                         uint16_t class_id);

void* hz6_front_source_slot_kind(Hz6Allocator* allocator,
                                 uint16_t front_id,
                                 uint16_t class_id,
                                 size_t user_bytes,
                                 size_t source_bytes,
                                 size_t source_offset,
                                 Hz6SourceKind source_kind);

void* hz6_front_source_slot_ops(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t user_bytes,
                                size_t source_bytes,
                                size_t source_offset,
                                const Hz6OsMemoryOps* source_ops,
                                Hz6SourceKind source_kind);

void* hz6_front_source_block_slot(Hz6Allocator* allocator,
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t user_bytes,
                                  size_t source_offset,
                                  Hz6SourceBlock* source_block);

size_t hz6_front_prefill_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind,
                                     size_t count);

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
