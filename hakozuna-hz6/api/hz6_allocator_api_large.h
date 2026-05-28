#ifndef HZ6_ALLOCATOR_API_LARGE_H
#define HZ6_ALLOCATOR_API_LARGE_H

#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hz6_allocator_large_span_pool_init(Hz6Allocator* allocator);

void hz6_allocator_note_large_span_central_push(Hz6Allocator* allocator);

void hz6_allocator_note_large_span_central_pop(Hz6Allocator* allocator);

void hz6_allocator_note_source_alloc_for_front(Hz6Allocator* allocator,
                                               uint16_t front_id);

int hz6_allocator_large_span_pool_push(Hz6Allocator* allocator,
                                       Hz6ObjectDescriptor* descriptor);

int hz6_allocator_large_span_pool_pop(Hz6Allocator* allocator,
                                      uint16_t class_id,
                                      Hz6ObjectDescriptor** out);

size_t hz6_allocator_large_span_pool_count(const Hz6Allocator* allocator,
                                           uint16_t class_id);

size_t hz6_allocator_large_span_pool_capacity(uint16_t class_id);

#ifdef __cplusplus
}
#endif

#endif
