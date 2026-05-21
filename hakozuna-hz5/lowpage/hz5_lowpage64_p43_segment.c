#include "hz5_lowpage64_p43_segment.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef HZ5_DIAGNOSTIC_STATS
#define HZ5_DIAGNOSTIC_STATS 0
#endif

#ifndef HZ5_LOWPAGE64_STATS
#ifdef BENCHLAB_HZ5_P25_STATS
#define HZ5_LOWPAGE64_STATS BENCHLAB_HZ5_P25_STATS
#else
#define HZ5_LOWPAGE64_STATS HZ5_DIAGNOSTIC_STATS
#endif
#endif

#if HZ5_LOWPAGE64_STATS
#define HZ5_P43_COUNT_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
#else
#define HZ5_P43_COUNT_ADD(counter, value) ((void)0)
#endif

#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#if HZ5_LOWPAGE64_P43_SLOT_COUNT > 32u
#error "P43 segment slot bitmap currently supports up to 32 slots"
#endif

#define HZ5_LOWPAGE64_P43_COMMITTED_LISTS \
  (HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS || \
   HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP > 0u || \
   HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u)

typedef struct Hz5Lowpage64Segment Hz5Lowpage64Segment;

typedef struct Hz5Lowpage64P43SlotRef {
  Hz5Lowpage64Segment* seg;
  uint32_t slot;
} Hz5Lowpage64P43SlotRef;

struct Hz5Lowpage64Segment {
  void* base;
  uint32_t allocated_mask;
  uint32_t committed_mask;
  uint32_t cold_mask;
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
  Hz5Lowpage64P43SlotRef free_next[HZ5_LOWPAGE64_P43_SLOT_COUNT];
#endif
  Hz5Lowpage64Segment* next;
};

static INIT_ONCE g_hz5_lowpage64_p43_lock_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION g_hz5_lowpage64_p43_lock;
static Hz5Lowpage64Segment* g_hz5_lowpage64_p43_segments;
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static Hz5Lowpage64P43SlotRef g_hz5_lowpage64_p43_committed_head;
static size_t g_hz5_lowpage64_p43_committed_count;
#endif
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u
typedef struct Hz5Lowpage64P43TlsCache {
  Hz5Lowpage64P43SlotRef refs[HZ5_LOWPAGE64_P43_TLS_CACHE_CAP];
  uint32_t count;
} Hz5Lowpage64P43TlsCache;

__declspec(thread) static Hz5Lowpage64P43TlsCache
    g_hz5_lowpage64_p43_tls_cache;
#endif
#endif

#if HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_p43_segments_reserved;
static _Atomic size_t g_hz5_lowpage64_p43_segments_released;
static _Atomic size_t g_hz5_lowpage64_p43_slot_commits;
static _Atomic size_t g_hz5_lowpage64_p43_slot_commit_failures;
static _Atomic size_t g_hz5_lowpage64_p43_slot_decommits;
static _Atomic size_t g_hz5_lowpage64_p43_slot_decommit_failures;
static _Atomic size_t g_hz5_lowpage64_p43_tls_hits;
static _Atomic size_t g_hz5_lowpage64_p43_tls_pushes;
static _Atomic size_t g_hz5_lowpage64_p43_tls_overflow_pushes;
static _Atomic size_t g_hz5_lowpage64_p43_global_hits;
static _Atomic size_t g_hz5_lowpage64_p43_global_pushes;
static _Atomic size_t g_hz5_lowpage64_p43_cold_hits;
static _Atomic size_t g_hz5_lowpage64_p43_free_slot_hits;
static _Atomic size_t g_hz5_lowpage64_p43_va_allocs;
static _Atomic size_t g_hz5_lowpage64_p43_va_alloc_failures;
static _Atomic size_t g_hz5_lowpage64_p43_va_releases;
#endif

#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
static BOOL CALLBACK hz5_lowpage64_p43_init_lock(PINIT_ONCE once,
                                                 PVOID param,
                                                 PVOID* context) {
  (void)once;
  (void)param;
  (void)context;
  InitializeCriticalSection(&g_hz5_lowpage64_p43_lock);
  return TRUE;
}

static void hz5_lowpage64_p43_lock_enter(void) {
  InitOnceExecuteOnce(&g_hz5_lowpage64_p43_lock_once,
                      hz5_lowpage64_p43_init_lock, NULL, NULL);
  EnterCriticalSection(&g_hz5_lowpage64_p43_lock);
}

