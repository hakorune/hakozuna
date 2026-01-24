// hz4_tcache.h - HZ4 TCache Box API
#ifndef HZ4_TCACHE_H
#define HZ4_TCACHE_H

#include <stddef.h>

void* hz4_malloc(size_t size);
void  hz4_free(void* ptr);
size_t hz4_small_usable_size(void* ptr);

#endif // HZ4_TCACHE_H
