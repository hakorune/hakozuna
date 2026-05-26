#include "hz5_lowpage64.h"
#include "hz5_lowpage64_p43_segment.h"
#include "hz5_trace.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <sys/mman.h>
#endif

/*
 * HZ5 P43 segment-slot source layer.
 *
 * P43i/P45 balanced lanes keep the P25 bridge as the speed layer and use this
 * module only as the segment-slot source beneath it. Direct descriptor release,
 * slot decommit, PAGE_NOACCESS, and runtime segment release are evidence or
 * future RSS-return contracts, not part of the current lockless prepared-bridge
 * speed contract.
 */

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

#if HZ5_LOWPAGE64_STATS && HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#define HZ5_P43_COUNT_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
static void hz5_p43_count_max(_Atomic size_t* counter, size_t value) {
  size_t current = atomic_load_explicit(counter, memory_order_relaxed);
  while (current < value &&
         !atomic_compare_exchange_weak_explicit(
             counter, &current, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}
#define HZ5_P43_COUNT_MAX(counter, value) \
  hz5_p43_count_max(&(counter), (value))
#else
#define HZ5_P43_COUNT_ADD(counter, value) ((void)0)
#define HZ5_P43_COUNT_MAX(counter, value) ((void)0)
#endif

#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#if HZ5_LOWPAGE64_P43_SLOT_COUNT > 32u
#error "P43 segment slot bitmap currently supports up to 32 slots"
#endif

#define HZ5_LOWPAGE64_P43_COMMITTED_LISTS \
  (HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS || \
   HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP > 0u || \
   HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u || \
   HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u)

typedef struct Hz5Lowpage64Segment Hz5Lowpage64Segment;

typedef struct Hz5Lowpage64P43SlotRef {
  Hz5Lowpage64Segment* seg;
  uint32_t slot;
} Hz5Lowpage64P43SlotRef;

struct Hz5Lowpage64Segment {
  void* base;
  _Atomic uint32_t allocated_mask;
  _Atomic uint32_t committed_mask;
  _Atomic uint32_t cold_mask;
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u
  _Atomic uint32_t pending_free_mask;
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
  Hz5Lowpage64P43SlotRef free_next[HZ5_LOWPAGE64_P43_SLOT_COUNT];
#endif
  Hz5Lowpage64Segment* next;
};

#if defined(_WIN32)
static INIT_ONCE g_hz5_lowpage64_p43_lock_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION g_hz5_lowpage64_p43_lock;
#else
static pthread_mutex_t g_hz5_lowpage64_p43_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
static Hz5Lowpage64Segment* g_hz5_lowpage64_p43_segments;
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
static _Atomic(Hz5Lowpage64Segment*)
    g_hz5_lowpage64_p43_fast_lookup[HZ5_LOWPAGE64_P43_FAST_LOOKUP_CAP];
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static Hz5Lowpage64P43SlotRef g_hz5_lowpage64_p43_committed_head;
static size_t g_hz5_lowpage64_p43_committed_count;
#endif
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
typedef struct Hz5Lowpage64P43TlsCache {
  Hz5Lowpage64P43SlotRef refs[HZ5_LOWPAGE64_P43_TLS_CACHE_CAP];
  uint32_t count;
} Hz5Lowpage64P43TlsCache;

#if defined(_WIN32)
__declspec(thread) static Hz5Lowpage64P43TlsCache
    g_hz5_lowpage64_p43_tls_cache;
#else
static __thread Hz5Lowpage64P43TlsCache g_hz5_lowpage64_p43_tls_cache;
#endif
#endif
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
typedef struct Hz5Lowpage64P43ReleaseBuffer {
  Hz5Lowpage64P43SlotRef refs[HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP];
  uint32_t count;
} Hz5Lowpage64P43ReleaseBuffer;

#if defined(_WIN32)
__declspec(thread) static Hz5Lowpage64P43ReleaseBuffer
    g_hz5_lowpage64_p43_release_buffer;
#else
static __thread Hz5Lowpage64P43ReleaseBuffer
    g_hz5_lowpage64_p43_release_buffer;
#endif
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
static _Atomic size_t g_hz5_lowpage64_p43_source_alloc_calls;
static _Atomic size_t g_hz5_lowpage64_p43_source_cold_segment_scans_total;
static _Atomic size_t g_hz5_lowpage64_p43_source_cold_segment_scans_max;
static _Atomic size_t g_hz5_lowpage64_p43_source_free_segment_scans_total;
static _Atomic size_t g_hz5_lowpage64_p43_source_free_segment_scans_max;
static _Atomic size_t g_hz5_lowpage64_p43_source_new_segment_scan_total;
static _Atomic size_t g_hz5_lowpage64_p43_source_new_segment_scan_max;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_calls;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_active;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_nonactive;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_miss;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_segments_scanned_total;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_segments_scanned_max;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_fast_hits;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_fast_misses;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_lockless_hits;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_lockless_misses;
static _Atomic size_t g_hz5_lowpage64_p43_find_fast_hits;
static _Atomic size_t g_hz5_lowpage64_p43_find_segments_scanned_total;
static _Atomic size_t g_hz5_lowpage64_p43_find_segments_scanned_max;
static _Atomic size_t g_hz5_lowpage64_p43_va_allocs;
static _Atomic size_t g_hz5_lowpage64_p43_va_alloc_failures;
static _Atomic size_t g_hz5_lowpage64_p43_va_releases;
#endif

#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#if defined(_WIN32)
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
#else
static void hz5_lowpage64_p43_lock_enter(void) {
  pthread_mutex_lock(&g_hz5_lowpage64_p43_lock);
}

static void hz5_lowpage64_p43_lock_leave(void) {
  pthread_mutex_unlock(&g_hz5_lowpage64_p43_lock);
}
#endif

static void* hz5_p43_os_reserve(size_t bytes) {
#if defined(_WIN32)
  return VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_READWRITE);
#else
  void* p = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return p == MAP_FAILED ? NULL : p;
#endif
}

static int hz5_p43_os_commit(void* addr, size_t bytes) {
#if defined(_WIN32)
  return VirtualAlloc(addr, bytes, MEM_COMMIT, PAGE_READWRITE) != NULL;
#else
  (void)bytes;
  return addr != NULL;
#endif
}

static int hz5_p43_os_decommit(void* addr, size_t bytes) {
#if defined(_WIN32)
  return VirtualFree(addr, bytes, MEM_DECOMMIT) != 0;
#else
  (void)addr;
  (void)bytes;
  return 0;
#endif
}

static void hz5_p43_os_release(void* addr, size_t bytes) {
#if defined(_WIN32)
  (void)bytes;
  if (addr) {
    VirtualFree(addr, 0, MEM_RELEASE);
  }
#else
  if (addr && bytes) {
    munmap(addr, bytes);
  }
#endif
}

#include "hz5_lowpage64_p43_segment_helpers.inc"

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

static int hz5_lowpage64_p43_lookup_impl(void* ptr,
                                         Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  int result = HZ5_LOWPAGE64_LOOKUP_MISS;
  uintptr_t p = (uintptr_t)ptr;
  size_t scanned = 0;
  Hz5Lowpage64Segment* found_seg = NULL;
  uint32_t found_slot = 0;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_calls, 1);
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP && HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP
  result = hz5_lowpage64_p43_lookup_fast_lockless(p, &scanned, &found_seg,
                                                  &found_slot);
  if (result != HZ5_LOWPAGE64_LOOKUP_MISS) {
    goto hz5_p43_lookup_count_result;
  }
#endif
  hz5_lowpage64_p43_lock_enter();
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
  {
    uint32_t slot = 0;
    Hz5Lowpage64Segment* hint =
        atomic_load_explicit(
            &g_hz5_lowpage64_p43_fast_lookup[
                hz5_lowpage64_p43_fast_lookup_index(p)],
            memory_order_acquire);
    if (hz5_lowpage64_p43_segment_contains(hint, p, &slot)) {
      result = hz5_lowpage64_p43_slot_lookup_result(hint, slot);
      found_seg = hint;
      found_slot = slot;
      scanned = 1;
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_hits, 1);
      goto hz5_p43_lookup_done;
    }
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_misses, 1);
  }
