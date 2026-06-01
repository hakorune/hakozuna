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

int hz6_allocator_release_source_block(Hz6SourceBlock* block);

void hz6_allocator_note_source_run_reuse_dryrun(Hz6Allocator* allocator,
                                                Hz6SourceKind source_kind,
                                                size_t block_bytes,
                                                size_t slot_bytes);

#ifdef __cplusplus
}
#endif

#endif
