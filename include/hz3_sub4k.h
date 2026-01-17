#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hz3_config.h"
#include "hz3_types.h"

// Sub-4KB size classes for 2049..4095 (4 classes).
int hz3_sub4k_sc_from_size(size_t size);
size_t hz3_sub4k_sc_to_size(int sc);

void* hz3_sub4k_alloc(size_t size);
void  hz3_sub4k_bin_flush(int sc, Hz3Bin* bin);
void  hz3_sub4k_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);

// S40: central push list (exposed for SoA destructor)
void  hz3_sub4k_central_push_list(uint8_t shard, int sc, void* head, void* tail, uint32_t n);

// S62-1b: External APIs for sub4k retire (with validation guards)
uint32_t hz3_sub4k_central_pop_batch_ext(uint8_t shard, int sc, void** out, uint32_t want);
void hz3_sub4k_central_push_list_ext(uint8_t shard, int sc, void* head, void* tail, uint32_t n);
size_t hz3_sub4k_run_capacity(int sc);