static void hz5_lowpage64_p43_lock_leave(void) {
  LeaveCriticalSection(&g_hz5_lowpage64_p43_lock);
}

static Hz5Lowpage64Segment* hz5_lowpage64_p43_find_segment_locked(
    void* raw,
    uint32_t* slot_out) {
  uintptr_t p = (uintptr_t)raw;
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    uintptr_t base = (uintptr_t)seg->base;
    uintptr_t end = base + HZ5_LOWPAGE64_P43_SEGMENT_SIZE;
    if (p < base || p >= end) {
      continue;
    }
    uintptr_t offset = p - base;
    if ((offset % HZ5_LOWPAGE64_P43_SLOT_SIZE) != 0) {
      return NULL;
    }
    uint32_t slot = (uint32_t)(offset / HZ5_LOWPAGE64_P43_SLOT_SIZE);
    if (slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
      return NULL;
    }
    if (slot_out) {
      *slot_out = slot;
    }
    return seg;
  }
  return NULL;
}

static void hz5_lowpage64_p43_unlink_segment_locked(
    Hz5Lowpage64Segment* target) {
  Hz5Lowpage64Segment** cur = &g_hz5_lowpage64_p43_segments;
  while (*cur) {
    if (*cur == target) {
      *cur = target->next;
      return;
    }
    cur = &(*cur)->next;
  }
}

#if (HZ5_LOWPAGE64_P43_SLOT_DECOMMIT || \
     HZ5_LOWPAGE64_P43_PAGE_NOACCESS) && \
    HZ5_LOWPAGE64_P43_REWARM_BATCH > 1u
static uint32_t hz5_lowpage64_p43_first_set_bit(uint32_t mask) {
  uint32_t slot = 0;
  while (((mask >> slot) & 1u) == 0u) {
    slot++;
  }
  return slot;
}

static int hz5_lowpage64_p43_rewarm_slot(Hz5Lowpage64Segment* seg,
                                         uint32_t slot) {
  uint32_t bit = (uint32_t)1u << slot;
  void* raw = (void*)((uintptr_t)seg->base +
                      (uintptr_t)slot * HZ5_LOWPAGE64_P43_SLOT_SIZE);
  seg->allocated_mask |= bit;
  hz5_lowpage64_p43_lock_leave();

#if HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  DWORD old_protect = 0;
  int ok = VirtualProtect(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, PAGE_READWRITE,
                          &old_protect) != 0;
#else
  int ok = VirtualAlloc(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, MEM_COMMIT,
                        PAGE_READWRITE) != NULL;
#endif

  hz5_lowpage64_p43_lock_enter();
  if (!ok) {
    seg->allocated_mask &= ~bit;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
    return 0;
  }

  seg->cold_mask &= ~bit;
  seg->committed_mask |= bit;
  seg->allocated_mask &= ~bit;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
  return 1;
}

static void hz5_lowpage64_p43_rewarm_segment_neighbors(
    Hz5Lowpage64Segment* seg,
    uint32_t already_allocated_slot) {
  size_t warmed = 1u;
  while (warmed < (size_t)HZ5_LOWPAGE64_P43_REWARM_BATCH) {
    uint32_t candidates = seg->cold_mask & ~seg->allocated_mask;
    candidates &= ~((uint32_t)1u << already_allocated_slot);
    if (!candidates) {
      return;
    }

    uint32_t slot = hz5_lowpage64_p43_first_set_bit(candidates);
    if (!hz5_lowpage64_p43_rewarm_slot(seg, slot)) {
      return;
    }
    warmed++;
  }
}
#endif

#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
static void hz5_lowpage64_p43_ref_clear(Hz5Lowpage64P43SlotRef* ref) {
  ref->seg = NULL;
  ref->slot = 0u;
}

#if !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static int hz5_lowpage64_p43_ref_valid(Hz5Lowpage64P43SlotRef ref) {
  return ref.seg != NULL;
}

static Hz5Lowpage64P43SlotRef hz5_lowpage64_p43_ref_next_locked(
    Hz5Lowpage64P43SlotRef ref) {
  Hz5Lowpage64P43SlotRef next = {0};
  if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return next;
  }
  return ref.seg->free_next[ref.slot];
}

static void hz5_lowpage64_p43_committed_push_locked(Hz5Lowpage64Segment* seg,
                                                    uint32_t slot) {
  if (!seg || slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return;
  }
  seg->free_next[slot] = g_hz5_lowpage64_p43_committed_head;
  g_hz5_lowpage64_p43_committed_head.seg = seg;
  g_hz5_lowpage64_p43_committed_head.slot = slot;
  g_hz5_lowpage64_p43_committed_count++;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_global_pushes, 1);
}