#endif
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    scanned++;
    uint32_t slot = 0;
    if (!hz5_lowpage64_p43_segment_contains(seg, p, &slot)) {
      continue;
    }
    result = hz5_lowpage64_p43_slot_lookup_result(seg, slot);
    found_seg = seg;
    found_slot = slot;
    break;
  }
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
hz5_p43_lookup_done:
#endif
  hz5_lowpage64_p43_lock_leave();
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP && HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP
hz5_p43_lookup_count_result:
#endif
  hz5_lowpage64_p43_fill_free_ctx(ctx, result, found_seg, found_slot);
  hz5_lowpage64_p43_count_lookup_result(result, scanned);
  return result;
#else
  (void)ptr;
  if (ctx) {
    ctx->lookup_kind = HZ5_LOWPAGE64_LOOKUP_MISS;
  }
  return HZ5_LOWPAGE64_LOOKUP_MISS;
#endif
}

static int hz5_lowpage64_p43_lookup_raw_fast_impl(
    void* raw,
    Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS && HZ5_LOWPAGE64_P43_FAST_LOOKUP
  int result = HZ5_LOWPAGE64_LOOKUP_MISS;
  uintptr_t p = (uintptr_t)raw;
  size_t scanned = 0;
  Hz5Lowpage64Segment* found_seg = NULL;
  uint32_t found_slot = 0;
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_calls, 1);
  Hz5Lowpage64Segment* hint =
      atomic_load_explicit(
          &g_hz5_lowpage64_p43_fast_lookup[
              hz5_lowpage64_p43_fast_lookup_index(p)],
          memory_order_acquire);
  uint32_t slot = 0;
  if (hz5_lowpage64_p43_segment_contains(hint, p, &slot) &&
      hz5_lowpage64_p43_slot_base(hint, slot) == raw) {
#if HZ5_LOWPAGE64_P43_RAW_ALLOCATED_LOOKUP_ONLY
    uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
    result = (hz5_lowpage64_p43_mask_load(&hint->allocated_mask) & bit)
                 ? HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE
                 : HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE;
#else
    result = hz5_lowpage64_p43_slot_lookup_result(hint, slot);
#endif
    found_seg = hint;
    found_slot = slot;
    scanned = 1;
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_hits, 1);
  } else {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_lookup_fast_misses, 1);
  }
  hz5_lowpage64_p43_fill_free_ctx(ctx, result, found_seg, found_slot);
  hz5_lowpage64_p43_count_lookup_result(result, scanned);
  return result;
