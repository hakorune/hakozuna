#ifndef HZ5_H
#define HZ5_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* hz5_p1_alloc_aligned(size_t size, size_t alignment);
void hz5_p1_free(void* ptr);
int hz5_p1_owns(void* ptr);
int hz5_p1_validate(void* ptr, size_t size, size_t alignment);

void* hz5_p2_alloc_aligned(size_t size, size_t alignment);
void hz5_p2_free(void* ptr);
size_t hz5_p2_flush_remote(void);
size_t hz5_p2_drain_current_owner(void);
void hz5_stats_print_once(void);

#ifdef __cplusplus
}
#endif

#endif