static int hz5_lowpage64_p43_committed_pop_locked(
    Hz5Lowpage64P43SlotRef* out) {
  while (hz5_lowpage64_p43_ref_valid(g_hz5_lowpage64_p43_committed_head)) {
    Hz5Lowpage64P43SlotRef ref = g_hz5_lowpage64_p43_committed_head;
    g_hz5_lowpage64_p43_committed_head =
        hz5_lowpage64_p43_ref_next_locked(ref);
    if (g_hz5_lowpage64_p43_committed_count > 0u) {
      g_hz5_lowpage64_p43_committed_count--;
    }

    uint32_t bit = (uint32_t)1u << ref.slot;
    if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
      continue;
    }
    if ((ref.seg->allocated_mask & bit) != 0u) {
      continue;
    }
    if ((ref.seg->committed_mask & bit) == 0u) {
      continue;
    }
    if ((ref.seg->cold_mask & bit) != 0u) {
      continue;
    }
    ref.seg->allocated_mask |= bit;
    hz5_lowpage64_p43_ref_clear(&ref.seg->free_next[ref.slot]);
    *out = ref;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_global_hits, 1);
    return 1;
  }
  hz5_lowpage64_p43_ref_clear(&g_hz5_lowpage64_p43_committed_head);
  g_hz5_lowpage64_p43_committed_count = 0u;
  return 0;
}
#endif
#endif

#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static int hz5_lowpage64_p43_tls_push(Hz5Lowpage64Segment* seg,
                                      uint32_t slot) {
  if (!seg || slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return 0;
  }
  if (g_hz5_lowpage64_p43_tls_cache.count >=
      (uint32_t)HZ5_LOWPAGE64_P43_TLS_CACHE_CAP) {
    return 0;
  }
  Hz5Lowpage64P43SlotRef* ref =
      &g_hz5_lowpage64_p43_tls_cache
           .refs[g_hz5_lowpage64_p43_tls_cache.count++];
  ref->seg = seg;
  ref->slot = slot;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_tls_pushes, 1);
  return 1;
}

static int hz5_lowpage64_p43_tls_pop_locked(Hz5Lowpage64P43SlotRef* out) {
  while (g_hz5_lowpage64_p43_tls_cache.count > 0u) {
    Hz5Lowpage64P43SlotRef ref =
        g_hz5_lowpage64_p43_tls_cache
            .refs[--g_hz5_lowpage64_p43_tls_cache.count];
    if (!ref.seg || ref.slot >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
      continue;
    }
    uint32_t bit = (uint32_t)1u << ref.slot;
    if ((ref.seg->allocated_mask & bit) != 0u) {
      continue;
    }
    if ((ref.seg->committed_mask & bit) == 0u) {
      continue;
    }
    if ((ref.seg->cold_mask & bit) != 0u) {
      continue;
    }
    ref.seg->allocated_mask |= bit;
    *out = ref;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_tls_hits, 1);
    return 1;
  }
  return 0;
}
#endif
#endif

int hz5_lowpage64_p43_may_own(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  return hz5_lowpage64_p43_lookup(ptr) != HZ5_LOWPAGE64_LOOKUP_MISS;
#else
  (void)ptr;
  return 0;
#endif
}

int hz5_lowpage64_p43_active_owns(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  return hz5_lowpage64_p43_lookup(ptr) ==
         HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
#else
  (void)ptr;
  return 0;
#endif
}

int hz5_lowpage64_p43_lookup(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  int result = HZ5_LOWPAGE64_LOOKUP_MISS;
  uintptr_t p = (uintptr_t)ptr;
  hz5_lowpage64_p43_lock_enter();
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    uintptr_t base = (uintptr_t)seg->base;
    uintptr_t end = base + HZ5_LOWPAGE64_P43_SEGMENT_SIZE;
    if (p < base || p >= end) {
      continue;
    }
    uint32_t slot =
        (uint32_t)((p - base) / HZ5_LOWPAGE64_P43_SLOT_SIZE);
    if (slot < (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
      uint32_t bit = (uint32_t)1u << slot;
      int active = ((seg->allocated_mask & bit) != 0u) &&
                   ((seg->committed_mask & bit) != 0u) &&
                   ((seg->cold_mask & bit) == 0u);
      result = active ? HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE
                      : HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE;
    }
    break;
  }
  hz5_lowpage64_p43_lock_leave();
  return result;
#else
  (void)ptr;
  return HZ5_LOWPAGE64_LOOKUP_MISS;
#endif
}