#else
  return hz5_lowpage64_p43_lookup_impl(raw, ctx);
#endif
}

int hz5_lowpage64_p43_lookup(void* ptr) {
  return hz5_lowpage64_p43_lookup_impl(ptr, NULL);
}

int hz5_lowpage64_p43_prepare_free_user(void* ptr,
                                        Hz5Lowpage64FreeCtx* ctx) {
  return hz5_lowpage64_p43_lookup_impl(ptr, ctx);
}

int hz5_lowpage64_p43_prepare_free_raw(void* raw,
                                       Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_RAW_FAST_LOOKUP_ONLY
  return hz5_lowpage64_p43_lookup_raw_fast_impl(raw, ctx);
#else
  return hz5_lowpage64_p43_lookup_impl(raw, ctx);
#endif
}

int hz5_lowpage64_p43_release_token(void* segment_token,
                                    uint32_t slot_index,
                                    void* raw) {
  Hz5Lowpage64FreeCtx ctx;
  ctx.lookup_kind = HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE;
  ctx.segment_token = segment_token;
  ctx.slot_base = raw;
  ctx.slot_size = HZ5_LOWPAGE64_P43_SLOT_SIZE;
  ctx.slot_index = slot_index;
  ctx.flags = 0;
  return hz5_lowpage64_p43_release_prepared_slot(&ctx);
}

#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
static void hz5_lowpage64_p43_fill_alloc_ctx(Hz5Lowpage64FreeCtx* ctx,
                                             Hz5Lowpage64Segment* seg,
                                             uint32_t slot) {
  hz5_lowpage64_p43_fill_free_ctx(ctx, HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE, seg,
                                  slot);
}
#endif

