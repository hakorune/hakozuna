#include "hz5.h"

#include "hz5_internal.h"
#include "hz5_largefront.h"
#include "hz5_midfront.h"
#include "hz5_ownerhub.h"
#include "hz5_smallfront.h"
#include "hz5_wrapper.h"

#include <dlfcn.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Experimental full LD_PRELOAD front-end.
 *
 * Unlike hz5_preload_hybrid.c, this does not use a size gate. Normal malloc,
 * calloc, realloc, posix_memalign, and aligned_alloc calls are sent to HZ5.
 * The only real-libc path is the reentrant/bootstrap path needed while the
 * interposer itself resolves symbols or while HZ5 is already executing.
 */

#ifndef HZ5_PRELOAD_FULL_TRACK_BITS
#define HZ5_PRELOAD_FULL_TRACK_BITS 22u
#endif

#define HZ5_PRELOAD_FULL_TRACK_CAP \
  ((size_t)1u << HZ5_PRELOAD_FULL_TRACK_BITS)
#define HZ5_PRELOAD_FULL_TOMBSTONE ((void*)(uintptr_t)1u)

typedef void* (*Hz5RealMallocFn)(size_t);
typedef void (*Hz5RealFreeFn)(void*);
typedef int (*Hz5RealPosixMemalignFn)(void**, size_t, size_t);
typedef void* (*Hz5RealCallocFn)(size_t, size_t);
typedef void* (*Hz5RealReallocFn)(void*, size_t);
typedef void* (*Hz5RealAlignedAllocFn)(size_t, size_t);
typedef size_t (*Hz5RealMallocUsableSizeFn)(void*);
typedef int (*Hz5RealLibcStartMainFn)(int (*main)(int, char**, char**),
                                      int,
                                      char**,
                                      void (*init)(void),
                                      void (*fini)(void),
                                      void (*rtld_fini)(void),
                                      void*);
typedef int (*Hz5MainFn)(int, char**, char**);

enum {
  HZ5_PRELOAD_FULL_OWNER_EMPTY = 0,
  HZ5_PRELOAD_FULL_OWNER_HZ5 = 1,
  HZ5_PRELOAD_FULL_OWNER_REAL = 2,
  HZ5_PRELOAD_FULL_OWNER_TOMBSTONE = 255
};

typedef struct Hz5PreloadFullEntry {
  void* ptr;
  uint8_t owner;
} Hz5PreloadFullEntry;

static Hz5PreloadFullEntry
    g_hz5_preload_full_table[HZ5_PRELOAD_FULL_TRACK_CAP];
static pthread_mutex_t g_hz5_preload_full_lock = PTHREAD_MUTEX_INITIALIZER;
static __thread int g_hz5_preload_full_inside;
static _Atomic int g_hz5_preload_full_ready;
static int g_hz5_preload_full_stats_enabled;
static atomic_flag g_hz5_preload_full_stats_registered = ATOMIC_FLAG_INIT;
static unsigned char g_hz5_preload_full_bootstrap[1024u * 1024u];
static _Atomic size_t g_hz5_preload_full_bootstrap_used;

static _Atomic uint64_t g_hz5_preload_full_malloc_hz5;
static _Atomic uint64_t g_hz5_preload_full_malloc_real;
static _Atomic uint64_t g_hz5_preload_full_malloc_fail;
static _Atomic uint64_t g_hz5_preload_full_posix_hz5;
static _Atomic uint64_t g_hz5_preload_full_posix_real;
static _Atomic uint64_t g_hz5_preload_full_posix_fail;
static _Atomic uint64_t g_hz5_preload_full_aligned_hz5;
static _Atomic uint64_t g_hz5_preload_full_aligned_real;
static _Atomic uint64_t g_hz5_preload_full_aligned_fail;
static _Atomic uint64_t g_hz5_preload_full_calloc_hz5;
static _Atomic uint64_t g_hz5_preload_full_calloc_real;
static _Atomic uint64_t g_hz5_preload_full_realloc_hz5;
static _Atomic uint64_t g_hz5_preload_full_realloc_real;
static _Atomic uint64_t g_hz5_preload_full_free_hz5;
static _Atomic uint64_t g_hz5_preload_full_free_real;
static _Atomic uint64_t g_hz5_preload_full_free_unknown_real;
static _Atomic uint64_t g_hz5_preload_full_track_insert_fail;