void* hz5_lowpage64_p43_alloc_slot(size_t raw_bytes) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS && defined(_WIN32)
  if (raw_bytes > HZ5_LOWPAGE64_P43_SLOT_SIZE) {
    return NULL;
  }

  hz5_lowpage64_p43_lock_enter();
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  {
    Hz5Lowpage64P43SlotRef ref = {0};
    if (hz5_lowpage64_p43_tls_pop_locked(&ref)) {
      void* raw = (void*)((uintptr_t)ref.seg->base +
                          (uintptr_t)ref.slot *
                              HZ5_LOWPAGE64_P43_SLOT_SIZE);
      hz5_lowpage64_p43_lock_leave();
      return raw;
    }
  }
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  {
    Hz5Lowpage64P43SlotRef ref = {0};
    if (hz5_lowpage64_p43_committed_pop_locked(&ref)) {
      void* raw = (void*)((uintptr_t)ref.seg->base +
                          (uintptr_t)ref.slot *
                              HZ5_LOWPAGE64_P43_SLOT_SIZE);
      hz5_lowpage64_p43_lock_leave();
      return raw;
    }
  }
#endif
#if HZ5_LOWPAGE64_P43_SLOT_DECOMMIT || HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    uint32_t cold_mask = seg->cold_mask & ~seg->allocated_mask;
    if (!cold_mask) {
      continue;
    }
    uint32_t slot =
#if HZ5_LOWPAGE64_P43_REWARM_BATCH > 1u
        hz5_lowpage64_p43_first_set_bit(cold_mask);
#else
        0;
    while (((cold_mask >> slot) & 1u) == 0u) {
      slot++;
    }
#endif
    void* raw = (void*)((uintptr_t)seg->base +
                        (uintptr_t)slot * HZ5_LOWPAGE64_P43_SLOT_SIZE);
    seg->allocated_mask |= ((uint32_t)1u << slot);
    hz5_lowpage64_p43_lock_leave();

#if HZ5_LOWPAGE64_P43_PAGE_NOACCESS
    DWORD old_protect = 0;
    if (!VirtualProtect(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, PAGE_READWRITE,
                        &old_protect)) {
      hz5_lowpage64_p43_lock_enter();
      seg->allocated_mask &= ~((uint32_t)1u << slot);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
      return NULL;
    }
#else
    if (!VirtualAlloc(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, MEM_COMMIT,
                      PAGE_READWRITE)) {
      hz5_lowpage64_p43_lock_enter();
      seg->allocated_mask &= ~((uint32_t)1u << slot);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
      return NULL;
    }
#endif
    hz5_lowpage64_p43_lock_enter();
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_cold_hits, 1);
    seg->cold_mask &= ~((uint32_t)1u << slot);
    seg->committed_mask |= ((uint32_t)1u << slot);
#if HZ5_LOWPAGE64_P43_REWARM_BATCH > 1u
    hz5_lowpage64_p43_rewarm_segment_neighbors(seg, slot);
#endif
    hz5_lowpage64_p43_lock_leave();
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
    return raw;
  }