static void* hz5_lowpage64_p43_alloc_slot_impl(size_t raw_bytes,
                                               Hz5Lowpage64FreeCtx* alloc_ctx) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  if (raw_bytes > HZ5_LOWPAGE64_P43_SLOT_SIZE) {
    return NULL;
  }
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_source_alloc_calls, 1);

  hz5_lowpage64_p43_lock_enter();
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  {
    Hz5Lowpage64P43SlotRef ref = {0};
	    if (hz5_lowpage64_p43_tls_pop_locked(&ref)) {
	      void* raw = hz5_lowpage64_p43_slot_base(ref.seg, ref.slot);
	      hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, ref.seg, ref.slot);
	      hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_TLS);
	      hz5_lowpage64_p43_lock_leave();
	      return raw;
	    }
  }
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  {
    Hz5Lowpage64P43SlotRef ref = {0};
	    if (hz5_lowpage64_p43_committed_pop_locked(&ref)) {
	      void* raw = hz5_lowpage64_p43_slot_base(ref.seg, ref.slot);
	      hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, ref.seg, ref.slot);
	      hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_COMMITTED);
	      hz5_lowpage64_p43_lock_leave();
	      return raw;
	    }
  }
#endif
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  if (g_hz5_lowpage64_p43_release_buffer.count > 0u) {
    hz5_lowpage64_p43_release_buffer_flush_locked();
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u
    {
      Hz5Lowpage64P43SlotRef ref = {0};
	      if (hz5_lowpage64_p43_tls_pop_locked(&ref)) {
	        void* raw = hz5_lowpage64_p43_slot_base(ref.seg, ref.slot);
	        hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, ref.seg, ref.slot);
	        hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_RELEASE_BUFFER);
	        hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_TLS);
	        hz5_lowpage64_p43_lock_leave();
	        return raw;
	      }
    }
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
    {
      Hz5Lowpage64P43SlotRef ref = {0};
	      if (hz5_lowpage64_p43_committed_pop_locked(&ref)) {
	        void* raw = hz5_lowpage64_p43_slot_base(ref.seg, ref.slot);
	        hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, ref.seg, ref.slot);
	        hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_RELEASE_BUFFER);
	        hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_COMMITTED);
	        hz5_lowpage64_p43_lock_leave();
	        return raw;
	      }
    }
#endif
  }
#endif
#if HZ5_LOWPAGE64_P43_SLOT_DECOMMIT || HZ5_LOWPAGE64_P43_PAGE_NOACCESS
#if HZ5_LOWPAGE64_STATS
  size_t cold_segments_scanned = 0;
#endif
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
#if HZ5_LOWPAGE64_STATS
    cold_segments_scanned++;
#endif
    uint32_t cold_mask =
        hz5_lowpage64_p43_mask_load(&seg->cold_mask) &
        ~hz5_lowpage64_p43_mask_load(&seg->allocated_mask);
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
    void* raw = hz5_lowpage64_p43_slot_base(seg, slot);
    uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
    hz5_lowpage64_p43_mask_or(&seg->allocated_mask, bit);
    hz5_lowpage64_p43_lock_leave();

#if HZ5_LOWPAGE64_P43_PAGE_NOACCESS
    DWORD old_protect = 0;
    if (!VirtualProtect(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE, PAGE_READWRITE,
                        &old_protect)) {
      hz5_lowpage64_p43_lock_enter();
      hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
      return NULL;
    }
#else
    if (!hz5_p43_os_commit(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE)) {
      hz5_lowpage64_p43_lock_enter();
      hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
      return NULL;
    }
#endif
    hz5_lowpage64_p43_lock_enter();
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_cold_hits, 1);
#if HZ5_LOWPAGE64_STATS
    HZ5_P43_COUNT_ADD(
        g_hz5_lowpage64_p43_source_cold_segment_scans_total,
        cold_segments_scanned);
    HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_source_cold_segment_scans_max,
                      cold_segments_scanned);
#endif
    hz5_lowpage64_p43_mask_and(&seg->cold_mask, ~bit);
    hz5_lowpage64_p43_mask_or(&seg->committed_mask, bit);
#if HZ5_LOWPAGE64_P43_REWARM_BATCH > 1u
    hz5_lowpage64_p43_rewarm_segment_neighbors(seg, slot);
#endif
	    hz5_lowpage64_p43_lock_leave();
	    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
	    hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, seg, slot);
	    hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_COLD);
	    return raw;
	  }
#if HZ5_LOWPAGE64_STATS
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_source_cold_segment_scans_total,
                    cold_segments_scanned);
  HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_source_cold_segment_scans_max,
                    cold_segments_scanned);
