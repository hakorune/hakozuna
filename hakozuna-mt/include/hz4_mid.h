// hz4_mid.h - HZ4 Mid Box API
#ifndef HZ4_MID_H
#define HZ4_MID_H

#include <stddef.h>

void* hz4_mid_malloc(size_t size);
void  hz4_mid_free(void* ptr);
size_t hz4_mid_max_size(void);
size_t hz4_mid_usable_size(void* ptr);

#endif // HZ4_MID_H
