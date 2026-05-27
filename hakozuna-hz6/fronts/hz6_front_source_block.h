#ifndef HZ6_FRONT_SOURCE_BLOCK_H
#define HZ6_FRONT_SOURCE_BLOCK_H

#include "../api/hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void* hz6_front_source_block_slot(Hz6Allocator* allocator,
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t user_bytes,
                                  size_t source_offset,
                                  Hz6SourceBlock* source_block);

#ifdef __cplusplus
}
#endif

#endif