static Hz5RealMallocFn g_real_malloc;
static Hz5RealFreeFn g_real_free;
static Hz5RealPosixMemalignFn g_real_posix_memalign;
static Hz5RealCallocFn g_real_calloc;
static Hz5RealReallocFn g_real_realloc;
static Hz5RealAlignedAllocFn g_real_aligned_alloc;
static Hz5RealMallocUsableSizeFn g_real_malloc_usable_size;
static Hz5RealLibcStartMainFn g_real_libc_start_main;
static Hz5MainFn g_hz5_preload_full_main;

__attribute__((constructor)) static void hz5_preload_full_constructor(void) {
  atomic_store_explicit(&g_hz5_preload_full_ready, 0, memory_order_release);
  g_hz5_preload_full_stats_enabled = getenv("HZ5_PRELOAD_STATS") != NULL;
}

static inline void hz5_preload_full_stat_inc(_Atomic uint64_t* counter) {
  if (g_hz5_preload_full_stats_enabled) {
    atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
  }
}

static int hz5_preload_full_main_thunk(int argc, char** argv, char** envp) {
  g_hz5_preload_full_stats_enabled = getenv("HZ5_PRELOAD_STATS") != NULL;
  atomic_store_explicit(&g_hz5_preload_full_ready, 1, memory_order_release);
  return g_hz5_preload_full_main
             ? g_hz5_preload_full_main(argc, argv, envp)
             : 127;
}

int __libc_start_main(int (*main)(int, char**, char**),
                      int argc,
                      char** ubp_av,
                      void (*init)(void),
                      void (*fini)(void),
                      void (*rtld_fini)(void),
                      void* stack_end) {
  g_hz5_preload_full_inside++;
  g_real_libc_start_main =
      (Hz5RealLibcStartMainFn)dlsym(RTLD_NEXT, "__libc_start_main");
  g_hz5_preload_full_inside--;
  if (!g_real_libc_start_main) {
    return 127;
  }
  g_hz5_preload_full_main = main;
  return g_real_libc_start_main(hz5_preload_full_main_thunk, argc, ubp_av,
                                init, fini, rtld_fini, stack_end);
}

static void* hz5_preload_full_bootstrap_alloc(size_t size) {
  size_t align = sizeof(void*) - 1u;
  size_t need = (size + align) & ~align;
  if (need == 0) {
    need = sizeof(void*);
  }
  size_t off = atomic_fetch_add_explicit(
      &g_hz5_preload_full_bootstrap_used, need, memory_order_relaxed);
  if (off > sizeof(g_hz5_preload_full_bootstrap) ||
      need > sizeof(g_hz5_preload_full_bootstrap) - off) {
    return NULL;
  }
  return &g_hz5_preload_full_bootstrap[off];
}

static int hz5_preload_full_is_bootstrap_ptr(void* ptr) {
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)&g_hz5_preload_full_bootstrap[0];
  uintptr_t end = base + sizeof(g_hz5_preload_full_bootstrap);
  return p >= base && p < end;
}

