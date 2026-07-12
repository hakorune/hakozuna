#ifndef H8_MEDIUM_PAGE_BACKEND_H
#define H8_MEDIUM_PAGE_BACKEND_H
#include "../include/h8.h"

#include <stdbool.h>
#include <stddef.h>

void* h8_medium_page_backend_malloc(size_t size);
bool h8_medium_page_backend_accepts_size(size_t size);
bool h8_medium_page_backend_free(void* ptr, bool* owned_out);
bool h8_medium_page_backend_free_record(const void* record, void* ptr,
                                        bool* owned_out);
H8RouteKind h8_medium_page_backend_route(const void* ptr);
bool h8_medium_page_backend_usable_size(const void* ptr,
                                        size_t* usable_out,
                                        bool* owned_out);
#endif