#endif
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    uint32_t full_mask =
        ((uint32_t)1u << (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) - 1u;
    uint32_t free_mask = (~(seg->allocated_mask | seg->cold_mask)) & full_mask;
    if (!free_mask) {
      continue;
    }
    uint32_t slot = 0;
    while (((free_mask >> slot) & 1u) == 0u) {
      slot++;
    }
    void* raw = (void*)((uintptr_t)seg->base +
                        (uintptr_t)slot * HZ5_LOWPAGE64_P43_SLOT_SIZE);
    int already_committed =
        ((seg->committed_mask >> slot) & (uint32_t)1u) != 0u;
    seg->allocated_mask |= ((uint32_t)1u << slot);
    hz5_lowpage64_p43_lock_leave();

    if (!already_committed &&
        !VirtualAlloc(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, MEM_COMMIT,
                      PAGE_READWRITE)) {
      hz5_lowpage64_p43_lock_enter();
      seg->allocated_mask &= ~((uint32_t)1u << slot);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
      return NULL;
    }
    if (!already_committed) {
      hz5_lowpage64_p43_lock_enter();
      seg->committed_mask |= ((uint32_t)1u << slot);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
    }
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_free_slot_hits, 1);
    return raw;
  }
  hz5_lowpage64_p43_lock_leave();

  void* base = VirtualAlloc(NULL, HZ5_LOWPAGE64_P43_SEGMENT_SIZE,
                            MEM_RESERVE, PAGE_READWRITE);
  if (!base) {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_alloc_failures, 1);
    return NULL;
  }

  Hz5Lowpage64Segment* seg =
      (Hz5Lowpage64Segment*)malloc(sizeof(Hz5Lowpage64Segment));
  if (!seg) {
    VirtualFree(base, 0, MEM_RELEASE);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_alloc_failures, 1);
    return NULL;
  }

  void* raw = base;
  if (!VirtualAlloc(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, MEM_COMMIT,
                    PAGE_READWRITE)) {
    free(seg);
    VirtualFree(base, 0, MEM_RELEASE);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
    return NULL;
  }

  seg->base = base;
  seg->allocated_mask = 1u;
  seg->committed_mask = 1u;
  seg->cold_mask = 0u;
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
  for (uint32_t i = 0; i < (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT; i++) {
    hz5_lowpage64_p43_ref_clear(&seg->free_next[i]);
  }
#endif
  hz5_lowpage64_p43_lock_enter();
  seg->next = g_hz5_lowpage64_p43_segments;
  g_hz5_lowpage64_p43_segments = seg;
  hz5_lowpage64_p43_lock_leave();

  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_allocs, 1);
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_segments_reserved, 1);
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
  return raw;
#else
  (void)raw_bytes;
  return NULL;
#endif
}

int hz5_lowpage64_p43_release_slot(void* raw) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS && defined(_WIN32)
  uint32_t slot = 0;
  hz5_lowpage64_p43_lock_enter();
  Hz5Lowpage64Segment* seg =
      hz5_lowpage64_p43_find_segment_locked(raw, &slot);
  if (!seg || ((seg->allocated_mask >> slot) & 1u) == 0u) {
    hz5_lowpage64_p43_lock_leave();
    return 0;
  }
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  if ((seg->committed_mask & ((uint32_t)1u << slot)) != 0u &&
      (seg->cold_mask & ((uint32_t)1u << slot)) == 0u) {
    seg->allocated_mask &= ~((uint32_t)1u << slot);
    if (hz5_lowpage64_p43_tls_push(seg, slot)) {
      hz5_lowpage64_p43_lock_leave();
      return 1;
    }
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_tls_overflow_pushes, 1);
    hz5_lowpage64_p43_committed_push_locked(seg, slot);
    hz5_lowpage64_p43_lock_leave();
    return 1;
  }
#endif
#if HZ5_LOWPAGE64_P43_SLOT_DECOMMIT
#if HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP > 0u
  if (g_hz5_lowpage64_p43_committed_count <
          (size_t)HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP &&
      (seg->committed_mask & ((uint32_t)1u << slot)) != 0u &&
      (seg->cold_mask & ((uint32_t)1u << slot)) == 0u) {
    seg->allocated_mask &= ~((uint32_t)1u << slot);
    hz5_lowpage64_p43_committed_push_locked(seg, slot);
    hz5_lowpage64_p43_lock_leave();
    return 1;
  }
#endif
  seg->cold_mask |= ((uint32_t)1u << slot);
  int release_segment = 0;
#elif HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  seg->cold_mask |= ((uint32_t)1u << slot);
  int release_segment = 0;
#elif HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS
  seg->allocated_mask &= ~((uint32_t)1u << slot);
  if ((seg->committed_mask & ((uint32_t)1u << slot)) != 0u) {
    hz5_lowpage64_p43_committed_push_locked(seg, slot);
  }
  int release_segment = 0;
#else
  seg->allocated_mask &= ~((uint32_t)1u << slot);
  int release_segment = (seg->allocated_mask == 0u);
#endif
  if (release_segment) {
    hz5_lowpage64_p43_unlink_segment_locked(seg);
  }
  hz5_lowpage64_p43_lock_leave();

  if (release_segment) {
    VirtualFree(seg->base, 0, MEM_RELEASE);
    free(seg);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_releases, 1);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_segments_released, 1);
    return 1;
  }

