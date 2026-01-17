#pragma once

// hz3 List Utilities (inline helpers for intrusive linked lists)
//
// All functions are inline for zero-cost abstraction.
// Assumes: next pointer stored at obj[0] (intrusive list convention).

#include "hz3_tcache.h"  // for hz3_obj_get_next/set_next

// Find tail and count of linked list.
// Precondition: head is non-NULL.
// Postcondition: *out_tail points to last element, *out_count = list length.
static inline void hz3_list_find_tail(void* head, void** out_tail, uint32_t* out_count) {
    *out_tail = head;
    *out_count = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        *out_tail = cur;
        (*out_count)++;
        cur = hz3_obj_get_next(cur);
    }
}

// Get length of linked list (may be 0 if head is NULL).
static inline uint32_t hz3_list_length(void* head) {
    uint32_t count = 0;
    void* cur = head;
    while (cur) {
        count++;
        cur = hz3_obj_get_next(cur);
    }
    return count;
}

// Find nth node (0-indexed). Returns NULL if n exceeds length or head is NULL.
static inline void* hz3_list_nth(void* head, uint32_t n) {
    void* cur = head;
    for (uint32_t i = 0; i < n && cur; i++) {
        cur = hz3_obj_get_next(cur);
    }
    return cur;
}
