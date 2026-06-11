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
#if defined(H7_REMOTE_NATURAL_PRESET) && !defined(H7_ROUTE_CAPACITY)
#define H7_ROUTE_CAPACITY 16384u
#endif
#ifndef H7_DIRECT_RETAIN_CAP
#define H7_DIRECT_RETAIN_CAP 64u
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

static void h7_pending_release_set(H7PendingRelease* release,
                                   void* ptr,
                                   size_t size);

#include "hz7_route.inc"
#include "hz7_span.inc"

static int h7_small_slot_index(H7Span* span, void* ptr, uint32_t* out_index);
static int h7_big_is_user_ptr(H7Direct* direct, void* ptr);

static H7RouteKind h7_route_kind_for_user_ptr(H7RouteResult route, void* ptr) {
  if (route.kind != H7_ROUTE_VALID || !route.region) {
    return route.kind;
  }
  if (route.region->kind == H7_REGION_SMALL_SPAN) {
    return h7_small_slot_index((H7Span*)route.region, ptr, 0)
               ? H7_ROUTE_VALID
               : H7_ROUTE_INVALID;
  }
  if (route.region->kind == H7_REGION_DIRECT) {
    return h7_big_is_user_ptr((H7Direct*)route.region, ptr)
               ? H7_ROUTE_VALID
               : H7_ROUTE_INVALID;
  }
  return H7_ROUTE_INVALID;
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

#include "hz7_big.inc"
