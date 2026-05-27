#ifndef HZ6_ALLOCATOR_API_SCAVENGE_H
#define HZ6_ALLOCATOR_API_SCAVENGE_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t hz6_allocator_scavenge_orphans(Hz6Allocator* allocator,
                                      size_t max_bytes);

size_t hz6_allocator_scavenge_local_free(Hz6Allocator* allocator,
                                         size_t max_bytes);

size_t hz6_allocator_scavenge_profile(Hz6Allocator* allocator);

size_t hz6_allocator_drain_remote_pending(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
