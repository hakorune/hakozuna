#include "hz7.h"

#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifndef H7_SPAN_CLASS_MAX
#define H7_SPAN_CLASS_MAX 16384u
#endif
#define H7_DIRECT_MIN (H7_SPAN_CLASS_MAX + 1u)
#define H7_SPAN_BYTES (64u * 1024u)
#ifndef H7_EMPTY_SPAN_CAP
#define H7_EMPTY_SPAN_CAP 4u
#endif
#define H7_DIRECT_RETAIN_32K (32u * 1024u)
#define H7_DIRECT_RETAIN_64K (64u * 1024u)
#define H7_DIRECT_RETAIN_BUCKET_COUNT 2u
#ifndef H7_DIRECT_RETAIN_CAP
#define H7_DIRECT_RETAIN_CAP 32u
#endif
#define H7_FREE_NONE UINT32_MAX
#define H7_ROUTE_SLOT_NONE UINT32_MAX
#define H7_MAGIC 0x48375A37u
#define H7_COOKIE 0x7A17C0DEu
#define H7_REGION_ACTIVE 0x1u
#define H7_REGION_RETAINED 0x2u
#ifndef H7_ROUTE_CAPACITY
#define H7_ROUTE_CAPACITY 4096u
#endif

_Static_assert((H7_SPAN_BYTES & (H7_SPAN_BYTES - 1u)) == 0,
               "H7_SPAN_BYTES must be a power of two");
_Static_assert((H7_ROUTE_CAPACITY & (H7_ROUTE_CAPACITY - 1u)) == 0,
               "H7_ROUTE_CAPACITY must be a power of two");

typedef enum H7RegionKind {
  H7_REGION_SMALL_SPAN = 1,
  H7_REGION_DIRECT = 2
} H7RegionKind;

typedef struct H7RegionHeader {
  uint32_t magic;
  uint32_t cookie;
  uint16_t kind;
  uint16_t flags;
  uint32_t reserved;
  size_t region_size;
} H7RegionHeader;

typedef struct H7Span {
  H7RegionHeader region;
  uint16_t class_id;
  uint16_t span_flags;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t used_count;
  uint32_t free_head;
  uint32_t bitmap_words;
  uint32_t slot_offset;
  struct H7Span* next;
  struct H7Span* prev;
} H7Span;

typedef struct H7Direct {
  H7RegionHeader region;
  size_t requested_size;
} H7Direct;

typedef struct H7Class {
  uint32_t slot_size;
  H7Span* partial;
  H7Span* empty;
  uint32_t empty_count;
} H7Class;

typedef struct H7DirectRetainBucket {
  size_t max_size;
  H7Direct* items[H7_DIRECT_RETAIN_CAP];
  uint32_t count;
} H7DirectRetainBucket;

typedef struct H7RouteEntry {
  uintptr_t base;
  size_t size;
  H7RegionKind kind;
  uint16_t active;
} H7RouteEntry;

typedef struct H7RouteResult {
  H7RouteKind kind;
  H7RegionHeader* region;
} H7RouteResult;

typedef struct H7PendingRelease {
  void* ptr;
  size_t size;
} H7PendingRelease;

static H7Class g_h7_classes[] = {
    {16u, 0, 0, 0},   {32u, 0, 0, 0},   {64u, 0, 0, 0},
    {128u, 0, 0, 0},  {256u, 0, 0, 0},  {512u, 0, 0, 0},
    {1024u, 0, 0, 0}, {2048u, 0, 0, 0}, {4096u, 0, 0, 0},
    {8192u, 0, 0, 0}, {16384u, 0, 0, 0},
#if H7_SPAN_CLASS_MAX >= 32768u
    {32768u, 0, 0, 0},
#endif
};

static H7Stats g_h7_stats;
static H7RouteEntry g_h7_routes[H7_ROUTE_CAPACITY];
static H7DirectRetainBucket g_h7_direct_retain[H7_DIRECT_RETAIN_BUCKET_COUNT] = {
    {H7_DIRECT_RETAIN_32K, {0}, 0},
    {H7_DIRECT_RETAIN_64K, {0}, 0},
};

