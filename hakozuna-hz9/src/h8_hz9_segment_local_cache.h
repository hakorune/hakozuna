#ifndef H8_HZ9_SEGMENT_LOCAL_CACHE_H
#define H8_HZ9_SEGMENT_LOCAL_CACHE_H

#include "../include/h8.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(H9_SEGMENT_LOCAL_CACHE_L0)
void h9_segment_local_cache_debug_reset(void);
size_t h9_segment_local_cache_debug_state_size(void);
bool h9_segment_local_cache_debug_put(uint32_t class_id, uint32_t slot);
bool h9_segment_local_cache_debug_take(uint32_t class_id,
                                       uint32_t* slot_out);
bool h9_segment_local_cache_debug_free_allocated(uint32_t class_id,
                                                 uint32_t slot);
bool h9_segment_local_cache_debug_remote_mark(uint32_t class_id,
                                              uint32_t slot);
bool h9_segment_local_cache_debug_drain_remote(uint32_t class_id,
                                               uint64_t* pending_out);
bool h9_segment_local_cache_debug_retire(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_release_all(void);
bool h9_segment_local_cache_debug_class_geometry(uint32_t class_id,
                                                 uint32_t* slot_size_out,
                                                 uint32_t* run_size_out,
                                                 uint16_t* slot_count_out);
bool h9_segment_local_cache_debug_class_capacity(uint32_t class_id,
                                                 size_t* payload_bytes_out,
                                                 size_t* slack_bytes_out);
H8RouteKind h9_segment_local_cache_debug_route_offset(uint32_t class_id,
                                                      size_t offset);
bool h9_segment_local_cache_debug_bind_base(uint32_t class_id,
                                            uintptr_t base_addr);
H8RouteKind h9_segment_local_cache_debug_route_addr(uint32_t class_id,
                                                    uintptr_t addr);
uint32_t h9_segment_local_cache_debug_state(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_free_bits(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_alloc_bits(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_remote_bits(uint32_t class_id);
uint64_t h9_segment_local_cache_debug_touched_classes(void);
#endif

#endif
