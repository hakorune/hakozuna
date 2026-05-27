#ifndef HZ6_ALLOCATOR_DESTROY_INTERNAL_H
#define HZ6_ALLOCATOR_DESTROY_INTERNAL_H

#include "hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_allocator_destroy_descriptors(Hz6Allocator* allocator);

void hz6_allocator_destroy_source_blocks(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
