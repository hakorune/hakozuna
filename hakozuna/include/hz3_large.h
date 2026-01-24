#pragma once

#include <stddef.h>

// Large (>32KB) allocation box (mmap-backed, correctness-first)
void*  hz3_large_alloc(size_t size);
void*  hz3_large_aligned_alloc(size_t alignment, size_t size);
int    hz3_large_free(void* ptr);
size_t hz3_large_usable_size(const void* ptr);
