#include "hz11_thread_cache.h"

#include <errno.h>
#include <string.h>
#include <dlfcn.h>

/* ---------- state ---------- */

_Thread_local H11ThreadCache* hz11_tls = NULL;
_Thread_local int hz11_resolving = 0;

static void* (*hz11_real_malloc)(size_t) = NULL;
static void (*hz11_real_free)(void*) = NULL;
static void* (*hz11_real_realloc)(void*, size_t) = NULL;
static void* (*hz11_real_calloc)(size_t, size_t) = NULL;
static size_t (*hz11_real_usable_size)(void*) = NULL;
static int (*hz11_real_posix_memalign)(void**, size_t, size_t) = NULL;
static void* (*hz11_real_aligned_alloc)(size_t, size_t) = NULL;
static void* (*hz11_real_memalign)(size_t, size_t) = NULL;

static volatile int hz11_resolved = 0;

#define HZ11_BOOTSTRAP_BYTES 65536u
static char hz11_bootstrap_buf[HZ11_BOOTSTRAP_BYTES];
static volatile size_t hz11_bootstrap_off = 0;

static void* hz11_bootstrap_alloc(size_t n) {
  n = (n + 15u) & ~(size_t)15u;
  size_t off = __atomic_fetch_add(&hz11_bootstrap_off, n, __ATOMIC_RELAXED);
  if (off + n > (size_t)HZ11_BOOTSTRAP_BYTES) {
    return NULL;
  }
  return hz11_bootstrap_buf + off;
}

static int hz11_is_bootstrap(const void* p) {
  return (const char*)p >= hz11_bootstrap_buf &&
         (const char*)p < hz11_bootstrap_buf + HZ11_BOOTSTRAP_BYTES;
}

/* ---------- resolver ---------- */

void hz11_resolver_ensure(void) {
  if (hz11_resolved) {
    return;
  }
  if (hz11_resolving) {
    return;
  }
  hz11_resolving = 1;
  hz11_real_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
  hz11_real_free = (void (*)(void*))dlsym(RTLD_NEXT, "free");
  hz11_real_realloc = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
  hz11_real_calloc = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  hz11_real_usable_size = (size_t (*)(void*))dlsym(RTLD_NEXT, "malloc_usable_size");
  hz11_real_posix_memalign =
      (int (*)(void**, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
  hz11_real_aligned_alloc =
      (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "aligned_alloc");
  hz11_real_memalign = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");
  hz11_resolving = 0;
  hz11_resolved = 1;
}

/* ---------- system allocator wrappers ---------- */

void* hz11_sys_malloc(size_t n) {
  hz11_resolver_ensure();
  if (hz11_resolving) {
    return hz11_bootstrap_alloc(n);
  }
  if (hz11_real_malloc) {
    return hz11_real_malloc(n);
  }
  return hz11_bootstrap_alloc(n);
}

void hz11_sys_free(void* p) {
  if (!p) {
    return;
  }
  if (hz11_is_bootstrap(p)) {
    return;
  }
  hz11_resolver_ensure();
  if (hz11_real_free) {
    hz11_real_free(p);
  }
}

void* hz11_sys_realloc(void* p, size_t n) {
  if (hz11_is_bootstrap(p)) {
    void* np = hz11_sys_malloc(n);
    if (np && p) {
      memcpy(np, p, n);
    }
    return np;
  }
  hz11_resolver_ensure();
  if (hz11_real_realloc) {
    return hz11_real_realloc(p, n);
  }
  return NULL;
}

void* hz11_sys_calloc(size_t count, size_t size) {
  hz11_resolver_ensure();
  if (hz11_real_calloc) {
    return hz11_real_calloc(count, size);
  }
  size_t total = count * size;
  if (count != 0u && size > SIZE_MAX / count) {
    return NULL;
  }
  void* p = hz11_sys_malloc(total);
  if (p) {
    memset(p, 0, total);
  }
  return p;
}

size_t hz11_sys_usable_size(void* p) {
  hz11_resolver_ensure();
  if (hz11_real_usable_size) {
    return hz11_real_usable_size(p);
  }
  return 0;
}

int hz11_sys_posix_memalign(void** memptr, size_t alignment, size_t size) {
  hz11_resolver_ensure();
  if (hz11_real_posix_memalign) {
    return hz11_real_posix_memalign(memptr, alignment, size);
  }
  return ENOMEM;
}

void* hz11_sys_aligned_alloc(size_t alignment, size_t size) {
  hz11_resolver_ensure();
  if (hz11_real_aligned_alloc) {
    return hz11_real_aligned_alloc(alignment, size);
  }
  return NULL;
}

void* hz11_sys_memalign(size_t alignment, size_t size) {
  hz11_resolver_ensure();
  if (hz11_real_memalign) {
    return hz11_real_memalign(alignment, size);
  }
  return NULL;
}

/* ---------- TLS cache init + slow paths ---------- */

H11ThreadCache* hz11_thread_cache_init_slow(void) {
  void* raw = hz11_sys_malloc(sizeof(H11ThreadCache));
  if (!raw) {
    return NULL;
  }
  H11ThreadCache* tc = (H11ThreadCache*)raw;
  memset(tc, 0, sizeof(*tc));
#if HZ11_CLASSIFY_SPAN
  hz11_span_init(); /* ensure the arena is mapped before any span carve */
#endif
  hz11_tls = tc;
#if HZ11_CACHE_TOPPTR
  for (uint32_t c = 0u; c < HZ11_CLASS_COUNT; ++c) {
    tc->class_cache[c].top = tc->class_cache[c].items;
  }
#endif
  return tc;
}

static void hz11_thread_cache_flush_class(H11ThreadCache* tc, uint8_t class_id) {
#if HZ11_CACHE_SOA
  uint32_t n = tc->class_counts[class_id];
  HZ11_COUNT_ADD(tc->flush_items, n);
  for (uint32_t i = 0u; i < n; ++i) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(tc->class_items[class_id][i])) {
      hz11_returned_push(class_id, tc->class_items[class_id][i]);
    } else {
      hz11_sys_free(tc->class_items[class_id][i]);
    }
#else
    hz11_sys_free(tc->class_items[class_id][i]);
#endif
    tc->class_items[class_id][i] = NULL;
  }
  tc->class_counts[class_id] = 0u;
#else
  H11ClassCache* cc = &tc->class_cache[class_id];
  size_t slot = hz11_class_slot_size(class_id);
#if HZ11_CACHE_TOPPTR
  void** p = cc->items;
  uint32_t n = (uint32_t)(cc->top - cc->items);
  HZ11_COUNT_ADD(tc->flush_items, n);
  while (p < cc->top) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(*p)) {
      hz11_returned_push(class_id, *p);
    } else {
      hz11_sys_free(*p);
    }