static void hz5_preload_full_stats_print(void) {
  if (!g_hz5_preload_full_stats_enabled) {
    return;
  }
  fprintf(stderr,
          "[HZ5_PRELOAD_FULL_STATS]"
          " malloc_hz5=%llu malloc_real=%llu malloc_fail=%llu"
          " posix_hz5=%llu posix_real=%llu posix_fail=%llu"
          " aligned_hz5=%llu aligned_real=%llu aligned_fail=%llu"
          " calloc_hz5=%llu calloc_real=%llu"
          " realloc_hz5=%llu realloc_real=%llu"
          " free_hz5=%llu free_real=%llu free_unknown_real=%llu"
          " track_insert_fail=%llu\n",
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_malloc_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_malloc_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_malloc_fail, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_posix_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_posix_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_posix_fail, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_aligned_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_aligned_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_aligned_fail, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_calloc_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_calloc_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_realloc_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_realloc_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_free_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_free_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_free_unknown_real, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_full_track_insert_fail, memory_order_relaxed));
}

static void hz5_preload_full_stats_register_once(void) {
  if (!g_hz5_preload_full_stats_enabled) {
    return;
  }
  if (atomic_flag_test_and_set_explicit(
          &g_hz5_preload_full_stats_registered, memory_order_acq_rel)) {
    return;
  }
}

__attribute__((destructor)) static void hz5_preload_full_stats_destructor(
    void) {
  hz5_preload_full_stats_print();
  hz5_ownerhub_stats_print();
}

static void hz5_preload_full_resolve(void) {
  hz5_preload_full_stats_register_once();
  if (g_hz5_preload_full_inside) {
    return;
  }
  g_hz5_preload_full_inside++;
  if (!g_real_malloc) {
    g_real_malloc = (Hz5RealMallocFn)dlsym(RTLD_NEXT, "malloc");
  }
  if (!g_real_free) {
    g_real_free = (Hz5RealFreeFn)dlsym(RTLD_NEXT, "free");
  }
  if (!g_real_posix_memalign) {
    g_real_posix_memalign =
        (Hz5RealPosixMemalignFn)dlsym(RTLD_NEXT, "posix_memalign");
  }
  if (!g_real_calloc) {
    g_real_calloc = (Hz5RealCallocFn)dlsym(RTLD_NEXT, "calloc");
  }
  if (!g_real_realloc) {
    g_real_realloc = (Hz5RealReallocFn)dlsym(RTLD_NEXT, "realloc");
  }
  if (!g_real_aligned_alloc) {
    g_real_aligned_alloc =
        (Hz5RealAlignedAllocFn)dlsym(RTLD_NEXT, "aligned_alloc");
  }
  if (!g_real_malloc_usable_size) {
    g_real_malloc_usable_size =
        (Hz5RealMallocUsableSizeFn)dlsym(RTLD_NEXT, "malloc_usable_size");
  }
  g_hz5_preload_full_inside--;
}

static size_t hz5_preload_full_hash(void* ptr) {
  uintptr_t x = (uintptr_t)ptr >> 4;
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  return (size_t)x & (HZ5_PRELOAD_FULL_TRACK_CAP - 1u);
}

static int hz5_preload_full_track_insert(void* ptr, uint8_t owner) {
  if (!ptr || ptr == HZ5_PRELOAD_FULL_TOMBSTONE) {
    return 0;
  }
  size_t idx = hz5_preload_full_hash(ptr);
  pthread_mutex_lock(&g_hz5_preload_full_lock);
  for (size_t probe = 0; probe < HZ5_PRELOAD_FULL_TRACK_CAP; ++probe) {
    Hz5PreloadFullEntry* entry =
        &g_hz5_preload_full_table[(idx + probe) &
                                  (HZ5_PRELOAD_FULL_TRACK_CAP - 1u)];
    if (!entry->ptr || entry->ptr == HZ5_PRELOAD_FULL_TOMBSTONE ||
        entry->ptr == ptr) {
      entry->ptr = ptr;
      entry->owner = owner;
      pthread_mutex_unlock(&g_hz5_preload_full_lock);
      return 1;
    }
  }
  pthread_mutex_unlock(&g_hz5_preload_full_lock);
  return 0;
}

