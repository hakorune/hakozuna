#ifndef H8_MEDIUM_PAGE_BACKEND_H
#define H8_MEDIUM_PAGE_BACKEND_H
#include <stdbool.h>
#include <stddef.h>
void* h8_medium_page_backend_malloc(size_t size);
bool h8_medium_page_backend_free(void* ptr, bool* owned_out);
#endif
