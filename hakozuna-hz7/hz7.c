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

#define H7_SPAN_CLASS_MAX 16384u
#define H7_DIRECT_MIN (H7_SPAN_CLASS_MAX + 1u)
#define H7_SPAN_BYTES (64u * 1024u)
#define H7_EMPTY_SPAN_CAP 1u
#define H7_FREE_NONE UINT32_MAX
#define H7_MAGIC 0x48375A37u
#define H7_COOKIE 0x7A17C0DEu
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

static H7Class g_h7_classes[] = {
    {16u, 0, 0, 0},   {32u, 0, 0, 0},   {64u, 0, 0, 0},
    {128u, 0, 0, 0},  {256u, 0, 0, 0},  {512u, 0, 0, 0},
    {1024u, 0, 0, 0}, {2048u, 0, 0, 0}, {4096u, 0, 0, 0},
    {8192u, 0, 0, 0}, {16384u, 0, 0, 0},
};

static H7Stats g_h7_stats;
static H7RouteEntry g_h7_routes[H7_ROUTE_CAPACITY];

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

static size_t h7_route_hash(uintptr_t base) {
  return (size_t)((base >> 16u) & (H7_ROUTE_CAPACITY - 1u));
}

static int h7_route_register(void* base, size_t size, H7RegionKind kind) {
  size_t i;
  uintptr_t key = (uintptr_t)base;
  size_t start = h7_route_hash(key);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (!g_h7_routes[slot].active) {
      g_h7_routes[slot].base = key;
      g_h7_routes[slot].size = size;
      g_h7_routes[slot].kind = kind;
      g_h7_routes[slot].active = 1u;
      ++g_h7_stats.route_count;
      return 1;
    }
  }
  ++g_h7_stats.route_register_fail;
  return 0;
}

static void h7_route_unregister(void* base) {
  size_t i;
  uintptr_t key = (uintptr_t)base;
  size_t start = h7_route_hash(key);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (g_h7_routes[slot].active && g_h7_routes[slot].base == key) {
      g_h7_routes[slot].active = 0u;
      g_h7_routes[slot].base = 0u;
      g_h7_routes[slot].size = 0u;
      g_h7_routes[slot].kind = 0;
      --g_h7_stats.route_count;
      return;
    }
  }
}

static H7RouteResult h7_route_result_for_entry(const H7RouteEntry* entry) {
  H7RouteResult result;
  H7RegionHeader* region = (H7RegionHeader*)entry->base;
  result.kind = H7_ROUTE_VALID;
  result.region = region;
  if (region->magic != H7_MAGIC || region->cookie != H7_COOKIE ||
      region->kind != entry->kind) {
    result.kind = H7_ROUTE_INVALID;
  }
  return result;
}

static H7RouteResult h7_route_lookup_raw(const void* ptr) {
  H7RouteResult result;
  uintptr_t region_base;
  uintptr_t addr = (uintptr_t)ptr;
  size_t i;
  size_t start;
  result.kind = H7_ROUTE_MISS;
  result.region = 0;
  if (!ptr) {
    return result;
  }
  region_base = h7_region_base_from_ptr(ptr);
  start = h7_route_hash(region_base);
  for (i = 0; i < H7_ROUTE_CAPACITY; ++i) {
    size_t slot = (start + i) & (H7_ROUTE_CAPACITY - 1u);
    if (!g_h7_routes[slot].active) {
      continue;
    }
    if (g_h7_routes[slot].base == region_base) {
      return h7_route_result_for_entry(&g_h7_routes[slot]);
    }
  }

  /* Direct regions can span multiple 64KiB chunks; keep INVALID semantics for
     interior pointers by falling back to a bounded range scan on miss. */
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
  return result;
}

static uint64_t* h7_span_bitmap(H7Span* span) {
  return (uint64_t*)((unsigned char*)span + sizeof(H7Span));
}

static unsigned char* h7_span_slots(H7Span* span) {
  return (unsigned char*)span + span->slot_offset;
}

static int h7_bitmap_test(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  return (bitmap[index / 64u] & (UINT64_C(1) << (index % 64u))) != 0;
}

static void h7_bitmap_set(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  bitmap[index / 64u] |= (UINT64_C(1) << (index % 64u));
}

static void h7_bitmap_clear(H7Span* span, uint32_t index) {
  uint64_t* bitmap = h7_span_bitmap(span);
  bitmap[index / 64u] &= ~(UINT64_C(1) << (index % 64u));
}