#else
    hz11_sys_free(*p);
#endif
    ++p;
  }
  cc->top = cc->items;
  tc->cached_bytes -= slot * n;
#else
  HZ11_COUNT_ADD(tc->flush_items, cc->count);
  for (uint32_t i = 0; i < cc->count; ++i) {
#if HZ11_CLASSIFY_SPAN
    if (hz11_arena_contains(cc->items[i])) {
      hz11_returned_push(class_id, cc->items[i]);
    } else {
      hz11_sys_free(cc->items[i]);
    }
#else
    hz11_sys_free(cc->items[i]);
#endif
    cc->items[i] = NULL;
  }
  tc->cached_bytes -= slot * cc->count;
  cc->count = 0;
#endif
#endif /* HZ11_CACHE_SOA */
  HZ11_COUNT_INC(tc->flush_count);
}

void hz11_thread_cache_push_overflow_slow(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  HZ11_COUNT_INC(tc->overflow_count);
  hz11_thread_cache_flush_class(tc, class_id);
  if (class_id < HZ11_CLASS_COUNT) {
#if HZ11_CACHE_SOA
    tc->class_items[class_id][0] = ptr;
    tc->class_counts[class_id] = 1u;
#else
    H11ClassCache* cc = &tc->class_cache[class_id];
#if HZ11_CACHE_TOPPTR
    *cc->top++ = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
#else
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
#endif
#endif /* HZ11_CACHE_SOA */
  } else {
    hz11_sys_free(ptr);
  }
}

void* hz11_thread_cache_refill(H11ThreadCache* tc, uint8_t class_id) {
#if !HZ11_CLASSIFY_SPAN && !HZ11_ENABLE_HOT_COUNTERS
  (void)tc; /* token lane: tc only used for the now-compiled-out refill_count */
#endif
  HZ11_COUNT_INC(tc->refill_count);
#if HZ11_CLASSIFY_SPAN
  /* 1. per-class returned-object sink first (reuse before carving a fresh span) */
  void* reused = hz11_returned_pop(class_id);
  if (reused) {
    return reused;
  }
  /* 2. bump from the per-thread current span */
  size_t slot = hz11_class_slot_size(class_id);
  H11SpanCurrent* cs = &tc->current[class_id];
  if (cs->base && cs->bump_index < cs->slot_count) {
    return cs->base + (size_t)cs->bump_index++ * slot;
  }
  /* 3. current span exhausted (or none yet): carve a fresh 64 KiB span */
  char* base = (char*)hz11_span_carve_for_class(class_id);
  if (!base) {
    return hz11_sys_malloc(slot); /* arena full -- fall back (bench never hits) */
  }
  cs->base = base;
  cs->bump_index = 0;
  cs->slot_count = (uint32_t)(HZ11_SPAN_BYTES / slot);
  return cs->base + (size_t)cs->bump_index++ * slot;
#else
  return hz11_sys_malloc(hz11_class_slot_size(class_id));
#endif
}
