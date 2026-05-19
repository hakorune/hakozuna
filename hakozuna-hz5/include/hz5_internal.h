#ifndef HZ5_INTERNAL_H
#define HZ5_INTERNAL_H

#include "hz5_config.h"

#include <stdatomic.h>
#include <stdint.h>

typedef enum Hz5PageKind {
    HZ5_PAGE_FREE = 0,
    HZ5_PAGE_RUN_START = 1,
    HZ5_PAGE_RUN_CONT = 2
} Hz5PageKind;

typedef enum Hz5DecommitState {
    HZ5_COMMITTED = 0,
    HZ5_RESET = 1,
    HZ5_DECOMMITTED = 2
} Hz5DecommitState;

typedef struct Hz5OwnerToken {
    uint16_t slot;
    uint16_t generation;
} Hz5OwnerToken;

typedef struct Hz5PageMeta {
    _Atomic(void*) remote_head;
    _Atomic(uint32_t) remote_count_hint;
    uint16_t kind;
    uint16_t sc;
    uint16_t run_pages;
    uint16_t run_start_page;
    uint8_t align_log2;
    uint8_t decommit_state;
    uint16_t flags;
    Hz5OwnerToken owner;
    uint32_t epoch;
} Hz5PageMeta;

typedef struct Hz5Seg {
    uint64_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t live_pages;
    uint32_t reserved0;
    uint64_t free_bitmap[HZ5_SEG_PAGES / 64u];
    Hz5PageMeta page[HZ5_SEG_PAGES];
} Hz5Seg;

typedef struct Hz5RemoteEntry {
    Hz5Seg* seg;
    uint32_t page_idx;
    void* ptr;
    Hz5OwnerToken owner;
} Hz5RemoteEntry;

typedef struct Hz5RemoteBuffer {
    Hz5RemoteEntry entries[HZ5_REMOTE_BUF_CAP];
    uint32_t count;
} Hz5RemoteBuffer;

typedef struct Hz5RunCacheClass {
    void* slots[HZ5_RUN_CACHE_CAP];
    uint16_t count;
} Hz5RunCacheClass;

typedef struct Hz5RunCache {
    Hz5RunCacheClass cls[HZ5_RUN_CACHE_CLASS_COUNT];
} Hz5RunCache;

typedef enum Hz5StatId {
    HZ5_STAT_ALLOC_CALL = 0,
    HZ5_STAT_ALLOC_TCACHE_HIT,
    HZ5_STAT_ALLOC_DRAIN_CALL,
    HZ5_STAT_ALLOC_DRAIN_HIT,
    HZ5_STAT_ALLOC_SEGMENT,
    HZ5_STAT_FREE_LOCAL_CACHE,
    HZ5_STAT_FREE_LOCAL_RELEASE,
    HZ5_STAT_FREE_REMOTE_BUFFERED,
    HZ5_STAT_REMOTE_BUFFER_FLUSH,
    HZ5_STAT_REMOTE_BUFFER_FLUSH_ENTRY,
    HZ5_STAT_REMOTE_DRAIN_NODE,
    HZ5_STAT_REMOTE_DRAIN_CACHE,
    HZ5_STAT_REMOTE_DRAIN_RELEASE,
    HZ5_STAT_SEGMENT_ALLOC_SCAN_STEP,
    HZ5_STAT_TLS_DESTRUCTOR_FLUSH,
    HZ5_STAT_TLS_DESTRUCTOR_FLUSH_ENTRY,
    HZ5_STAT_OWNER_DESTRUCTOR_DRAIN,
    HZ5_STAT_OWNER_DESTRUCTOR_RELEASE,
    HZ5_STAT_TCACHE_DESTRUCTOR_RELEASE,
    HZ5_STAT_FINAL_PENDING_RELEASE,
    HZ5_STAT_COUNT
} Hz5StatId;

_Static_assert(sizeof(Hz5Seg) <= HZ5_SEG_HEADER_PAGES * HZ5_PAGE_SIZE,
               "HZ5 segment metadata must fit in the committed header region");

Hz5Seg* hz5_p1_segment_get(void);
uint32_t hz5_p1_segment_count(void);
Hz5Seg* hz5_p1_segment_at(uint32_t index);
void* hz5_p1_segment_alloc_run_aligned(Hz5Seg* seg,
                                       uint32_t pages,
                                       uint32_t align_pages,
                                       uint8_t align_log2,
                                       Hz5OwnerToken owner);
void* hz5_p1_segment_alloc_any_run_aligned(uint32_t pages,
                                           uint32_t align_pages,
                                           uint8_t align_log2,
                                           Hz5OwnerToken owner);
void hz5_p1_segment_free_run(Hz5Seg* seg, uint32_t page_idx);
Hz5Seg* hz5_p1_seg_from_ptr(void* ptr);
uint32_t hz5_p1_page_index(Hz5Seg* seg, void* ptr);

Hz5OwnerToken hz5_owner_current(void);
Hz5OwnerToken hz5_owner_current_if_initialized(void);
int hz5_owner_equal(Hz5OwnerToken a, Hz5OwnerToken b);
int hz5_owner_is_alive(Hz5OwnerToken owner);
Hz5RemoteBuffer* hz5_remote_tls_buffer(void);
void hz5_remote_buffer_add(Hz5Seg* seg,
                           uint32_t page_idx,
                           void* ptr,
                           Hz5OwnerToken owner);
size_t hz5_remote_flush_buffer(Hz5RemoteBuffer* buffer);
size_t hz5_remote_drain_owner(Hz5OwnerToken owner);
size_t hz5_remote_release_owner(Hz5OwnerToken owner);
size_t hz5_remote_release_all_pending(void);
void hz5_remote_push_group(Hz5Seg* seg,
                           uint32_t page_idx,
                           void* head,
                           void* tail,
                           uint32_t count);

uint32_t hz5_run_class_index(uint32_t pages, uint8_t align_log2);
void* hz5_tcache_pop(uint32_t pages, uint8_t align_log2);
int hz5_tcache_push(void* ptr);
size_t hz5_tcache_release_all(void);

void hz5_stats_print_once(void);

#if HZ5_DIAGNOSTIC_STATS || defined(HZ5_STATS_IMPLEMENTATION)
void hz5_stats_inc(Hz5StatId id);
void hz5_stats_add(Hz5StatId id, uint64_t value);
void hz5_stats_inc_pages(Hz5StatId id, uint32_t pages);
void hz5_stats_add_pages(Hz5StatId id, uint32_t pages, uint64_t value);
#else
#define hz5_stats_inc(id) ((void)0)
#define hz5_stats_add(id, value) ((void)0)
#define hz5_stats_inc_pages(id, pages) ((void)0)
#define hz5_stats_add_pages(id, pages, value) ((void)0)
#endif

#endif
