#include "hz5_internal.h"

#include <stddef.h>
#include <stdint.h>

static _Thread_local Hz5RemoteBuffer t_hz5_remote_buffer;

static void hz5_remote_set_next(void* ptr, void* next) {
    *(void**)ptr = next;
}

Hz5RemoteBuffer* hz5_remote_tls_buffer(void) {
    return &t_hz5_remote_buffer;
}

void hz5_remote_push_group(Hz5Seg* seg,
                           uint32_t page_idx,
                           void* head,
                           void* tail,
                           uint32_t count) {
    if (!seg || page_idx >= HZ5_SEG_PAGES || !head || !tail || count == 0) {
        return;
    }

    Hz5PageMeta* meta = &seg->page[page_idx];
    void* old = atomic_load_explicit(&meta->remote_head, memory_order_acquire);
    for (;;) {
        hz5_remote_set_next(tail, old);
        if (atomic_compare_exchange_weak_explicit(&meta->remote_head,
                                                  &old,
                                                  head,
                                                  memory_order_release,
                                                  memory_order_acquire)) {
            break;
        }
    }
    atomic_fetch_add_explicit(&meta->remote_count_hint, count, memory_order_relaxed);
}

size_t hz5_remote_flush_buffer(Hz5RemoteBuffer* buffer) {
    if (!buffer || buffer->count == 0) {
        return 0;
    }

    hz5_stats_inc(HZ5_STAT_REMOTE_BUFFER_FLUSH);
    hz5_stats_add(HZ5_STAT_REMOTE_BUFFER_FLUSH_ENTRY, buffer->count);
    size_t flushed = 0;
    for (uint32_t i = 0; i < buffer->count; ++i) {
        if (!buffer->entries[i].ptr) {
            continue;
        }

        Hz5Seg* seg = buffer->entries[i].seg;
        uint32_t page = buffer->entries[i].page_idx;
        Hz5OwnerToken owner = buffer->entries[i].owner;
        void* head = buffer->entries[i].ptr;
        void* tail = head;
        uint32_t count = 1;
        buffer->entries[i].ptr = NULL;

        for (uint32_t j = i + 1; j < buffer->count; ++j) {
            Hz5RemoteEntry* other = &buffer->entries[j];
            if (!other->ptr || other->seg != seg || other->page_idx != page ||
                !hz5_owner_equal(other->owner, owner)) {
                continue;
            }
            hz5_remote_set_next(tail, other->ptr);
            tail = other->ptr;
            other->ptr = NULL;
            ++count;
        }
        hz5_remote_set_next(tail, NULL);
        hz5_remote_push_group(seg, page, head, tail, count);
#if !HZ5_P11_SPEED_CORE
        atomic_fetch_sub_explicit(&seg->remote_buffer_pending_hint,
                                  count,
                                  memory_order_relaxed);
#endif
        flushed += count;
    }

    buffer->count = 0;
    return flushed;
}

void hz5_remote_buffer_add(Hz5Seg* seg,
                           uint32_t page_idx,
                           void* ptr,
                           Hz5OwnerToken owner) {
    Hz5RemoteBuffer* buffer = hz5_remote_tls_buffer();
    if (buffer->count >= HZ5_REMOTE_BUF_CAP) {
        hz5_remote_flush_buffer(buffer);
    }
    uint32_t idx = buffer->count++;
    buffer->entries[idx].seg = seg;
    buffer->entries[idx].page_idx = page_idx;
    buffer->entries[idx].ptr = ptr;
    buffer->entries[idx].owner = owner;
#if !HZ5_P11_SPEED_CORE
    atomic_fetch_add_explicit(&seg->remote_buffer_pending_hint,
                              1u,
                              memory_order_relaxed);
#endif
}

size_t hz5_remote_drain_owner(Hz5OwnerToken owner) {
    size_t drained = 0;
    uint32_t seg_count = hz5_p1_segment_count();
    for (uint32_t seg_idx = 0; seg_idx < seg_count; ++seg_idx) {
        Hz5Seg* seg = hz5_p1_segment_at(seg_idx);
        if (!seg) {
            continue;
        }
        for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
            Hz5PageMeta* meta = &seg->page[page];
            if (meta->kind != HZ5_PAGE_RUN_START ||
                !hz5_owner_equal(meta->owner, owner)) {
                continue;
            }

            void* list = atomic_exchange_explicit(&meta->remote_head,
                                                  NULL,
                                                  memory_order_acquire);
            if (!list) {
                continue;
            }
            atomic_store_explicit(&meta->remote_count_hint, 0, memory_order_relaxed);

            while (list) {
                void* next = *(void**)list;
                hz5_stats_inc_pages(HZ5_STAT_REMOTE_DRAIN_NODE, meta->run_pages);
                if (!hz5_tcache_push(list)) {
                    hz5_stats_inc_pages(HZ5_STAT_REMOTE_DRAIN_RELEASE, meta->run_pages);
                    hz5_p1_segment_free_run(seg, page);
                } else {
                    hz5_stats_inc_pages(HZ5_STAT_REMOTE_DRAIN_CACHE, meta->run_pages);
                }
                list = next;
                ++drained;
            }
        }
    }
    return drained;
}

size_t hz5_remote_release_owner(Hz5OwnerToken owner) {
    size_t released = 0;
    uint32_t seg_count = hz5_p1_segment_count();
    for (uint32_t seg_idx = 0; seg_idx < seg_count; ++seg_idx) {
        Hz5Seg* seg = hz5_p1_segment_at(seg_idx);
        if (!seg) {
            continue;
        }
        for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
            Hz5PageMeta* meta = &seg->page[page];
            if (meta->kind != HZ5_PAGE_RUN_START ||
                !hz5_owner_equal(meta->owner, owner)) {
                continue;
            }

            void* list = atomic_exchange_explicit(&meta->remote_head,
                                                  NULL,
                                                  memory_order_acquire);
            if (!list) {
                continue;
            }
            atomic_store_explicit(&meta->remote_count_hint, 0, memory_order_relaxed);

            while (list) {
                void* next = *(void**)list;
                uint32_t pages = meta->run_pages;
                hz5_stats_inc_pages(HZ5_STAT_OWNER_DESTRUCTOR_DRAIN, pages);
                hz5_p1_segment_free_run(seg, page);
                hz5_stats_inc_pages(HZ5_STAT_OWNER_DESTRUCTOR_RELEASE, pages);
                (void)pages;
                list = next;
                ++released;
            }
        }
    }
    return released;
}

size_t hz5_remote_release_all_pending(void) {
    size_t released = 0;
    uint32_t seg_count = hz5_p1_segment_count();
    for (uint32_t seg_idx = 0; seg_idx < seg_count; ++seg_idx) {
        Hz5Seg* seg = hz5_p1_segment_at(seg_idx);
        if (!seg) {
            continue;
        }
        for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
            Hz5PageMeta* meta = &seg->page[page];
            if (meta->kind != HZ5_PAGE_RUN_START) {
                continue;
            }

            void* list = atomic_exchange_explicit(&meta->remote_head,
                                                  NULL,
                                                  memory_order_acquire);
            if (!list) {
                continue;
            }
            atomic_store_explicit(&meta->remote_count_hint, 0, memory_order_relaxed);

            while (list) {
                void* next = *(void**)list;
                uint32_t pages = meta->run_pages;
                hz5_p1_segment_free_run(seg, page);
                hz5_stats_inc_pages(HZ5_STAT_FINAL_PENDING_RELEASE, pages);
                (void)pages;
                list = next;
                ++released;
            }
        }
    }
    return released;
}
