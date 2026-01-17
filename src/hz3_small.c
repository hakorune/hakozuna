#include "hz3_small.h"

#include "hz3_tcache.h"

#include <stdint.h>

// Small v1 is archived; these are stubs only.
void* hz3_small_alloc(size_t size) {
    (void)size;
    return NULL;
}

void hz3_small_free(void* ptr, int sc) {
    (void)ptr;
    (void)sc;
}

void hz3_small_bin_flush(int sc, Hz3Bin* bin) {
    (void)sc;
    (void)bin;
}

#if HZ3_PTAG16_DISPATCH_ENABLE
void hz3_small_free_by_tag(void* ptr, int sc, int owner) {
    (void)ptr;
    (void)sc;
    (void)owner;
}
#endif