#ifdef _WIN32
static volatile LONG g_h7_lock;
#else
static volatile int g_h7_lock;
#endif

static void h7_lock(void) {
#ifdef _WIN32
  while (InterlockedCompareExchange(&g_h7_lock, 1, 0) != 0) {
    Sleep(0);
  }
#else
  while (__sync_lock_test_and_set(&g_h7_lock, 1) != 0) {
    sched_yield();
  }
#endif
}

static void h7_unlock(void) {
#ifdef _WIN32
  InterlockedExchange(&g_h7_lock, 0);
#else
  __sync_lock_release(&g_h7_lock);
#endif
}

static size_t h7_align_up(size_t x, size_t a) {
  return (x + a - 1u) & ~(a - 1u);
}

static size_t h7_region_align_up(size_t x) {
  return h7_align_up(x, H7_SPAN_BYTES);
}

static uintptr_t h7_region_base_from_ptr(const void* ptr) {
  return (uintptr_t)ptr & ~((uintptr_t)H7_SPAN_BYTES - 1u);
}

static int h7_size_is_small(size_t size) {
  return size <= H7_SPAN_CLASS_MAX;
}

static void h7_region_header_init(H7RegionHeader* region,
                                  H7RegionKind kind,
                                  uint16_t flags,
                                  size_t region_size) {
  region->magic = H7_MAGIC;
  region->cookie = H7_COOKIE;
  region->kind = kind;
  region->flags = flags;
  region->reserved = H7_ROUTE_SLOT_NONE;
  region->region_size = region_size;
}

static size_t h7_route_hash(uintptr_t base) {
  return (size_t)((base >> 16u) & (H7_ROUTE_CAPACITY - 1u));
}

static H7DirectRetainBucket* h7_direct_retain_bucket_for_size(size_t size) {
  size_t i;
  if (h7_size_is_small(size)) {
    return 0;
  }
  for (i = 0; i < H7_DIRECT_RETAIN_BUCKET_COUNT; ++i) {
    if (size <= g_h7_direct_retain[i].max_size) {
      return &g_h7_direct_retain[i];
    }
  }
  return 0;
}

static H7Direct* h7_direct_retain_pop(size_t size) {
  H7DirectRetainBucket* retain = h7_direct_retain_bucket_for_size(size);
  H7Direct* direct;
  if (!retain || retain->count == 0) {
    return 0;
  }
  --retain->count;
  direct = retain->items[retain->count];
  retain->items[retain->count] = 0;
  return direct;
}

static int h7_direct_retain_push(H7Direct* direct) {
  H7DirectRetainBucket* retain;
  if (!direct) {
    return 0;
  }
  retain = h7_direct_retain_bucket_for_size(direct->requested_size);
  if (!retain || retain->count >= H7_DIRECT_RETAIN_CAP) {
    return 0;
  }
  retain->items[retain->count] = direct;
  ++retain->count;
  return 1;
}

static int h7_route_register(void* base, size_t size, H7RegionKind kind) {
  size_t i;
  uintptr_t key = (uintptr_t)base;
  H7RegionHeader* region = (H7RegionHeader*)base;
  size_t start = h7_route_hash(key);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (!g_h7_routes[slot].active) {
      g_h7_routes[slot].base = key;
      g_h7_routes[slot].size = size;
      g_h7_routes[slot].kind = kind;
      g_h7_routes[slot].active = 1u;
      if (region) {
        region->reserved = (uint32_t)slot;
      }
      ++g_h7_stats.route_count;
      return 1;
    }
  }
  ++g_h7_stats.route_register_fail;
  return 0;
}

