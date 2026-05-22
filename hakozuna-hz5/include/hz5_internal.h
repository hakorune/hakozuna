#ifndef HZ5_INTERNAL_H
#define HZ5_INTERNAL_H

#include "hz5_config.h"

#include <stdatomic.h>
#include <stdint.h>

#define HZ5_BITMAP_INDEX(page) ((page) >> 6u)
#define HZ5_BITMAP_MASK(page) (1ULL << ((page) & 63u))
#define HZ5_ALIGN_8K_LOG2 13u
#define HZ5_RUN_16P_PAGES 16u

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
#if HZ5_P11_SPEED_CORE
    uint32_t reserved0;
#else
    uint32_t run16_a8192_hint_page;
    _Atomic(uint32_t) tcache_refs;
    _Atomic(uint32_t) remote_buffer_pending_hint;
#endif
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

typedef enum Hz5RunBand {
    HZ5_RUNBAND_UNKNOWN = 0,
    HZ5_RUNBAND_SMALL_OVA = 1,
    HZ5_RUNBAND_LARGE16 = 2,
    HZ5_RUNBAND_HIGH = 3
} Hz5RunBand;

typedef struct Hz5RunClassPolicy {
    uint16_t pages;
    uint8_t align_log2;
    uint8_t band;
    uint16_t owner_cache_cap;
    uint16_t central_cache_cap;
    uint16_t remote_flush_cap;
    uint32_t central_byte_cap;
    uint32_t flags;
} Hz5RunClassPolicy;

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
    HZ5_STAT_P14_EMPTY_TRANSITION,
    HZ5_STAT_P14_EMPTY_TRANSITION_BYTES,
    HZ5_STAT_P14_RETIRE_SCAN_CALL,
    HZ5_STAT_P14_RETIRE_CANDIDATE_SEGMENT,
    HZ5_STAT_P14_RETIRE_CANDIDATE_BYTES,
    HZ5_STAT_P14_RETIRE_OK,
    HZ5_STAT_P14_RETIRE_OK_BYTES,
    HZ5_STAT_P14_RETIRE_REJECT_LIVE,
    HZ5_STAT_P14_RETIRE_REJECT_REMOTE,
    HZ5_STAT_P14_RETIRE_REJECT_STATE,
    HZ5_STAT_P14_LOCKED_LOOKUP_CALL,
    HZ5_STAT_P14_LOCKED_LOOKUP_HIT,
    HZ5_STAT_P14_LOCKED_LOOKUP_MISS_RANGE,
    HZ5_STAT_P14_LOCKED_LOOKUP_MISS_TABLE,
    HZ5_STAT_P14_LOCKED_LOOKUP_BAD_MAGIC,
    HZ5_STAT_COUNT
} Hz5StatId;

_Static_assert(sizeof(Hz5Seg) <= HZ5_SEG_HEADER_PAGES * HZ5_PAGE_SIZE,
               "HZ5 segment metadata must fit in the committed header region");

/* Internal P1/P2 entrypoints used by core, policy, and smoke harnesses. */
void* hz5_p1_alloc_aligned(size_t size, size_t alignment);
void hz5_p1_free(void* ptr);
int hz5_p1_owns(void* ptr);
int hz5_p1_validate(void* ptr, size_t size, size_t alignment);

void* hz5_p2_alloc_aligned(size_t size, size_t alignment);
void hz5_p2_free(void* ptr);
size_t hz5_p2_flush_remote(void);
size_t hz5_p2_drain_current_owner(void);

Hz5Seg* hz5_p1_segment_get(void);
uint32_t hz5_p1_segment_count(void);
Hz5Seg* hz5_p1_segment_at(uint32_t index);
uint32_t hz5_p1_segment_retire_empty_quarantine(void);
uint32_t hz5_p1_segment_release_empty_for_shutdown(void);
void* hz5_p1_segment_alloc_run_aligned(Hz5Seg* seg,
                                       uint32_t pages,
                                       uint32_t align_pages,
                                       uint8_t align_log2,
                                       uint16_t sc,
                                       Hz5OwnerToken owner);
void* hz5_p1_segment_alloc_any_run_aligned(uint32_t pages,
                                           uint32_t align_pages,
                                           uint8_t align_log2,
                                           uint16_t sc,
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

uint16_t hz5_run_sc_for_size(size_t size, uint32_t pages);
uint32_t hz5_run_class_index(uint32_t pages, uint8_t align_log2, uint16_t sc);
const Hz5RunClassPolicy* hz5_run_policy_for(uint32_t pages,
                                            uint8_t align_log2,
                                            uint16_t sc);
void* hz5_tcache_pop(uint32_t pages, uint8_t align_log2, uint16_t sc);
int hz5_tcache_push(void* ptr);
size_t hz5_tcache_release_all(void);

void hz5_stats_print_once(void);

#if HZ5_DIAGNOSTIC_STATS || defined(HZ5_STATS_IMPLEMENTATION)
void hz5_stats_inc(Hz5StatId id);
void hz5_stats_add(Hz5StatId id, uint64_t value);
void hz5_stats_inc_pages(Hz5StatId id, uint32_t pages);
void hz5_stats_add_pages(Hz5StatId id, uint32_t pages, uint64_t value);
void hz5_stats_inc_run(Hz5StatId id, uint32_t pages, uint16_t sc);
void hz5_stats_add_run(Hz5StatId id,
                       uint32_t pages,
                       uint16_t sc,
                       uint64_t value);
#else
#define hz5_stats_inc(id) ((void)0)
#define hz5_stats_add(id, value) ((void)0)
#define hz5_stats_inc_pages(id, pages) ((void)0)
#define hz5_stats_add_pages(id, pages, value) ((void)0)
#define hz5_stats_inc_run(id, pages, sc) ((void)0)
#define hz5_stats_add_run(id, pages, sc, value) ((void)0)
#endif

#endif
