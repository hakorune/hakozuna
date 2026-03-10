#include <errno.h>
#include <stddef.h>

#include <gperftools/tcmalloc.h>

static size_t hz_memcached_page_size(void) {
    return 4096u;
}

void* malloc(size_t size) {
    return tc_malloc(size);
}

void free(void* ptr) {
    tc_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    return tc_calloc(nmemb, size);
}

void* realloc(void* ptr, size_t size) {
    return tc_realloc(ptr, size);
}

size_t malloc_usable_size(void* ptr) {
    return tc_malloc_size(ptr);
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    return tc_posix_memalign(memptr, alignment, size);
}

void* memalign(size_t alignment, size_t size) {
    return tc_memalign(alignment, size);
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
    return tc_memalign(alignment, size);
}

void* valloc(size_t size) {
    return tc_memalign(hz_memcached_page_size(), size);
}

void* pvalloc(size_t size) {
    const size_t page_size = hz_memcached_page_size();
    const size_t rounded = (size + page_size - 1u) & ~(page_size - 1u);
    return tc_memalign(page_size, rounded);
}