static void h7_route_clear_slot(size_t slot, H7RegionHeader* region) {
  g_h7_routes[slot].active = 0u;
  g_h7_routes[slot].base = 0u;
  g_h7_routes[slot].size = 0u;
  g_h7_routes[slot].kind = 0;
  if (region) {
    region->reserved = H7_ROUTE_SLOT_NONE;
  }
  --g_h7_stats.route_count;
}

static void h7_route_unregister(void* base) {
  size_t i;
  uintptr_t key = (uintptr_t)base;
  H7RegionHeader* region = (H7RegionHeader*)base;
  if (region && region->reserved < H7_ROUTE_CAPACITY) {
    size_t slot = (size_t)region->reserved;
    if (g_h7_routes[slot].active && g_h7_routes[slot].base == key) {
      h7_route_clear_slot(slot, region);
      return;
    }
  }
  {
    size_t start = h7_route_hash(key);
    for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
      size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
      if (g_h7_routes[slot].active && g_h7_routes[slot].base == key) {
        h7_route_clear_slot(slot, region);
        return;
      }
    }
  }
}

static int h7_region_matches_route_entry(const H7RegionHeader* region,
                                         const H7RouteEntry* entry) {
  return region->magic == H7_MAGIC &&
         region->cookie == H7_COOKIE &&
         region->kind == entry->kind;
}

static int h7_region_is_active(const H7RegionHeader* region) {
  return (region->flags & H7_REGION_ACTIVE) != 0;
}

static void h7_region_mark_active(H7RegionHeader* region) {
  region->flags = H7_REGION_ACTIVE;
}

static void h7_region_mark_retained(H7RegionHeader* region) {
  region->flags = H7_REGION_RETAINED;
}

static void h7_region_mark_released(H7RegionHeader* region) {
  region->flags = 0;
}

static H7RouteResult h7_route_result_for_entry(const H7RouteEntry* entry) {
  H7RouteResult result;
  H7RegionHeader* region = (H7RegionHeader*)entry->base;
  result.kind = H7_ROUTE_VALID;
  result.region = region;
  if (!h7_region_matches_route_entry(region, entry)) {
    result.kind = H7_ROUTE_INVALID;
  } else if (!h7_region_is_active(region)) {
    result.kind = H7_ROUTE_INVALID;
  }
  return result;
}

static H7RouteResult h7_route_miss(void) {
  H7RouteResult result;
  result.kind = H7_ROUTE_MISS;
  result.region = 0;
  return result;
}

static H7RouteResult h7_route_lookup_exact_base(uintptr_t region_base) {
  size_t i;
  size_t start = h7_route_hash(region_base);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (!g_h7_routes[slot].active) {
      continue;
    }
    if (g_h7_routes[slot].base == region_base) {
      return h7_route_result_for_entry(&g_h7_routes[slot]);
    }
  }
  return h7_route_miss();
}

static H7RouteResult h7_route_lookup_range(uintptr_t addr) {
  size_t i;
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    uintptr_t base;
    uintptr_t end;
    if (!g_h7_routes[i].active) {
      continue;
    }
    base = g_h7_routes[i].base;
    end = base + g_h7_routes[i].size;
    if (addr >= base && addr < end) {
      return h7_route_result_for_entry(&g_h7_routes[i]);
    }
  }
  return h7_route_miss();
}

static H7RouteResult h7_route_lookup_raw(const void* ptr) {
  H7RouteResult result;
  uintptr_t region_base;
  uintptr_t addr = (uintptr_t)ptr;
  if (!ptr) {
    return h7_route_miss();
  }
  region_base = h7_region_base_from_ptr(ptr);
  result = h7_route_lookup_exact_base(region_base);
  if (result.kind != H7_ROUTE_MISS) {
    return result;
  }

  /* Direct regions can span multiple 64KiB chunks; keep INVALID semantics for
     interior pointers by falling back to a bounded range scan on miss. */
  return h7_route_lookup_range(addr);
}

static uint64_t* h7_span_bitmap(H7Span* span) {
  return (uint64_t*)((unsigned char*)span + sizeof(H7Span));
}