static uint8_t hz5_preload_full_track_lookup(void* ptr) {
  if (!ptr || ptr == HZ5_PRELOAD_FULL_TOMBSTONE) {
    return HZ5_PRELOAD_FULL_OWNER_EMPTY;
  }
  size_t idx = hz5_preload_full_hash(ptr);
  pthread_mutex_lock(&g_hz5_preload_full_lock);
  for (size_t probe = 0; probe < HZ5_PRELOAD_FULL_TRACK_CAP; ++probe) {
    Hz5PreloadFullEntry* entry =
        &g_hz5_preload_full_table[(idx + probe) &
                                  (HZ5_PRELOAD_FULL_TRACK_CAP - 1u)];
    if (!entry->ptr) {
      pthread_mutex_unlock(&g_hz5_preload_full_lock);
      return HZ5_PRELOAD_FULL_OWNER_EMPTY;
    }
    if (entry->ptr == ptr) {
      uint8_t owner = entry->owner;
      pthread_mutex_unlock(&g_hz5_preload_full_lock);
      return owner;
    }
  }
  pthread_mutex_unlock(&g_hz5_preload_full_lock);
  return HZ5_PRELOAD_FULL_OWNER_EMPTY;
}

static uint8_t hz5_preload_full_track_remove(void* ptr) {
  if (!ptr || ptr == HZ5_PRELOAD_FULL_TOMBSTONE) {
    return HZ5_PRELOAD_FULL_OWNER_EMPTY;
  }
  size_t idx = hz5_preload_full_hash(ptr);
  pthread_mutex_lock(&g_hz5_preload_full_lock);
  for (size_t probe = 0; probe < HZ5_PRELOAD_FULL_TRACK_CAP; ++probe) {
    Hz5PreloadFullEntry* entry =
        &g_hz5_preload_full_table[(idx + probe) &
                                  (HZ5_PRELOAD_FULL_TRACK_CAP - 1u)];
    if (!entry->ptr) {
      pthread_mutex_unlock(&g_hz5_preload_full_lock);
      return HZ5_PRELOAD_FULL_OWNER_EMPTY;
    }
    if (entry->ptr == ptr) {
      uint8_t owner = entry->owner;
      entry->ptr = HZ5_PRELOAD_FULL_TOMBSTONE;
      entry->owner = HZ5_PRELOAD_FULL_OWNER_TOMBSTONE;
      pthread_mutex_unlock(&g_hz5_preload_full_lock);
      return owner;
    }
  }
  pthread_mutex_unlock(&g_hz5_preload_full_lock);
  return HZ5_PRELOAD_FULL_OWNER_EMPTY;
}

static void* hz5_preload_full_real_malloc(size_t size) {
  void* ptr = g_real_malloc ? g_real_malloc(size)
                            : hz5_preload_full_bootstrap_alloc(size);
  if (ptr) {
    if (!hz5_preload_full_is_bootstrap_ptr(ptr) &&
        !hz5_preload_full_track_insert(ptr, HZ5_PRELOAD_FULL_OWNER_REAL)) {
      hz5_preload_full_stat_inc(&g_hz5_preload_full_track_insert_fail);
    }
    hz5_preload_full_stat_inc(&g_hz5_preload_full_malloc_real);
  }
  return ptr;
}

static void* hz5_preload_full_hz5_malloc(size_t size, size_t align) {
  if (hz5_smallfront_can_handle(size, align)) {
    return hz5_smallfront_alloc(size, align);
  }
  if (hz5_midfront_can_handle(size, align)) {
    return hz5_midfront_alloc(size, align);
  }
  if (hz5_largefront_can_handle(size, align)) {
    return hz5_largefront_alloc(size, align);
  }

  g_hz5_preload_full_inside++;
  void* ptr = hz5_aligned_alloc(size, align);
  g_hz5_preload_full_inside--;
  if (!ptr) {
    return NULL;
  }
  if (!hz5_preload_full_track_insert(ptr, HZ5_PRELOAD_FULL_OWNER_HZ5)) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_track_insert_fail);
    g_hz5_preload_full_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_full_inside--;
    return NULL;
  }
  return ptr;
}

