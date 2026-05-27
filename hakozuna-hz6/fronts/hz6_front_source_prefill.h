#ifndef HZ6_FRONT_SOURCE_PREFILL_H
#define HZ6_FRONT_SOURCE_PREFILL_H

#include "../api/hz6_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t hz6_front_prefill_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind,
                                     size_t count);

#ifdef __cplusplus
}
#endif

#endif