#if HZ5_LOWPAGE64_P43_SLOT_DECOMMIT
  if (VirtualFree(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, MEM_DECOMMIT)) {
    hz5_lowpage64_p43_lock_enter();
    Hz5Lowpage64Segment* cold =
        hz5_lowpage64_p43_find_segment_locked(raw, &slot);
    if (cold) {
      cold->committed_mask &= ~((uint32_t)1u << slot);
      cold->allocated_mask &= ~((uint32_t)1u << slot);
    }
    hz5_lowpage64_p43_lock_leave();
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_decommits, 1);
  } else {
    hz5_lowpage64_p43_lock_enter();
    Hz5Lowpage64Segment* restore =
        hz5_lowpage64_p43_find_segment_locked(raw, &slot);
    if (restore) {
      restore->cold_mask &= ~((uint32_t)1u << slot);
      restore->allocated_mask |= ((uint32_t)1u << slot);
    }
    hz5_lowpage64_p43_lock_leave();
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_decommit_failures, 1);
  }
#elif HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  {
    DWORD old_protect = 0;
    if (VirtualProtect(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, PAGE_NOACCESS,
                       &old_protect)) {
      hz5_lowpage64_p43_lock_enter();
      Hz5Lowpage64Segment* cold =
          hz5_lowpage64_p43_find_segment_locked(raw, &slot);
      if (cold) {
        cold->allocated_mask &= ~((uint32_t)1u << slot);
      }
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_decommits, 1);
    } else {
      hz5_lowpage64_p43_lock_enter();
      Hz5Lowpage64Segment* restore =
          hz5_lowpage64_p43_find_segment_locked(raw, &slot);
      if (restore) {
        restore->cold_mask &= ~((uint32_t)1u << slot);
      }
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_decommit_failures, 1);
    }
  }
#else
  (void)raw;
#endif
  return 1;
#else
  (void)raw;
  return 0;
#endif
}

void hz5_lowpage64_p43_stats_snapshot(
    Hz5Lowpage64P43StatsSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }
#if HZ5_LOWPAGE64_STATS
  snapshot->segments_reserved = atomic_load_explicit(
      &g_hz5_lowpage64_p43_segments_reserved, memory_order_relaxed);
  snapshot->segments_released = atomic_load_explicit(
      &g_hz5_lowpage64_p43_segments_released, memory_order_relaxed);
  snapshot->slot_commits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_commits, memory_order_relaxed);
  snapshot->slot_commit_failures = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_commit_failures, memory_order_relaxed);
  snapshot->slot_decommits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_decommits, memory_order_relaxed);
  snapshot->slot_decommit_failures = atomic_load_explicit(
      &g_hz5_lowpage64_p43_slot_decommit_failures, memory_order_relaxed);
  snapshot->tls_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_tls_hits, memory_order_relaxed);
  snapshot->tls_pushes = atomic_load_explicit(
      &g_hz5_lowpage64_p43_tls_pushes, memory_order_relaxed);
  snapshot->tls_overflow_pushes = atomic_load_explicit(
      &g_hz5_lowpage64_p43_tls_overflow_pushes, memory_order_relaxed);
  snapshot->global_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_global_hits, memory_order_relaxed);
  snapshot->global_pushes = atomic_load_explicit(
      &g_hz5_lowpage64_p43_global_pushes, memory_order_relaxed);
  snapshot->cold_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_cold_hits, memory_order_relaxed);
  snapshot->free_slot_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_free_slot_hits, memory_order_relaxed);
  snapshot->va_allocs = atomic_load_explicit(
      &g_hz5_lowpage64_p43_va_allocs, memory_order_relaxed);
  snapshot->va_alloc_failures = atomic_load_explicit(
      &g_hz5_lowpage64_p43_va_alloc_failures, memory_order_relaxed);
  snapshot->va_releases = atomic_load_explicit(
      &g_hz5_lowpage64_p43_va_releases, memory_order_relaxed);
#else
  snapshot->segments_reserved = 0;
  snapshot->segments_released = 0;
  snapshot->slot_commits = 0;
  snapshot->slot_commit_failures = 0;
  snapshot->slot_decommits = 0;
  snapshot->slot_decommit_failures = 0;
  snapshot->tls_hits = 0;
  snapshot->tls_pushes = 0;
  snapshot->tls_overflow_pushes = 0;
  snapshot->global_hits = 0;
  snapshot->global_pushes = 0;
  snapshot->cold_hits = 0;
  snapshot->free_slot_hits = 0;
  snapshot->va_allocs = 0;
  snapshot->va_alloc_failures = 0;
  snapshot->va_releases = 0;
#endif
}