static size_t hz5_preload_full_hz5_usable_size(void* ptr) {
  size_t small = hz5_smallfront_usable_size(ptr);
  if (small != 0) {
    return small;
  }
  size_t mid = hz5_midfront_usable_size(ptr);
  if (mid != 0) {
    return mid;
  }
  size_t large = hz5_largefront_usable_size(ptr);
  if (large != 0) {
    return large;
  }

  Hz5WrapperHdr* wrapped = NULL;
  if (hz5_wrapper_decode(ptr, &wrapped)) {
    return wrapped->requested;
  }

  Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
  if (!seg) {
    return 0;
  }
  uint32_t page = hz5_p1_page_index(seg, ptr);
  if (page >= HZ5_SEG_PAGES) {
    return 0;
  }
  Hz5PageMeta* meta = &seg->page[page];
  if (meta->kind != HZ5_PAGE_RUN_START || meta->run_pages == 0) {
    return 0;
  }
  return (size_t)meta->run_pages * HZ5_PAGE_SIZE;
}

void* malloc(size_t size) {
  if (!atomic_load_explicit(&g_hz5_preload_full_ready,
                            memory_order_acquire)) {
    return hz5_preload_full_bootstrap_alloc(size);
  }
  if (g_hz5_preload_full_inside) {
    hz5_preload_full_resolve();
    return hz5_preload_full_real_malloc(size);
  }
  hz5_preload_full_stats_register_once();

  void* ptr = hz5_preload_full_hz5_malloc(size, 16u);
  if (ptr) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_malloc_hz5);
    return ptr;
  }

  hz5_preload_full_stat_inc(&g_hz5_preload_full_malloc_fail);
  errno = ENOMEM;
  return NULL;
}

void free(void* ptr) {
  if (!ptr) {
    return;
  }
  if (hz5_preload_full_is_bootstrap_ptr(ptr)) {
    return;
  }

  Hz5SmallFrontFreeResult small_free = hz5_smallfront_free(ptr);
  if (small_free == HZ5_SMALLFRONT_FREE_OK ||
      small_free == HZ5_SMALLFRONT_FREE_INVALID) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_free_hz5);
    return;
  }
  Hz5MidFrontFreeResult mid_free = hz5_midfront_free(ptr);
  if (mid_free == HZ5_MIDFRONT_FREE_OK ||
      mid_free == HZ5_MIDFRONT_FREE_INVALID) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_free_hz5);
    return;
  }
  Hz5LargeFrontFreeResult large_free = hz5_largefront_free(ptr);
  if (large_free == HZ5_LARGEFRONT_FREE_OK ||
      large_free == HZ5_LARGEFRONT_FREE_INVALID) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_free_hz5);
    return;
  }

  uint8_t owner = hz5_preload_full_track_remove(ptr);
  if (owner == HZ5_PRELOAD_FULL_OWNER_HZ5) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_free_hz5);
    g_hz5_preload_full_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_full_inside--;
    return;
  }
  if (owner == HZ5_PRELOAD_FULL_OWNER_REAL) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_free_real);
    if (g_real_free) {
      g_real_free(ptr);
    }
    return;
  }

  hz5_preload_full_stat_inc(&g_hz5_preload_full_free_unknown_real);
  hz5_preload_full_resolve();
  if (g_real_free) {
    g_real_free(ptr);
  }
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  if (!atomic_load_explicit(&g_hz5_preload_full_ready,
                            memory_order_acquire)) {
    void* boot = hz5_preload_full_bootstrap_alloc(size + alignment);
    if (!boot) {
      return ENOMEM;
    }
    uintptr_t raw = (uintptr_t)boot;
    *memptr = (void*)((raw + alignment - 1u) & ~(uintptr_t)(alignment - 1u));
    return 0;
  }
  if (g_hz5_preload_full_inside) {
    hz5_preload_full_resolve();
    int rc = ENOMEM;
    if (g_real_posix_memalign) {
      rc = g_real_posix_memalign(memptr, alignment, size);
    } else {
      void* boot = hz5_preload_full_bootstrap_alloc(size + alignment);
      if (boot) {
        uintptr_t raw = (uintptr_t)boot;
        *memptr = (void*)((raw + alignment - 1u) &
                          ~(uintptr_t)(alignment - 1u));
        rc = 0;
      }
    }
    if (rc == 0 && *memptr) {
      if (!hz5_preload_full_is_bootstrap_ptr(*memptr)) {
        hz5_preload_full_track_insert(*memptr, HZ5_PRELOAD_FULL_OWNER_REAL);
      }
      hz5_preload_full_stat_inc(&g_hz5_preload_full_posix_real);
    }
    return rc;
  }

  void* ptr = hz5_preload_full_hz5_malloc(size, alignment);
  if (ptr) {
    *memptr = ptr;
    hz5_preload_full_stat_inc(&g_hz5_preload_full_posix_hz5);
    return 0;
  }
  hz5_preload_full_stat_inc(&g_hz5_preload_full_posix_fail);
  return ENOMEM;
}

