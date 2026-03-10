#include <errno.h>
#include <stddef.h>

#include <mimalloc.h>

static size_t hz_memcached_page_size(void) {
    return 4096u;
}

void* malloc(size_t size) {
    return mi_malloc(size);
}

void free(void* ptr) {
    mi_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    return mi_calloc(nmemb, size);
}

void* realloc(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
}

size_t malloc_usable_size(void* ptr) {
    return mi_usable_size(ptr);
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    return mi_posix_memalign(memptr, alignment, size);
}

void* memalign(size_t alignment, size_t size) {
    return mi_malloc_aligned(size, alignment);
}

void* aligned_alloc(size_t alignment, size_t size) {
    if (alignment == 0 || (alignment & (alignment - 1u)) != 0) {
        errno = EINVAL;
        return NULL;
    }
    if ((size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }
    return mi_malloc_aligned(size, alignment);
}

void* valloc(size_t size) {
    return mi_malloc_aligned(size, hz_memcached_page_size());
}

void* pvalloc(size_t size) {
    const size_t page_size = hz_memcached_page_size();
    const size_t rounded = (size + page_size - 1u) & ~(page_size - 1u);
    return mi_malloc_aligned(rounded, page_size);
}