#endif
#endif
#if HZ5_LOWPAGE64_STATS
  size_t free_segments_scanned = 0;
#endif
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
#if HZ5_LOWPAGE64_STATS
    free_segments_scanned++;
#endif
    uint32_t full_mask = hz5_lowpage64_p43_slot_full_mask();
    uint32_t free_mask =
        (~(hz5_lowpage64_p43_mask_load(&seg->allocated_mask) |
           hz5_lowpage64_p43_mask_load(&seg->cold_mask))) &
        full_mask;
    if (!free_mask) {
      continue;
    }
    uint32_t slot = 0;
    while (((free_mask >> slot) & 1u) == 0u) {
      slot++;
    }
    void* raw = hz5_lowpage64_p43_slot_base(seg, slot);
    uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
    int already_committed =
        ((hz5_lowpage64_p43_mask_load(&seg->committed_mask) >> slot) &
         (uint32_t)1u) != 0u;
    hz5_lowpage64_p43_mask_or(&seg->allocated_mask, bit);
    hz5_lowpage64_p43_lock_leave();

    if (!already_committed &&
        !hz5_p43_os_commit(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE)) {
      hz5_lowpage64_p43_lock_enter();
      hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
      return NULL;
    }
    if (!already_committed) {
      hz5_lowpage64_p43_lock_enter();
      hz5_lowpage64_p43_mask_or(&seg->committed_mask, bit);
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
    }
	    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_free_slot_hits, 1);
#if HZ5_LOWPAGE64_STATS
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_source_free_segment_scans_total,
                      free_segments_scanned);
    HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_source_free_segment_scans_max,
                      free_segments_scanned);
#endif
	    hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, seg, slot);
	    hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_FREE_SLOT);
	    return raw;
	  }
  hz5_lowpage64_p43_lock_leave();
#if HZ5_LOWPAGE64_STATS
  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_source_new_segment_scan_total,
                    free_segments_scanned);
  HZ5_P43_COUNT_MAX(g_hz5_lowpage64_p43_source_new_segment_scan_max,
                    free_segments_scanned);
#endif

  void* base = hz5_p43_os_reserve(HZ5_LOWPAGE64_P43_SEGMENT_SIZE);
  if (!base) {
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_alloc_failures, 1);
    return NULL;
  }

  Hz5Lowpage64Segment* seg =
      (Hz5Lowpage64Segment*)malloc(sizeof(Hz5Lowpage64Segment));
  if (!seg) {
    hz5_p43_os_release(base, HZ5_LOWPAGE64_P43_SEGMENT_SIZE);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_alloc_failures, 1);
    return NULL;
  }

  void* raw = base;
  if (!hz5_p43_os_commit(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE)) {
    free(seg);
    hz5_p43_os_release(base, HZ5_LOWPAGE64_P43_SEGMENT_SIZE);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commit_failures, 1);
    return NULL;
  }

  seg->base = base;
  hz5_lowpage64_p43_mask_store(&seg->allocated_mask, 1u);
  hz5_lowpage64_p43_mask_store(&seg->committed_mask, 1u);
  hz5_lowpage64_p43_mask_store(&seg->cold_mask, 0u);
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u
  hz5_lowpage64_p43_mask_store(&seg->pending_free_mask, 0u);
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
  for (uint32_t i = 0; i < (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT; i++) {
    hz5_lowpage64_p43_ref_clear(&seg->free_next[i]);
  }
#endif
  hz5_lowpage64_p43_lock_enter();
  seg->next = g_hz5_lowpage64_p43_segments;
  g_hz5_lowpage64_p43_segments = seg;
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
  hz5_lowpage64_p43_publish_fast_lookup_locked(seg);
#endif
  hz5_lowpage64_p43_lock_leave();

	  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_allocs, 1);
	  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_segments_reserved, 1);
	  HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_commits, 1);
	  hz5_lowpage64_p43_fill_alloc_ctx(alloc_ctx, seg, 0);
	  hz5_trace_inc(HZ5_TRACE_ALLOC_P43_SOURCE_NEW_SEGMENT);
	  return raw;
#else
	  (void)raw_bytes;
	  (void)alloc_ctx;
	  return NULL;
#endif
}

void* hz5_lowpage64_p43_alloc_slot(size_t raw_bytes) {
  return hz5_lowpage64_p43_alloc_slot_impl(raw_bytes, NULL);
}

