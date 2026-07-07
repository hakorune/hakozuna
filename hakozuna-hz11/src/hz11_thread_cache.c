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
  hz11_tls = tc;
  return tc;
}

static void hz11_thread_cache_flush_class(H11ThreadCache* tc, uint8_t class_id) {
  H11ClassCache* cc = &tc->class_cache[class_id];
  size_t slot = hz11_class_slot_size(class_id);
  tc->flush_items += cc->count;
  for (uint32_t i = 0; i < cc->count; ++i) {
    hz11_sys_free(cc->items[i]);
    cc->items[i] = NULL;
  }
  tc->cached_bytes -= slot * cc->count;
  cc->count = 0;
  tc->flush_count += 1u;
}

void hz11_thread_cache_push_overflow_slow(H11ThreadCache* tc, uint8_t class_id,
                                          void* ptr) {
  tc->overflow_count += 1u;
  hz11_thread_cache_flush_class(tc, class_id);
  if (class_id < HZ11_CLASS_COUNT) {
    H11ClassCache* cc = &tc->class_cache[class_id];
    cc->items[cc->count++] = ptr;
    tc->cached_bytes += hz11_class_slot_size(class_id);
  } else {
    hz11_sys_free(ptr);
  }
}

void* hz11_thread_cache_refill(H11ThreadCache* tc, uint8_t class_id) {
  tc->refill_count += 1u;
  return hz11_sys_malloc(hz11_class_slot_size(class_id));
}