static unsigned char* h7_span_slots(H7Span* span) {
  return (unsigned char*)span + span->slot_offset;
}

static unsigned char* h7_span_slot_ptr(H7Span* span, uint32_t index) {
  return h7_span_slots(span) + (size_t)index * span->slot_size;
}

static int h7_bitmap_test(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  return (bitmap[index / 64u] & (UINT64_C(1) << (index % 64u))) != 0;
}

static int h7_small_slot_index(H7Span* span, void* ptr, uint32_t* out_index) {
  uintptr_t slots;
  uintptr_t addr;
  uint32_t index;
  if (!span || !ptr || span->slot_size == 0) {
    return 0;
  }
  slots = (uintptr_t)h7_span_slots(span);
  addr = (uintptr_t)ptr;
  if (addr < slots || addr >= (uintptr_t)span + H7_SPAN_BYTES) {
    return 0;
  }
  if (((addr - slots) % span->slot_size) != 0) {
    return 0;
  }
  index = (uint32_t)((addr - slots) / span->slot_size);
  if (index >= span->slot_count || !h7_bitmap_test(span, index)) {
    return 0;
  }
  if (out_index) {
    *out_index = index;
  }
  return 1;
}

static void h7_bitmap_set(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  bitmap[index / 64u] |= (UINT64_C(1) << (index % 64u));
}

static void h7_bitmap_clear(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  bitmap[index / 64u] &= ~(UINT64_C(1) << (index % 64u));
}

static uint32_t* h7_span_free_slot_next_cell(H7Span* span, uint32_t index) {
  return (uint32_t*)h7_span_slot_ptr(span, index);
}

static uint32_t h7_span_free_slot_next(H7Span* span, uint32_t index) {
  return *h7_span_free_slot_next_cell(span, index);
}

static void h7_span_set_free_slot_next(H7Span* span,
                                       uint32_t index,
                                       uint32_t next) {
  *h7_span_free_slot_next_cell(span, index) = next;
}

static uint32_t h7_span_pop_free_slot(H7Span* span) {
  uint32_t index = span->free_head;
  if (index != H7_FREE_NONE) {
    span->free_head = h7_span_free_slot_next(span, index);
  }
  return index;
}

static void h7_span_push_free_slot(H7Span* span, uint32_t index) {
  h7_span_set_free_slot_next(span, index, span->free_head);
  span->free_head = index;
}

static void h7_list_remove(H7Span** head, H7Span* span) {
  if (!span) {
    return;
  }
  if (span->prev) {
    span->prev->next = span->next;
  } else if (*head == span) {
    *head = span->next;
  }
  if (span->next) {
    span->next->prev = span->prev;
  }
  span->next = 0;
  span->prev = 0;
}

static void h7_list_push(H7Span** head, H7Span* span) {
  span->prev = 0;
  span->next = *head;
  if (*head) {
    (*head)->prev = span;
  }
  *head = span;
}

static H7Span* h7_empty_span_pop(H7Class* klass) {
  H7Span* span = klass->empty;
  if (!span) {
    return 0;
  }
  klass->empty = span->next;
  if (klass->empty) {
    klass->empty->prev = 0;
  }
  span->next = 0;
  span->prev = 0;
  --klass->empty_count;
  return span;
}

static int h7_empty_span_try_push(H7Class* klass, H7Span* span) {
  if (klass->empty_count >= H7_EMPTY_SPAN_CAP) {
    return 0;
  }
  h7_list_push(&klass->empty, span);
  ++klass->empty_count;
  return 1;
}

