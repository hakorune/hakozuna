#ifndef H8_HZ9_SEGMENT_ENTRY_H
#define H8_HZ9_SEGMENT_ENTRY_H

#include "../include/h8.h"

#include <stdbool.h>
#include <stdint.h>

#if defined(H9_SEGMENT_ENTRY_L1)
void h9_segment_entry_debug_reset(void);
bool h9_segment_entry_debug_alloc(uint32_t class_id, void** ptr_out);
bool h9_segment_entry_debug_free(void* ptr, bool* owned_out);
bool h9_segment_entry_debug_cycle_fused(uint32_t class_id, void** ptr_out);
bool h9_segment_entry_debug_cycle_active_fast(uint32_t class_id,
                                              void** ptr_out);
H8RouteKind h9_segment_entry_debug_route(void* ptr);
uint64_t h9_segment_entry_debug_free_bits(uint32_t page_id);
uint64_t h9_segment_entry_debug_alloc_bits(uint32_t page_id);
uint32_t h9_segment_entry_debug_page_count(void);
#endif

#endif
