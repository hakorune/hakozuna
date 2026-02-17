// hz4_large.h - HZ4 Large Box API
#ifndef HZ4_LARGE_H
#define HZ4_LARGE_H

#include <stddef.h>
#include <stdint.h>

#include "hz4_sizeclass.h"

// Header bytes for large allocation user-pointer offset.
// Keep this in one place so free-route guards do not duplicate ad-hoc constants.
#define HZ4_LARGE_HEADER_FIELDS_BYTES (sizeof(uint32_t) * 2u + sizeof(size_t) * 2u)
#define HZ4_LARGE_HEADER_BYTES \
    ((((HZ4_LARGE_HEADER_FIELDS_BYTES) + (HZ4_SIZE_ALIGN - 1u)) / HZ4_SIZE_ALIGN) * HZ4_SIZE_ALIGN)

void* hz4_large_malloc(size_t size);
void  hz4_large_free(void* ptr);
size_t hz4_large_usable_size(void* ptr);
int hz4_large_header_valid(void* ptr);
int hz4_large_try_free(void* ptr);

#endif // HZ4_LARGE_H
