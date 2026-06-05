#ifndef HZ6_ALLOCATOR_API_SOURCE_H
#define HZ6_ALLOCATOR_API_SOURCE_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6SourceBlock* hz6_allocator_create_source_block(
    Hz6Allocator* allocator,
    size_t bytes,
    const Hz6OsMemoryOps* source_ops,
    Hz6SourceKind source_kind);

int hz6_allocator_retain_source_block(Hz6SourceBlock* block);

int hz6_allocator_release_source_block(Hz6Allocator* allocator,
                                       Hz6SourceBlock* block);

int hz6_allocator_source_block_is_elastic_depot(
    const Hz6SourceBlock* block);

int hz6_allocator_source_run_init(Hz6SourceBlock* block,
                                  uint16_t class_id,
                                  size_t slot_bytes);

Hz6SourceBlock* hz6_allocator_source_run_find_reusable(
    Hz6Allocator* allocator,
    Hz6SourceKind source_kind,
    size_t block_bytes,
    uint16_t class_id,
    size_t slot_bytes);

int hz6_allocator_source_run_reserve_slot(Hz6SourceBlock* block,
                                          size_t* slot_index);

void hz6_allocator_source_run_commit_slot(Hz6SourceBlock* block,
                                          size_t slot_index);

void hz6_allocator_source_run_rollback_slot(Hz6SourceBlock* block,
                                            size_t slot_index);

void hz6_allocator_source_run_release_slot(Hz6SourceBlock* block,
                                           void* ptr);

void hz6_allocator_note_source_run_reuse_dryrun(Hz6Allocator* allocator,
                                                Hz6SourceKind source_kind,
                                                size_t block_bytes,
                                                size_t slot_bytes);

#ifdef __cplusplus
}
#endif

#endif