static void* h7_os_alloc(size_t size) {
#ifdef _WIN32
  return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
  void* p = mmap(0, size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return p == MAP_FAILED ? 0 : p;
#endif
}

static void h7_os_free(void* ptr, size_t size) {
  if (!ptr) {
    return;
  }
#ifdef _WIN32
  (void)size;
  VirtualFree(ptr, 0, MEM_RELEASE);
#else
  munmap(ptr, size);
#endif
}

static void h7_pending_release_clear(H7PendingRelease* release) {
  release->ptr = 0;
  release->size = 0;
}

static void h7_pending_release_set(H7PendingRelease* release,
                                   void* ptr,
                                   size_t size) {
  release->ptr = ptr;
  release->size = size;
}

static void h7_pending_release_now(H7PendingRelease* release) {
  if (release->ptr) {
    h7_os_free(release->ptr, release->size);
    h7_pending_release_clear(release);
  }
}

static void* h7_os_alloc_region(size_t region_size) {
#ifdef _WIN32
  return h7_os_alloc(region_size);
#else
  size_t request = region_size + H7_SPAN_BYTES;
  void* raw = h7_os_alloc(request);
  if (!raw) {
    return 0;
  }
  uintptr_t base = (uintptr_t)raw;
  uintptr_t aligned = (base + H7_SPAN_BYTES - 1u) &
                      ~((uintptr_t)H7_SPAN_BYTES - 1u);
  size_t prefix = aligned - base;
  size_t suffix = request - prefix - region_size;
  if (prefix) {
    h7_os_free((void*)base, prefix);
  }
  if (suffix) {
    h7_os_free((void*)(aligned + region_size), suffix);
  }
  return (void*)aligned;
#endif
}

static int h7_class_for_size(size_t size) {
  size_t count = sizeof(g_h7_classes) / sizeof(g_h7_classes[0]);
  size_t i;
  for (i = 0; i < count; ++i) {
    if (size <= g_h7_classes[i].slot_size) {
      return (int)i;
    }
  }
  return -1;
}

static uint32_t h7_span_slot_count(uint32_t slot_size,
                                   uint32_t* out_bitmap_words,
                                   uint32_t* out_slot_offset) {
  size_t header = sizeof(H7Span);
  size_t max_slots = (H7_SPAN_BYTES - h7_align_up(header, slot_size)) /
                     slot_size;
  for (;;) {
    size_t bitmap_words = (max_slots + 63u) / 64u;
    size_t slot_offset =
        h7_align_up(header + bitmap_words * sizeof(uint64_t), slot_size);
    size_t slot_count = (H7_SPAN_BYTES - slot_offset) / slot_size;
    if (slot_count == max_slots) {
      *out_bitmap_words = (uint32_t)bitmap_words;
      *out_slot_offset = (uint32_t)slot_offset;
      return (uint32_t)slot_count;
    }
    max_slots = slot_count;
  }
}

static void h7_span_prepare_region(H7Span* span, uint16_t class_id) {
  H7Class* klass = &g_h7_classes[class_id];
  uint32_t bitmap_words = 0;
  uint32_t slot_offset = 0;
  uint32_t slot_count =
      h7_span_slot_count(klass->slot_size, &bitmap_words, &slot_offset);
  uint32_t i;
  memset(span, 0, sizeof(H7Span));
  h7_region_header_init(&span->region,
                        H7_REGION_SMALL_SPAN,
                        H7_REGION_ACTIVE,
                        H7_SPAN_BYTES);
  span->class_id = class_id;
  span->slot_size = klass->slot_size;
  span->slot_count = slot_count;
  span->free_head = 0;
  span->bitmap_words = bitmap_words;
  span->slot_offset = slot_offset;
  for (i = 0; i < slot_count; ++i) {
    h7_span_set_free_slot_next(
        span, i, (i + 1u < slot_count) ? i + 1u : H7_FREE_NONE);
  }
}

static void h7_span_mark_committed(H7Span* span) {
  g_h7_stats.reserved_bytes += span->region.region_size;
  ++g_h7_stats.span_count;
}

static void h7_span_mark_released(H7Span* span) {
  g_h7_stats.reserved_bytes -= span->region.region_size;
  --g_h7_stats.span_count;
}

static int h7_span_commit_prepared(H7Span* span) {
  if (!h7_route_register(span, H7_SPAN_BYTES, H7_REGION_SMALL_SPAN)) {
    return 0;
  }
  h7_span_mark_committed(span);
  return 1;
}

static int h7_span_is_prepared_region(H7Span* span) {
  return span && span->region.magic == H7_MAGIC &&
         span->region.kind == H7_REGION_SMALL_SPAN;
}

static void h7_span_detach_for_release(H7Span* span,
                                       H7PendingRelease* release) {
  if (!span) {
    return;
  }
  h7_route_unregister(span);
  h7_span_mark_released(span);
  h7_region_mark_released(&span->region);
  h7_pending_release_set(release, span, span->region.region_size);
}

static void h7_span_mark_slot_active(H7Span* span) {
  ++span->used_count;
  g_h7_stats.active_bytes += span->slot_size;
}

static void h7_span_mark_slot_inactive(H7Span* span) {
  --span->used_count;
  g_h7_stats.active_bytes -= span->slot_size;
}

static void* h7_small_alloc_from_span(H7Span* span) {
  H7Class* klass = &g_h7_classes[span->class_id];
  uint32_t index = h7_span_pop_free_slot(span);
  unsigned char* ptr;
  if (index == H7_FREE_NONE) {
    return 0;
  }
  ptr = h7_span_slot_ptr(span, index);
  h7_bitmap_set(span, index);
  h7_span_mark_slot_active(span);
  if (span->free_head == H7_FREE_NONE) {
    h7_list_remove(&klass->partial, span);
  } else if (span->used_count == 1u) {
    h7_list_push(&klass->partial, span);
  }
  return ptr;
}

static void* h7_small_alloc_existing(size_t size) {
  int class_id = h7_class_for_size(size);
  H7Class* klass;
  H7Span* span;
  if (class_id < 0) {
    return 0;
  }
  klass = &g_h7_classes[class_id];
  span = klass->partial;
  if (!span) {
    span = h7_empty_span_pop(klass);
  }
  if (!span) {
    return 0;
  }
  return h7_small_alloc_from_span(span);
}

static void* h7_small_commit_and_alloc(H7Span* prepared) {
  if (!h7_span_is_prepared_region(prepared)) {
    return 0;
  }
  if (!h7_span_commit_prepared(prepared)) {
    return 0;
  }
  return h7_small_alloc_from_span(prepared);
}

static void h7_small_free(H7Span* span,
                          void* ptr,
                          H7PendingRelease* release) {
  H7Class* klass;
  uint32_t index;
  int was_full;
  if (!span || span->region.magic != H7_MAGIC ||
      span->region.kind != H7_REGION_SMALL_SPAN ||
      span->class_id >= sizeof(g_h7_classes) / sizeof(g_h7_classes[0])) {
    return;
  }
  klass = &g_h7_classes[span->class_id];
  if (!h7_small_slot_index(span, ptr, &index)) {
    return;
  }
  was_full = span->used_count == span->slot_count;
  h7_bitmap_clear(span, index);
  h7_span_push_free_slot(span, index);
  h7_span_mark_slot_inactive(span);
  if (span->used_count == 0) {
    h7_list_remove(&klass->partial, span);
    if (!h7_empty_span_try_push(klass, span)) {
      h7_span_detach_for_release(span, release);
    }
  } else if (was_full) {
    h7_list_push(&klass->partial, span);
  }
}

static size_t h7_big_user_offset(void) {
  return h7_align_up(sizeof(H7Direct), 16u);
}

static void* h7_big_user_ptr(H7Direct* direct) {
  return (unsigned char*)direct + h7_big_user_offset();
}

static int h7_big_is_user_ptr(H7Direct* direct, void* ptr) {
  return direct && ptr == h7_big_user_ptr(direct);
}

static size_t h7_big_region_size(size_t size) {
  return h7_region_align_up(h7_big_user_offset() + size);
}

static void h7_big_mark_active(H7Direct* direct, size_t size) {
  h7_region_mark_active(&direct->region);
  direct->requested_size = size;
  g_h7_stats.active_bytes += size;
  ++g_h7_stats.direct_count;
}

static void h7_big_mark_inactive(H7Direct* direct) {
  g_h7_stats.active_bytes -= direct->requested_size;
  --g_h7_stats.direct_count;
}

static void h7_big_mark_committed(H7Direct* direct) {
  g_h7_stats.reserved_bytes += direct->region.region_size;
}

static void h7_big_mark_released(H7Direct* direct) {
  g_h7_stats.reserved_bytes -= direct->region.region_size;
}

static void* h7_big_alloc_retained(size_t size) {
  H7Direct* direct = h7_direct_retain_pop(size);
  if (!direct) {
    return 0;
  }
  h7_big_mark_active(direct, size);
  return h7_big_user_ptr(direct);
}

static int h7_big_prepare_region(H7Direct* direct,
                                 size_t size,
                                 size_t region_size) {
  size_t user_offset = h7_big_user_offset();
  memset(direct, 0, user_offset);
  h7_region_header_init(&direct->region,
                        H7_REGION_DIRECT,
                        H7_REGION_ACTIVE,
                        region_size);
  direct->requested_size = size;
  return 1;
}

static int h7_big_commit_prepared(H7Direct* direct) {
  if (!h7_route_register(direct,
                         direct->region.region_size,
                         H7_REGION_DIRECT)) {
    return 0;
  }
  h7_big_mark_active(direct, direct->requested_size);
  h7_big_mark_committed(direct);
  return 1;
}

static int h7_big_is_prepared_region(H7Direct* direct) {
  return direct && direct->region.magic == H7_MAGIC &&
         direct->region.kind == H7_REGION_DIRECT;
}

static void* h7_big_commit_and_alloc(H7Direct* direct) {
  if (!h7_big_is_prepared_region(direct) || !h7_big_commit_prepared(direct)) {
    return 0;
  }
  return h7_big_user_ptr(direct);
}

static void* h7_big_alloc_locked(size_t size) {
  return h7_big_alloc_retained(size);
}

static void* h7_big_alloc_region_outside_lock(size_t size,
                                              H7PendingRelease* release) {
  size_t region_size = h7_big_region_size(size);
  H7Direct* direct = (H7Direct*)h7_os_alloc_region(region_size);
  if (!direct) {
    return 0;
  }
  h7_big_prepare_region(direct, size, region_size);
  h7_pending_release_set(release, direct, region_size);
  return direct;
}

static void h7_big_move_to_retained(H7Direct* direct) {
  h7_big_mark_inactive(direct);
  h7_region_mark_retained(&direct->region);
}

static void h7_big_detach_for_release(H7Direct* direct,
                                      H7PendingRelease* release) {
  h7_route_unregister(direct);
  h7_big_mark_inactive(direct);
  h7_big_mark_released(direct);
  h7_region_mark_released(&direct->region);
  h7_pending_release_set(release, direct, direct->region.region_size);
}

static int h7_malloc_prepare_region_outside_lock(size_t size,
                                                 H7PendingRelease* prealloc) {
  if (h7_size_is_small(size)) {
    int class_id = h7_class_for_size(size);
    void* span;
    if (class_id < 0) {
      return 0;
    }
    span = h7_os_alloc_region(H7_SPAN_BYTES);
    if (span) {
      h7_pending_release_set(prealloc, span, H7_SPAN_BYTES);
      h7_span_prepare_region((H7Span*)span, (uint16_t)class_id);
    }
  } else {
    (void)h7_big_alloc_region_outside_lock(size, prealloc);
  }
  return prealloc->ptr != 0;
}

static void h7_big_free(H7Direct* direct,
                        void* ptr,
                        H7PendingRelease* release) {
  if (!direct || direct->region.magic != H7_MAGIC ||
      direct->region.kind != H7_REGION_DIRECT ||
      (direct->region.flags & H7_REGION_ACTIVE) == 0 ||
      !h7_big_is_user_ptr(direct, ptr)) {
    return;
  }
  if (h7_direct_retain_push(direct)) {
    h7_big_move_to_retained(direct);
    return;
  }
  h7_big_detach_for_release(direct, release);
}

static void* h7_malloc_existing_locked(size_t size) {
  if (h7_size_is_small(size)) {
    return h7_small_alloc_existing(size);
  }
  return h7_big_alloc_locked(size);
}

static void* h7_malloc_commit_preallocated_locked(size_t size,
                                                  H7PendingRelease* prealloc) {
  void* ptr = h7_malloc_existing_locked(size);
  if (!ptr) {
    if (h7_size_is_small(size)) {
      ptr = h7_small_commit_and_alloc((H7Span*)prealloc->ptr);
    } else {
      ptr = h7_big_commit_and_alloc((H7Direct*)prealloc->ptr);
    }
    if (ptr) {
      h7_pending_release_clear(prealloc);
    }
  }
  return ptr;
}

void* h7_malloc(size_t size) {
  H7PendingRelease prealloc;
  void* ptr = 0;
  if (size == 0) {
    return 0;
  }
  h7_pending_release_clear(&prealloc);

  h7_lock();
  ptr = h7_malloc_existing_locked(size);
  h7_unlock();
  if (ptr) {
    return ptr;
  }

  if (!h7_malloc_prepare_region_outside_lock(size, &prealloc)) {
    return 0;
  }

  h7_lock();
  ptr = h7_malloc_commit_preallocated_locked(size, &prealloc);
  h7_unlock();

  h7_pending_release_now(&prealloc);
  return ptr;
}

void* h7_calloc(size_t count, size_t size) {
  size_t total;
  void* ptr;
  if (count != 0 && size > ((size_t)-1) / count) {
    return 0;
  }
  total = count * size;
  ptr = h7_malloc(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

static void h7_free_locked(void* ptr, H7PendingRelease* release) {
  H7RouteResult route = h7_route_lookup_raw(ptr);
  if (route.kind != H7_ROUTE_VALID || !route.region) {
    return;
  }
  if (route.region->kind == H7_REGION_SMALL_SPAN) {
    h7_small_free((H7Span*)route.region, ptr, release);
  } else if (route.region->kind == H7_REGION_DIRECT) {
    h7_big_free((H7Direct*)route.region, ptr, release);
  }
}

void h7_free(void* ptr) {
  H7PendingRelease release;
  if (!ptr) {
    return;
  }
  h7_pending_release_clear(&release);
  h7_lock();
  h7_free_locked(ptr, &release);
  h7_unlock();
  h7_pending_release_now(&release);
}

static H7RouteKind h7_route_unlocked(void* ptr) {
  H7RouteResult route = h7_route_lookup_raw(ptr);
  if (route.kind != H7_ROUTE_VALID || !route.region) {
    return route.kind;
  }
  if (route.region->kind == H7_REGION_SMALL_SPAN) {
    return h7_small_slot_index((H7Span*)route.region, ptr, 0)
               ? H7_ROUTE_VALID
               : H7_ROUTE_INVALID;
  }
  if (route.region->kind == H7_REGION_DIRECT) {
    return h7_big_is_user_ptr((H7Direct*)route.region, ptr) ? H7_ROUTE_VALID
                                                            : H7_ROUTE_INVALID;
  }
  return H7_ROUTE_INVALID;
}

H7RouteKind h7_route(void* ptr) {
  H7RouteKind kind;
  h7_lock();
  kind = h7_route_unlocked(ptr);
  h7_unlock();
  return kind;
}

static H7Stats h7_stats_locked(void) {
  return g_h7_stats;
}

H7Stats h7_stats(void) {
  H7Stats stats;
  h7_lock();
  stats = h7_stats_locked();
  h7_unlock();
  return stats;
}
