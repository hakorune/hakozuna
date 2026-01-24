#pragma once

#include "hz3_config.h"
#include <stdint.h>

#if HZ3_S82_STASH_GUARD
void hz3_s82_stash_guard_one(const char* where, void* ptr, uint32_t expect_bin);
void hz3_s82_stash_guard_list(const char* where, void* head, uint32_t expect_bin, uint32_t max_walk);
#endif