void* aligned_alloc(size_t alignment, size_t size) {
  if (!atomic_load_explicit(&g_hz5_preload_full_ready,
                            memory_order_acquire)) {
    void* boot = hz5_preload_full_bootstrap_alloc(size + alignment);
    if (!boot) {
      return NULL;
    }
    uintptr_t raw = (uintptr_t)boot;
    return (void*)((raw + alignment - 1u) & ~(uintptr_t)(alignment - 1u));
  }
  if (g_hz5_preload_full_inside) {
    hz5_preload_full_resolve();
    void* ptr = g_real_aligned_alloc ? g_real_aligned_alloc(alignment, size)
                                     : hz5_preload_full_bootstrap_alloc(
                                           size + alignment);
    if (ptr) {
      if (!hz5_preload_full_is_bootstrap_ptr(ptr)) {
        hz5_preload_full_track_insert(ptr, HZ5_PRELOAD_FULL_OWNER_REAL);
      }
      hz5_preload_full_stat_inc(&g_hz5_preload_full_aligned_real);
    }
    return ptr;
  }

  void* ptr = hz5_preload_full_hz5_malloc(size, alignment);
  if (ptr) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_aligned_hz5);
    return ptr;
  }
  hz5_preload_full_stat_inc(&g_hz5_preload_full_aligned_fail);
  errno = ENOMEM;
  return NULL;
}

void* calloc(size_t nmemb, size_t size) {
  if (nmemb != 0 && size > SIZE_MAX / nmemb) {
    errno = ENOMEM;
    return NULL;
  }
  size_t total = nmemb * size;
  if (!atomic_load_explicit(&g_hz5_preload_full_ready,
                            memory_order_acquire)) {
    void* boot = hz5_preload_full_bootstrap_alloc(total);
    if (boot) {
      memset(boot, 0, total);
    }
    return boot;
  }
  if (g_hz5_preload_full_inside) {
    hz5_preload_full_resolve();
    void* ptr = g_real_calloc ? g_real_calloc(nmemb, size)
                              : hz5_preload_full_bootstrap_alloc(total);
    if (ptr) {
      if (!g_real_calloc) {
        memset(ptr, 0, total);
      }
      if (!hz5_preload_full_is_bootstrap_ptr(ptr)) {
        hz5_preload_full_track_insert(ptr, HZ5_PRELOAD_FULL_OWNER_REAL);
      }
      hz5_preload_full_stat_inc(&g_hz5_preload_full_calloc_real);
    }
    return ptr;
  }

  void* ptr = hz5_preload_full_hz5_malloc(total, 16u);
  if (!ptr) {
    hz5_preload_full_stat_inc(&g_hz5_preload_full_malloc_fail);
    return NULL;
  }
  memset(ptr, 0, total);
  hz5_preload_full_stat_inc(&g_hz5_preload_full_calloc_hz5);
  return ptr;
}

