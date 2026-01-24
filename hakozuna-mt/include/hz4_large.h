// hz4_large.h - HZ4 Large Box API
#ifndef HZ4_LARGE_H
#define HZ4_LARGE_H

#include <stddef.h>

void* hz4_large_malloc(size_t size);
void  hz4_large_free(void* ptr);
size_t hz4_large_usable_size(void* ptr);

#endif // HZ4_LARGE_H
