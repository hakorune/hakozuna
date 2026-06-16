#ifndef HZ6_ALLOCATOR_SOURCE_RUN_H
#define HZ6_ALLOCATOR_SOURCE_RUN_H

#include "hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_source_run_reset(Hz6SourceBlock* block);
int hz6_source_run_descriptor_map_init(Hz6SourceBlock* block);

#ifdef __cplusplus
}
#endif

#endif