void* realloc(void* ptr, size_t size) {
  if (!atomic_load_explicit(&g_hz5_preload_full_ready,
                            memory_order_acquire)) {
    return NULL;
  }
  if (!ptr) {
    return hz5_preload_full_hz5_malloc(size, 16u);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  if (hz5_preload_full_is_bootstrap_ptr(ptr)) {
    return malloc(size);
  }

  size_t small_old_size = hz5_smallfront_usable_size(ptr);
  if (small_old_size != 0) {
    void* next = hz5_preload_full_hz5_malloc(size, 16u);
    if (!next) {
      return NULL;
    }
    memcpy(next, ptr, small_old_size < size ? small_old_size : size);
    free(ptr);
    hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_hz5);
    return next;
  }
  size_t mid_old_size = hz5_midfront_usable_size(ptr);
  if (mid_old_size != 0) {
    if (hz5_midfront_can_handle(size, 16u) &&
        hz5_midfront_usable_size(ptr) >= size) {
      hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_hz5);
      return ptr;
    }
    void* next = hz5_preload_full_hz5_malloc(size, 16u);
    if (!next) {
      return NULL;
    }
    memcpy(next, ptr, mid_old_size < size ? mid_old_size : size);
    free(ptr);
    hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_hz5);
    return next;
  }
  size_t large_old_size = hz5_largefront_usable_size(ptr);
  if (large_old_size != 0) {
    if (hz5_largefront_can_handle(size, 16u) &&
        hz5_largefront_usable_size(ptr) >= size) {
      hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_hz5);
      return ptr;
    }
    void* next = hz5_preload_full_hz5_malloc(size, 16u);
    if (!next) {
      return NULL;
    }
    memcpy(next, ptr, large_old_size < size ? large_old_size : size);
    free(ptr);
    hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_hz5);
    return next;
  }

  uint8_t owner = hz5_preload_full_track_lookup(ptr);
  if (owner == HZ5_PRELOAD_FULL_OWNER_REAL) {
    hz5_preload_full_resolve();
    uint8_t removed = hz5_preload_full_track_remove(ptr);
    (void)removed;
    void* next = g_real_realloc ? g_real_realloc(ptr, size) : NULL;
    if (next) {
      hz5_preload_full_track_insert(next, HZ5_PRELOAD_FULL_OWNER_REAL);
      hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_real);
    }
    return next;
  }

  if (owner == HZ5_PRELOAD_FULL_OWNER_HZ5) {
    size_t old_size = hz5_preload_full_hz5_usable_size(ptr);
    void* next = hz5_preload_full_hz5_malloc(size, 16u);
    if (!next) {
      return NULL;
    }
    if (old_size != 0) {
      memcpy(next, ptr, old_size < size ? old_size : size);
    }
    free(ptr);
    hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_hz5);
    return next;
  }

  hz5_preload_full_stat_inc(&g_hz5_preload_full_realloc_real);
  hz5_preload_full_resolve();
  return g_real_realloc ? g_real_realloc(ptr, size) : NULL;
}

size_t malloc_usable_size(void* ptr) {
  if (!ptr) {
    return 0;
  }
  size_t small = hz5_smallfront_usable_size(ptr);
  if (small != 0) {
    return small;
  }
  size_t mid = hz5_midfront_usable_size(ptr);
  if (mid != 0) {
    return mid;
  }
  size_t large = hz5_largefront_usable_size(ptr);
  if (large != 0) {
    return large;
  }
  uint8_t owner = hz5_preload_full_track_lookup(ptr);
  if (owner == HZ5_PRELOAD_FULL_OWNER_HZ5) {
    return hz5_preload_full_hz5_usable_size(ptr);
  }
  hz5_preload_full_resolve();
  return g_real_malloc_usable_size ? g_real_malloc_usable_size(ptr) : 0;
}
