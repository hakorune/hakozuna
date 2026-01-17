#include "hz3_hook_link.h"

#include <new>

void* operator new(size_t size) {
    void* p = hz3_hook_malloc(size);
    if (!p) {
        throw std::bad_alloc();
    }
    return p;
}

void operator delete(void* ptr) noexcept {
    hz3_hook_free(ptr);
}

void* operator new[](size_t size) {
    void* p = hz3_hook_malloc(size);
    if (!p) {
        throw std::bad_alloc();
    }
    return p;
}

void operator delete[](void* ptr) noexcept {
    hz3_hook_free(ptr);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
    return hz3_hook_malloc(size);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    hz3_hook_free(ptr);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    return hz3_hook_malloc(size);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    hz3_hook_free(ptr);
}
