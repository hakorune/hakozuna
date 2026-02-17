// hz4_mid.h - HZ4 Mid Box API
#ifndef HZ4_MID_H
#define HZ4_MID_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdint.h>

#include "hz4_config.h"
#include "hz4_sizeclass.h"

typedef struct hz4_mid_page {
    uint32_t magic;
    uint16_t sc;
    uint16_t _pad0;
    uint32_t obj_size;
    uint32_t capacity;
    uint32_t free_count;
    void* free;
    struct hz4_mid_page* next;
    uint8_t in_bin;
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    // MidOwnerRemoteQueueBox:
    // owner_tag==0 => unowned (global bin page)
    // owner_tag!=0 => owner-private page; cross-thread frees go to remote_head.
    _Atomic(uintptr_t) owner_tag;
    _Atomic(void*) remote_head;
    _Atomic(uint32_t) remote_count;
#endif
} hz4_mid_page_t;

static inline size_t hz4_mid_max_size_inline(void) {
    size_t hdr = hz4_align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN);
    if (hdr >= HZ4_PAGE_SIZE) {
        return 0;
    }
    size_t raw = (size_t)HZ4_PAGE_SIZE - hdr;
    return (raw / HZ4_MID_ALIGN) * HZ4_MID_ALIGN;
}

void* hz4_mid_malloc(size_t size);
void  hz4_mid_free(void* ptr);
size_t hz4_mid_max_size(void);
size_t hz4_mid_usable_size(void* ptr);

#if HZ4_MID_MAX_SIZE_INLINE
#define HZ4_MID_MAX_SIZE() hz4_mid_max_size_inline()
#else
#define HZ4_MID_MAX_SIZE() hz4_mid_max_size()
#endif

#endif // HZ4_MID_H
