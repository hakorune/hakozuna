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
uint32_t h9_segment_entry_debug_prepare_active(uint32_t class_id);
bool h9_segment_entry_debug_cycle_page(uint32_t page_id, void** ptr_out);
uintptr_t h9_segment_entry_debug_prepare_handle(uint32_t class_id);
bool h9_segment_entry_debug_cycle_handle(uintptr_t handle, void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_handle(uint32_t class_id,
                                             void** ptr_out);
bool h9_segment_entry_debug_alloc_tls_handle(uint32_t class_id,
                                             void** ptr_out);
bool h9_segment_entry_debug_free_tls_handle(uint32_t class_id, void* ptr,
                                            bool* owned_out);
bool h9_segment_entry_debug_alloc_tls_slot(uint32_t class_id, void** ptr_out,
                                           uint32_t* slot_out);
bool h9_segment_entry_debug_free_tls_slot(uint32_t class_id, uint32_t slot,
                                          bool* owned_out);
bool h9_segment_entry_debug_cycle_tls_checked(uint32_t class_id,
                                              void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_checked_touch(uint32_t class_id,
                                                    uint64_t value,
                                                    bool touch,
                                                    void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_epoch_body(uint32_t class_id,
                                                 uint64_t value,
                                                 bool touch,
                                                 void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_route_body(uint32_t class_id,
                                                 uint64_t value,
                                                 bool touch,
                                                 void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_route_every(uint32_t class_id,
                                                  uint64_t value,
                                                  bool touch,
                                                  uint32_t route_period,
                                                  void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_route64_body(uint32_t class_id,
                                                   uint64_t value,
                                                   bool touch,
                                                   void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_cache(uint32_t class_id, uint64_t value,
                                            bool touch, void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_ledger(uint32_t class_id, uint64_t value,
                                             bool touch, void** ptr_out);
bool h9_segment_entry_debug_cycle_tls_ledger_body(uint32_t class_id,
                                                  uint64_t value, bool touch,
                                                  void** ptr_out);
H8RouteKind h9_segment_entry_debug_route(void* ptr);
uint64_t h9_segment_entry_debug_free_bits(uint32_t page_id);
uint64_t h9_segment_entry_debug_alloc_bits(uint32_t page_id);
uint32_t h9_segment_entry_debug_page_count(void);
#endif

#endif
