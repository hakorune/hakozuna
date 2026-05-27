#ifndef HZ6_ALLOCATOR_API_ORPHAN_H
#define HZ6_ALLOCATOR_API_ORPHAN_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int hz6_allocator_release_orphan(Hz6Allocator* allocator, void* ptr);

int hz6_allocator_adopt_orphan(Hz6Allocator* adopter,
                               Hz6Allocator* source,
                               void* ptr);

#ifdef __cplusplus
}
#endif

#endif