static void h7_list_remove(H7Span** head, H7Span* span) {
  H7Span** cursor = head;
  while (*cursor) {
    if (*cursor == span) {
      *cursor = span->next;
      span->next = 0;
      return;
    }
    cursor = &(*cursor)->next;
  }
}

static void h7_list_push(H7Span** head, H7Span* span) {
  span->next = *head;
  *head = span;
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

static H7Span* h7_span_create(uint16_t class_id) {
  H7Class* klass = &g_h7_classes[class_id];
  uint32_t bitmap_words = 0;
  uint32_t slot_offset = 0;
  uint32_t slot_count =
      h7_span_slot_count(klass->slot_size, &bitmap_words, &slot_offset);
  H7Span* span = (H7Span*)h7_os_alloc_region(H7_SPAN_BYTES);
  uint32_t i;
  if (!span) {
    return 0;
  }
  memset(span, 0, H7_SPAN_BYTES);
  span->region.magic = H7_MAGIC;
  span->region.cookie = H7_COOKIE;
  span->region.kind = H7_REGION_SMALL_SPAN;
  span->region.region_size = H7_SPAN_BYTES;
  span->class_id = class_id;
  span->slot_size = klass->slot_size;
  span->slot_count = slot_count;
  span->free_head = 0;
  span->bitmap_words = bitmap_words;
  span->slot_offset = slot_offset;
  if (!h7_route_register(span, H7_SPAN_BYTES, H7_REGION_SMALL_SPAN)) {
    h7_os_free(span, H7_SPAN_BYTES);
    return 0;
  }
  for (i = 0; i < slot_count; ++i) {
    uint32_t* next =
        (uint32_t*)(h7_span_slots(span) + (size_t)i * span->slot_size);
    *next = (i + 1u < slot_count) ? i + 1u : H7_FREE_NONE;
  }
  g_h7_stats.reserved_bytes += H7_SPAN_BYTES;
  ++g_h7_stats.span_count;
  return span;
}

static void h7_span_destroy(H7Span* span) {
  if (!span) {
    return;
  }
  h7_route_unregister(span);
  g_h7_stats.reserved_bytes -= span->region.region_size;
  --g_h7_stats.span_count;
  h7_os_free(span, span->region.region_size);
}

static void* h7_small_alloc(size_t size) {
  int class_id = h7_class_for_size(size);
  H7Class* klass;
  H7Span* span;
  uint32_t index;
  unsigned char* ptr;
  if (class_id < 0) {
    return 0;
  }
  klass = &g_h7_classes[class_id];
  span = klass->partial;
  if (!span) {
    span = klass->empty;
    if (span) {
      klass->empty = span->next;
      span->next = 0;
      --klass->empty_count;
    }
  }
  if (!span) {
    span = h7_span_create((uint16_t)class_id);
    if (!span) {
      return 0;
    }
  }
  index = span->free_head;
  if (index == H7_FREE_NONE) {
    return 0;
  }
  ptr = h7_span_slots(span) + (size_t)index * span->slot_size;
  span->free_head = *(uint32_t*)ptr;
  h7_bitmap_set(span, index);
  ++span->used_count;
  g_h7_stats.active_bytes += span->slot_size;
  if (span->free_head == H7_FREE_NONE) {
    h7_list_remove(&klass->partial, span);
  } else if (span->used_count == 1u) {
    h7_list_push(&klass->partial, span);
  }
  return ptr;
}

static void h7_small_free(H7Span* span, void* ptr) {
  H7Class* klass;
  uintptr_t slots;
  uintptr_t addr;
  uint32_t index;
  int was_full;
  if (!span || span->region.magic != H7_MAGIC ||
      span->region.kind != H7_REGION_SMALL_SPAN ||
      span->class_id >= sizeof(g_h7_classes) / sizeof(g_h7_classes[0])) {
    return;
  }
  klass = &g_h7_classes[span->class_id];
  slots = (uintptr_t)h7_span_slots(span);
  addr = (uintptr_t)ptr;
  if (addr < slots || addr >= (uintptr_t)span + H7_SPAN_BYTES) {
    return;
  }
  if (((addr - slots) % span->slot_size) != 0) {
    return;
  }
  index = (uint32_t)((addr - slots) / span->slot_size);
  if (index >= span->slot_count || !h7_bitmap_test(span, index)) {
    return;
  }
  was_full = span->used_count == span->slot_count;
  h7_bitmap_clear(span, index);
  *(uint32_t*)ptr = span->free_head;
  span->free_head = index;
  --span->used_count;
  g_h7_stats.active_bytes -= span->slot_size;
  if (span->used_count == 0) {
    h7_list_remove(&klass->partial, span);
    if (klass->empty_count < H7_EMPTY_SPAN_CAP) {
      h7_list_push(&klass->empty, span);
      ++klass->empty_count;
    } else {
      h7_span_destroy(span);
    }
  } else if (was_full) {
    h7_list_push(&klass->partial, span);
  }
}

static void* h7_big_alloc(size_t size) {
  size_t user_offset = h7_align_up(sizeof(H7Direct), 16u);
  size_t region_size = h7_region_align_up(user_offset + size);
  H7Direct* direct = (H7Direct*)h7_os_alloc_region(region_size);
  if (!direct) {
    return 0;
  }
  memset(direct, 0, user_offset);
  direct->region.magic = H7_MAGIC;
  direct->region.cookie = H7_COOKIE;
  direct->region.kind = H7_REGION_DIRECT;
  direct->region.region_size = region_size;
  direct->requested_size = size;
  if (!h7_route_register(direct, region_size, H7_REGION_DIRECT)) {
    h7_os_free(direct, region_size);
    return 0;
  }
  g_h7_stats.active_bytes += size;
  g_h7_stats.reserved_bytes += region_size;
  ++g_h7_stats.direct_count;
  return (unsigned char*)direct + user_offset;
}

static void h7_big_free(H7Direct* direct, void* ptr) {
  size_t user_offset = h7_align_up(sizeof(H7Direct), 16u);
  if (!direct || direct->region.magic != H7_MAGIC ||
      direct->region.kind != H7_REGION_DIRECT ||
      ptr != (unsigned char*)direct + user_offset) {
    return;
  }
  h7_route_unregister(direct);
  g_h7_stats.active_bytes -= direct->requested_size;
  g_h7_stats.reserved_bytes -= direct->region.region_size;
  --g_h7_stats.direct_count;
  h7_os_free(direct, direct->region.region_size);
}

static void* h7_malloc_unlocked(size_t size) {
  if (size == 0) {
    return 0;
  }
  if (size <= H7_SPAN_CLASS_MAX) {
    return h7_small_alloc(size);
  }
  return h7_big_alloc(size);
}

void* h7_malloc(size_t size) {
  void* ptr;
  h7_lock();
  ptr = h7_malloc_unlocked(size);
  h7_unlock();
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

void h7_free(void* ptr) {
  H7RouteResult route;
  if (!ptr) {
    return;
  }
  h7_lock();
  route = h7_route_lookup_raw(ptr);
  if (route.kind != H7_ROUTE_VALID || !route.region) {
    h7_unlock();
    return;
  }
  if (route.region->kind == H7_REGION_SMALL_SPAN) {
    h7_small_free((H7Span*)route.region, ptr);
  } else if (route.region->kind == H7_REGION_DIRECT) {
    h7_big_free((H7Direct*)route.region, ptr);
  }
  h7_unlock();
}

static H7RouteKind h7_route_unlocked(void* ptr) {
  H7RouteResult route = h7_route_lookup_raw(ptr);
  if (route.kind != H7_ROUTE_VALID || !route.region) {
    return route.kind;
  }
  if (route.region->kind == H7_REGION_SMALL_SPAN) {
    H7Span* span = (H7Span*)route.region;
    uintptr_t slots = (uintptr_t)h7_span_slots(span);
    uintptr_t addr = (uintptr_t)ptr;
    uint32_t index;
    if (addr < slots || addr >= (uintptr_t)span + H7_SPAN_BYTES ||
        ((addr - slots) % span->slot_size) != 0) {
      return H7_ROUTE_INVALID;
    }
    index = (uint32_t)((addr - slots) / span->slot_size);
    if (index >= span->slot_count || !h7_bitmap_test(span, index)) {
      return H7_ROUTE_INVALID;
    }
    return H7_ROUTE_VALID;
  }
  if (route.region->kind == H7_REGION_DIRECT) {
    H7Direct* direct = (H7Direct*)route.region;
    size_t user_offset = h7_align_up(sizeof(H7Direct), 16u);
    return ptr == (unsigned char*)direct + user_offset ? H7_ROUTE_VALID
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

H7Stats h7_stats(void) {
  H7Stats stats;
  h7_lock();
  stats = g_h7_stats;
  h7_unlock();
  return stats;
}