void* hz5_lowpage64_p43_alloc_slot_prepared(size_t raw_bytes,
                                            Hz5Lowpage64FreeCtx* ctx) {
  return hz5_lowpage64_p43_alloc_slot_impl(raw_bytes, ctx);
}

int hz5_lowpage64_p43_release_slot(void* raw) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  uint32_t slot = 0;
  hz5_lowpage64_p43_lock_enter();
  Hz5Lowpage64Segment* seg =
      hz5_lowpage64_p43_find_segment_locked(raw, &slot);
  if (!seg ||
      ((hz5_lowpage64_p43_mask_load(&seg->allocated_mask) >> slot) & 1u) ==
          0u) {
    hz5_lowpage64_p43_lock_leave();
    return 0;
  }
  uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  if ((hz5_lowpage64_p43_mask_load(&seg->committed_mask) & bit) != 0u &&
      (hz5_lowpage64_p43_mask_load(&seg->cold_mask) & bit) == 0u) {
    hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
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
      (hz5_lowpage64_p43_mask_load(&seg->committed_mask) & bit) != 0u &&
      (hz5_lowpage64_p43_mask_load(&seg->cold_mask) & bit) == 0u) {
    hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
    hz5_lowpage64_p43_committed_push_locked(seg, slot);
    hz5_lowpage64_p43_lock_leave();
    return 1;
  }
#endif
  hz5_lowpage64_p43_mask_or(&seg->cold_mask, bit);
  int release_segment = 0;
#elif HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  hz5_lowpage64_p43_mask_or(&seg->cold_mask, bit);
  int release_segment = 0;
#elif HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS
  hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
  if ((hz5_lowpage64_p43_mask_load(&seg->committed_mask) & bit) != 0u) {
    hz5_lowpage64_p43_committed_push_locked(seg, slot);
  }
  int release_segment = 0;
#else
  hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
#if HZ5_LOWPAGE64_P43_LOCKLESS_CONTRACT
  int release_segment = 0;
#else
  int release_segment =
      (hz5_lowpage64_p43_mask_load(&seg->allocated_mask) == 0u);
#endif
#endif
  if (release_segment) {
    hz5_lowpage64_p43_unlink_segment_locked(seg);
  }
  hz5_lowpage64_p43_lock_leave();

  if (release_segment) {
    hz5_p43_os_release(seg->base, HZ5_LOWPAGE64_P43_SEGMENT_SIZE);
    free(seg);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_va_releases, 1);
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_segments_released, 1);
    return 1;
  }

