// hz4_mid.c - HZ4 Mid Box (2KB..~64KB, 1 page)
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "hz4_config.h"
#include "hz4_types.h"
#include "hz4_sizeclass.h"
#include "hz4_os.h"
#include "hz4_mid.h"
#if HZ4_PAGE_TAG_TABLE
#include "hz4_pagetag.h"
#endif

typedef struct hz4_mid_page {
    uint32_t magic;
    uint16_t sc;
    uint16_t _pad0;
    uint32_t obj_size;
    uint32_t capacity;
    uint32_t free_count;
    void* free;
    struct hz4_mid_page* next;
} hz4_mid_page_t;

static pthread_mutex_t g_hz4_mid_lock = PTHREAD_MUTEX_INITIALIZER;
static hz4_mid_page_t* g_hz4_mid_bins[(HZ4_PAGE_SIZE / HZ4_MID_ALIGN)];

typedef struct hz4_mid_seg {
    struct hz4_mid_seg* next;
    uint32_t next_page;
} hz4_mid_seg_t;

static hz4_mid_seg_t* g_hz4_mid_seg_head = NULL;
static hz4_mid_seg_t* g_hz4_mid_seg_cur = NULL;

static inline uint16_t hz4_mid_size_to_sc(size_t size) {
    size = hz4_align_up(size, HZ4_MID_ALIGN);
    if (size == 0 || size > HZ4_PAGE_SIZE) {
        return 0xFFFFu;
    }
    return (uint16_t)((size / HZ4_MID_ALIGN) - 1);
}

static inline size_t hz4_mid_sc_to_size(uint16_t sc) {
    return (size_t)(sc + 1) * HZ4_MID_ALIGN;
}

size_t hz4_mid_max_size(void) {
    size_t hdr = hz4_align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN);
    if (hdr >= HZ4_PAGE_SIZE) {
        return 0;
    }
    size_t raw = (size_t)HZ4_PAGE_SIZE - hdr;
    return (raw / HZ4_MID_ALIGN) * HZ4_MID_ALIGN;
}

size_t hz4_mid_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1));
    if (page->magic != HZ4_MID_MAGIC) {
        HZ4_FAIL("hz4_mid_usable_size: invalid page");
        abort();
    }
    return (size_t)page->obj_size;
}

static hz4_mid_page_t* hz4_mid_page_create(uint16_t sc, size_t obj_size) {
    if (!g_hz4_mid_seg_cur || g_hz4_mid_seg_cur->next_page >= HZ4_PAGES_PER_SEG) {
        hz4_mid_seg_t* seg = (hz4_mid_seg_t*)hz4_os_seg_acquire();
        if (!seg) {
            abort();
        }
        seg->next_page = HZ4_PAGE_IDX_MIN;
        seg->next = g_hz4_mid_seg_head;
        g_hz4_mid_seg_head = seg;
        g_hz4_mid_seg_cur = seg;
    }

    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)g_hz4_mid_seg_cur +
                                             ((uintptr_t)g_hz4_mid_seg_cur->next_page << HZ4_PAGE_SHIFT));
    g_hz4_mid_seg_cur->next_page++;

    page->magic = HZ4_MID_MAGIC;
    page->sc = sc;
    page->obj_size = (uint32_t)obj_size;
    page->capacity = 0;
    page->free_count = 0;
    page->free = NULL;
    page->next = NULL;

    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN);
    uintptr_t end = (uintptr_t)page + HZ4_PAGE_SIZE;

    void* head = NULL;
    uint32_t count = 0;
    for (uintptr_t p = start; p + obj_size <= end; p += obj_size) {
        void* obj = (void*)p;
        hz4_obj_set_next(obj, head);
        head = obj;
        count++;
    }

    page->free = head;
    page->capacity = count;
    page->free_count = count;

#if HZ4_PAGE_TAG_TABLE
    // Register page tag for fast lookup (owner_tid=0 for mid)
    uint32_t page_idx;
    if (hz4_pagetag_page_idx(page, &page_idx)) {
        uint32_t tag = hz4_pagetag_encode(HZ4_TAG_KIND_MID, (uint8_t)sc, 0);
        hz4_pagetag_store(page_idx, tag);
    }
#endif

    return page;
}

void* hz4_mid_malloc(size_t size) {
    size_t max = hz4_mid_max_size();
    size_t aligned = hz4_align_up(size, HZ4_MID_ALIGN);
    if (aligned > max) {
        HZ4_FAIL("hz4_mid_malloc: size too large");
        abort();
    }

    uint16_t sc = hz4_mid_size_to_sc(aligned);
    if (sc == 0xFFFFu) {
        HZ4_FAIL("hz4_mid_malloc: invalid size class");
        abort();
    }

    void* obj = NULL;

    pthread_mutex_lock(&g_hz4_mid_lock);

    hz4_mid_page_t* prev = NULL;
    hz4_mid_page_t* page = g_hz4_mid_bins[sc];
    while (page && page->free == NULL) {
        prev = page;
        page = page->next;
    }

    if (!page) {
        page = hz4_mid_page_create(sc, hz4_mid_sc_to_size(sc));
        page->next = g_hz4_mid_bins[sc];
        g_hz4_mid_bins[sc] = page;
        prev = NULL;
    }

    obj = page->free;
    if (obj) {
        page->free = hz4_obj_get_next(obj);
        hz4_obj_set_next(obj, NULL);
        if (page->free_count > 0) {
            page->free_count--;
        }
    }

    if (page->free_count == 0) {
        if (prev) {
            prev->next = page->next;
        } else if (g_hz4_mid_bins[sc] == page) {
            g_hz4_mid_bins[sc] = page->next;
        }
        page->next = NULL;
    }

    pthread_mutex_unlock(&g_hz4_mid_lock);

    if (!obj) {
        HZ4_FAIL("hz4_mid_malloc: empty page");
        abort();
    }

    return obj;
}

void hz4_mid_free(void* ptr) {
    if (!ptr) {
        return;
    }

    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1));
    if (page->magic != HZ4_MID_MAGIC) {
        HZ4_FAIL("hz4_mid_free: invalid page");
        abort();
    }

    uint16_t sc = page->sc;
    if (sc >= (HZ4_PAGE_SIZE / HZ4_MID_ALIGN)) {
        HZ4_FAIL("hz4_mid_free: invalid size class");
        abort();
    }

    pthread_mutex_lock(&g_hz4_mid_lock);

    if (page->free_count == 0) {
        page->next = g_hz4_mid_bins[sc];
        g_hz4_mid_bins[sc] = page;
    }

    hz4_obj_set_next(ptr, page->free);
    page->free = ptr;
    page->free_count++;

    pthread_mutex_unlock(&g_hz4_mid_lock);
}
