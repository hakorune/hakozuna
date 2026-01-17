#pragma once

#include <stddef.h>

// hz3 public API (internal to this repo; used by hz3_shim.c).
void* hz3_malloc(size_t size);
void  hz3_free(void* ptr);
void* hz3_calloc(size_t nmemb, size_t size);
void* hz3_realloc(void* ptr, size_t size);

// Get usable size of hz3-allocated pointer (Day 8: hybrid shim support)
size_t hz3_usable_size(void* ptr);

