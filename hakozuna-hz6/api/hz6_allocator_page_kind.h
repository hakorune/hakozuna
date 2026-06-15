#ifndef HZ6_ALLOCATOR_PAGE_KIND_H
#define HZ6_ALLOCATOR_PAGE_KIND_H

#include "hz6_allocator_types.h"

#include <stdint.h>

#if HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1

#define HZ6_PAGE_KIND_UNKNOWN ((uint16_t)0u)
#define HZ6_PAGE_KIND_TOY ((uint16_t)1u)
#define HZ6_PAGE_KIND_MIDPAGE ((uint16_t)2u)
#define HZ6_PAGE_KIND_MIXED ((uint16_t)3u)

static inline uintptr_t hz6_page_kind_page_for_ptr(const void* ptr) {
  return ((uintptr_t)ptr) & ~((uintptr_t)HZ6_ROUTE_PAGE_GRANULARITY - 1u);
}

static inline size_t hz6_page_kind_index(uintptr_t page) {
  uintptr_t key = page >> 12;
  key ^= key >> 17;
  key *= (uintptr_t)0xff51afd7ed558ccdULL;
  key ^= key >> 33;
  return (size_t)(key % HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY);
}

static inline size_t hz6_page_kind_wrap(size_t index) {
  return index % HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY;
}

static inline void hz6_allocator_page_kind_note(Hz6Allocator* allocator,
                                                const void* ptr,
                                                uint16_t kind) {
  if (!allocator || !ptr || HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY == 0 ||
      kind == HZ6_PAGE_KIND_UNKNOWN) {
    return;
  }
  uintptr_t page = hz6_page_kind_page_for_ptr(ptr);
  size_t base = hz6_page_kind_index(page);
  Hz6PageKindEntry* first_empty = NULL;
  for (size_t probe = 0; probe < HZ6_PAGE_KIND_FREE_SELECTOR_PROBE_LIMIT;
       ++probe) {
    Hz6PageKindEntry* entry =
        &allocator->page_kind_selector[hz6_page_kind_wrap(base + probe)];
    if (entry->page == page) {
      if (entry->kind == HZ6_PAGE_KIND_UNKNOWN) {
        entry->kind = kind;
      } else if (entry->kind != kind) {
        entry->kind = HZ6_PAGE_KIND_MIXED;
      }
      return;
    }
    if (entry->page == 0 && !first_empty) {
      first_empty = entry;
    }
  }
  Hz6PageKindEntry* target =
      first_empty ? first_empty : &allocator->page_kind_selector[base];
  target->page = page;
  target->kind = kind;
}

static inline uint16_t hz6_allocator_page_kind_lookup(
    const Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr || HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY == 0) {
    return HZ6_PAGE_KIND_UNKNOWN;
  }
  uintptr_t page = hz6_page_kind_page_for_ptr(ptr);
  size_t base = hz6_page_kind_index(page);
  for (size_t probe = 0; probe < HZ6_PAGE_KIND_FREE_SELECTOR_PROBE_LIMIT;
       ++probe) {
    const Hz6PageKindEntry* entry =
        &allocator->page_kind_selector[hz6_page_kind_wrap(base + probe)];
    if (entry->page == page) {
      return entry->kind;
    }
    if (entry->page == 0) {
      return HZ6_PAGE_KIND_UNKNOWN;
    }
  }
  return HZ6_PAGE_KIND_UNKNOWN;
}

#else

#define HZ6_PAGE_KIND_UNKNOWN ((uint16_t)0u)
#define HZ6_PAGE_KIND_TOY ((uint16_t)1u)
#define HZ6_PAGE_KIND_MIDPAGE ((uint16_t)2u)
#define HZ6_PAGE_KIND_MIXED ((uint16_t)3u)

#define hz6_allocator_page_kind_note(allocator, ptr, kind) ((void)0)
#define hz6_allocator_page_kind_lookup(allocator, ptr) \
  (HZ6_PAGE_KIND_UNKNOWN)

#endif

#endif