#if HZ5_LOWPAGE64_P43_SLOT_DECOMMIT
  if (hz5_p43_os_decommit(raw, HZ5_LOWPAGE64_P43_SLOT_SIZE)) {
    hz5_lowpage64_p43_lock_enter();
    Hz5Lowpage64Segment* cold =
        hz5_lowpage64_p43_find_segment_locked(raw, &slot);
    if (cold) {
      hz5_lowpage64_p43_mask_and(&cold->committed_mask, ~bit);
      hz5_lowpage64_p43_mask_and(&cold->allocated_mask, ~bit);
    }
    hz5_lowpage64_p43_lock_leave();
    HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_decommits, 1);
  } else {
    hz5_lowpage64_p43_lock_enter();
    Hz5Lowpage64Segment* restore =
        hz5_lowpage64_p43_find_segment_locked(raw, &slot);
    if (restore) {
      hz5_lowpage64_p43_mask_and(&restore->cold_mask, ~bit);
      hz5_lowpage64_p43_mask_or(&restore->allocated_mask, bit);
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
        hz5_lowpage64_p43_mask_and(&cold->allocated_mask, ~bit);
      }
      hz5_lowpage64_p43_lock_leave();
      HZ5_P43_COUNT_ADD(g_hz5_lowpage64_p43_slot_decommits, 1);
    } else {
      hz5_lowpage64_p43_lock_enter();
      Hz5Lowpage64Segment* restore =
          hz5_lowpage64_p43_find_segment_locked(raw, &slot);
      if (restore) {
        hz5_lowpage64_p43_mask_and(&restore->cold_mask, ~bit);
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

int hz5_lowpage64_p43_release_prepared_slot(
    const Hz5Lowpage64FreeCtx* ctx) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  if (!ctx || ctx->lookup_kind != HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE ||
      !ctx->segment_token ||
      ctx->slot_index >= (uint32_t)HZ5_LOWPAGE64_P43_SLOT_COUNT) {
    return 0;
  }

  Hz5Lowpage64Segment* seg =
      (Hz5Lowpage64Segment*)ctx->segment_token;
  uint32_t slot = ctx->slot_index;
  uint32_t bit = hz5_lowpage64_p43_slot_bit(slot);
  void* expected = hz5_lowpage64_p43_slot_base(seg, slot);
  if (ctx->slot_base && ctx->slot_base != expected) {
    return 0;
  }

  hz5_lowpage64_p43_lock_enter();
  if ((hz5_lowpage64_p43_mask_load(&seg->allocated_mask) & bit) == 0u) {
    hz5_lowpage64_p43_lock_leave();
    return 0;
  }
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  if ((hz5_lowpage64_p43_mask_load(&seg->pending_free_mask) & bit) != 0u) {
    hz5_lowpage64_p43_lock_leave();
    return 0;
  }
  if ((hz5_lowpage64_p43_mask_load(&seg->committed_mask) & bit) != 0u &&
      (hz5_lowpage64_p43_mask_load(&seg->cold_mask) & bit) == 0u) {
    hz5_lowpage64_p43_mask_or(&seg->pending_free_mask, bit);
    if (hz5_lowpage64_p43_release_buffer_push_locked(seg, slot)) {
      hz5_lowpage64_p43_lock_leave();
      return 1;
    }
    hz5_lowpage64_p43_mask_and(&seg->pending_free_mask, ~bit);
  }
#endif
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  if ((hz5_lowpage64_p43_mask_load(&seg->committed_mask) & bit) != 0u &&
      (hz5_lowpage64_p43_mask_load(&seg->cold_mask) & bit) == 0u) {
    hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
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
  hz5_lowpage64_p43_mask_and(&seg->allocated_mask, ~bit);
  hz5_lowpage64_p43_lock_leave();
  return 1;
#else
  (void)ctx;
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
  snapshot->source_alloc_calls = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_alloc_calls, memory_order_relaxed);
  snapshot->source_cold_segment_scans_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_cold_segment_scans_total,
      memory_order_relaxed);
  snapshot->source_cold_segment_scans_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_cold_segment_scans_max,
      memory_order_relaxed);
  snapshot->source_free_segment_scans_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_free_segment_scans_total,
      memory_order_relaxed);
  snapshot->source_free_segment_scans_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_free_segment_scans_max,
      memory_order_relaxed);
  snapshot->source_new_segment_scan_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_new_segment_scan_total,
      memory_order_relaxed);
  snapshot->source_new_segment_scan_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_source_new_segment_scan_max,
      memory_order_relaxed);
  snapshot->lookup_calls = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_calls, memory_order_relaxed);
  snapshot->lookup_active = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_active, memory_order_relaxed);
  snapshot->lookup_nonactive = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_nonactive, memory_order_relaxed);
  snapshot->lookup_miss = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_miss, memory_order_relaxed);
  snapshot->lookup_segments_scanned_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_segments_scanned_total,
      memory_order_relaxed);
  snapshot->lookup_segments_scanned_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_segments_scanned_max,
      memory_order_relaxed);
  snapshot->lookup_fast_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_fast_hits, memory_order_relaxed);
  snapshot->lookup_fast_misses = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_fast_misses, memory_order_relaxed);
  snapshot->lookup_lockless_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_lockless_hits, memory_order_relaxed);
  snapshot->lookup_lockless_misses = atomic_load_explicit(
      &g_hz5_lowpage64_p43_lookup_lockless_misses, memory_order_relaxed);
  snapshot->find_fast_hits = atomic_load_explicit(
      &g_hz5_lowpage64_p43_find_fast_hits, memory_order_relaxed);
  snapshot->find_segments_scanned_total = atomic_load_explicit(
      &g_hz5_lowpage64_p43_find_segments_scanned_total,
      memory_order_relaxed);
  snapshot->find_segments_scanned_max = atomic_load_explicit(
      &g_hz5_lowpage64_p43_find_segments_scanned_max, memory_order_relaxed);
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
  snapshot->source_alloc_calls = 0;
  snapshot->source_cold_segment_scans_total = 0;
  snapshot->source_cold_segment_scans_max = 0;
  snapshot->source_free_segment_scans_total = 0;
  snapshot->source_free_segment_scans_max = 0;
  snapshot->source_new_segment_scan_total = 0;
  snapshot->source_new_segment_scan_max = 0;
  snapshot->lookup_calls = 0;
  snapshot->lookup_active = 0;
  snapshot->lookup_nonactive = 0;
  snapshot->lookup_miss = 0;
  snapshot->lookup_segments_scanned_total = 0;
  snapshot->lookup_segments_scanned_max = 0;
  snapshot->lookup_fast_hits = 0;
  snapshot->lookup_fast_misses = 0;
  snapshot->lookup_lockless_hits = 0;
  snapshot->lookup_lockless_misses = 0;
  snapshot->find_fast_hits = 0;
  snapshot->find_segments_scanned_total = 0;
  snapshot->find_segments_scanned_max = 0;
  snapshot->va_allocs = 0;
  snapshot->va_alloc_failures = 0;
  snapshot->va_releases = 0;
