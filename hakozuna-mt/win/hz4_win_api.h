#ifndef HZ4_WIN_API_H
#define HZ4_WIN_API_H

#include <stddef.h>

void* hz4_win_malloc(size_t size);
void  hz4_win_free(void* ptr);
void* hz4_win_realloc(void* ptr, size_t size);
size_t hz4_win_usable_size(void* ptr);

#endif