#endif
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  snapshot->contract_lockless_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_LOCKLESS_CONTRACT;
  snapshot->lockless_lookup_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP;
  snapshot->slot_decommit_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_SLOT_DECOMMIT;
  snapshot->page_noaccess_enabled =
      (uint32_t)HZ5_LOWPAGE64_P43_PAGE_NOACCESS;
  snapshot->runtime_segment_release_enabled = 0;
  hz5_lowpage64_p43_lock_enter();
  for (Hz5Lowpage64Segment* seg = g_hz5_lowpage64_p43_segments; seg;
       seg = seg->next) {
    uint32_t allocated = hz5_lowpage64_p43_mask_load(&seg->allocated_mask);
    uint32_t committed = hz5_lowpage64_p43_mask_load(&seg->committed_mask);
    uint32_t cold = hz5_lowpage64_p43_mask_load(&seg->cold_mask);
    uint32_t active = allocated & committed & ~cold;
    uint32_t committed_free = committed & ~allocated & ~cold;

    snapshot->segments_current++;
    snapshot->slots_total_current += HZ5_LOWPAGE64_P43_SLOT_COUNT;
    snapshot->slots_active_current +=
        hz5_lowpage64_p43_popcount32(active);
    snapshot->slots_committed_current +=
        hz5_lowpage64_p43_popcount32(committed);
    snapshot->slots_committed_free_current +=
        hz5_lowpage64_p43_popcount32(committed_free);
    snapshot->slots_cold_current += hz5_lowpage64_p43_popcount32(cold);
    if (HZ5_LOWPAGE64_P43_SLOT_COUNT >=
        hz5_lowpage64_p43_popcount32(committed)) {
      snapshot->slots_uncommitted_current +=
          HZ5_LOWPAGE64_P43_SLOT_COUNT -
          hz5_lowpage64_p43_popcount32(committed);
    }
  }
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  snapshot->slots_global_retained_current =
      g_hz5_lowpage64_p43_committed_count;
#endif
  hz5_lowpage64_p43_lock_leave();
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
  snapshot->slots_tls_retained_current =
      g_hz5_lowpage64_p43_tls_cache.count;
#endif
#else
  snapshot->segments_current = 0;
  snapshot->slots_total_current = 0;
  snapshot->slots_active_current = 0;
  snapshot->slots_committed_current = 0;
  snapshot->slots_committed_free_current = 0;
  snapshot->slots_global_retained_current = 0;
  snapshot->slots_tls_retained_current = 0;
  snapshot->slots_cold_current = 0;
  snapshot->slots_uncommitted_current = 0;
  snapshot->contract_lockless_enabled = 0;
  snapshot->lockless_lookup_enabled = 0;
  snapshot->slot_decommit_enabled = 0;
  snapshot->page_noaccess_enabled = 0;
  snapshot->runtime_segment_release_enabled = 0;
#endif
}

#endif  /* HZ5_LOWPAGE64_P43_SEGMENT_SLOTS */

#if !HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
void hz5_lowpage64_p43_stats_snapshot(
    Hz5Lowpage64P43StatsSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }
  *snapshot = (Hz5Lowpage64P43StatsSnapshot){0};
}
#endif
