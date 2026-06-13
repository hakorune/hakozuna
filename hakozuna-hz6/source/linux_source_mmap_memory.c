#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "linux_source_mmap.h"
#include "hz6_source_util.h"
#include "hz6_config.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include <unistd.h>

#if HZ6_LINUX_MMAP_RETAIN_L1
typedef struct Hz6LinuxMmapRetainedEntry {
  void* ptr;
  size_t bytes;
} Hz6LinuxMmapRetainedEntry;

static pthread_mutex_t g_hz6_linux_mmap_retain_mutex =
    PTHREAD_MUTEX_INITIALIZER;
static Hz6LinuxMmapRetainedEntry
    g_hz6_linux_mmap_retained[HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT];
static size_t g_hz6_linux_mmap_retained_bytes;

static atomic_size_t g_hz6_linux_mmap_retain_reserve_calls;
static atomic_size_t g_hz6_linux_mmap_retain_reserve_64k_calls;
static atomic_size_t g_hz6_linux_mmap_retain_64k_take_hit;
static atomic_size_t g_hz6_linux_mmap_retain_64k_take_miss;
static atomic_size_t g_hz6_linux_mmap_retain_generic_take_hit;
static atomic_size_t g_hz6_linux_mmap_retain_generic_take_miss;
static atomic_size_t g_hz6_linux_mmap_retain_mmap_fallback;
static atomic_size_t g_hz6_linux_mmap_retain_release_calls;
static atomic_size_t g_hz6_linux_mmap_retain_release_64k_calls;
static atomic_size_t g_hz6_linux_mmap_retain_64k_put_hit;
static atomic_size_t g_hz6_linux_mmap_retain_64k_put_full;
static atomic_size_t g_hz6_linux_mmap_retain_generic_put_hit;
static atomic_size_t g_hz6_linux_mmap_retain_generic_put_full;
static atomic_size_t g_hz6_linux_mmap_retain_munmap_fallback;

static void hz6_linux_mmap_retain_counter_inc(atomic_size_t* counter) {
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

#if HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
#define HZ6_LINUX_MMAP_RETAIN_64K_BYTES ((size_t)64u * 1024u)

static void* g_hz6_linux_mmap_retained_64k[HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT];
static size_t g_hz6_linux_mmap_retained_64k_count;
#endif

#if HZ6_LINUX_MMAP_RETAIN_TLS_L1
static __thread Hz6LinuxMmapRetainedEntry
    g_hz6_linux_mmap_tls_retained[HZ6_LINUX_MMAP_RETAIN_TLS_SLOT_COUNT];
static __thread size_t g_hz6_linux_mmap_tls_retained_bytes;

static size_t hz6_linux_mmap_retain_tls_index(size_t bytes) {
  return (bytes >> 12) % HZ6_LINUX_MMAP_RETAIN_TLS_SLOT_COUNT;
}

static void* hz6_linux_mmap_retain_tls_take(size_t bytes) {
  if (HZ6_LINUX_MMAP_RETAIN_TLS_SLOT_COUNT == 0) {
    return NULL;
  }
  Hz6LinuxMmapRetainedEntry* entry =
      &g_hz6_linux_mmap_tls_retained[hz6_linux_mmap_retain_tls_index(bytes)];
  if (!entry->ptr || entry->bytes != bytes) {
    return NULL;
  }
  void* ptr = entry->ptr;
  entry->ptr = NULL;
  entry->bytes = 0;
  if (g_hz6_linux_mmap_tls_retained_bytes >= bytes) {
    g_hz6_linux_mmap_tls_retained_bytes -= bytes;
  } else {
    g_hz6_linux_mmap_tls_retained_bytes = 0;
  }
  return ptr;
}
#endif

static void* hz6_linux_mmap_retain_take(size_t bytes) {
#if HZ6_LINUX_MMAP_RETAIN_TLS_L1
  void* tls_ptr = hz6_linux_mmap_retain_tls_take(bytes);
  if (tls_ptr) {
    return tls_ptr;
  }
#endif
  void* ptr = NULL;
  pthread_mutex_lock(&g_hz6_linux_mmap_retain_mutex);
#if HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
  if (bytes == HZ6_LINUX_MMAP_RETAIN_64K_BYTES &&
      g_hz6_linux_mmap_retained_64k_count != 0) {
    ptr = g_hz6_linux_mmap_retained_64k
        [--g_hz6_linux_mmap_retained_64k_count];
    if (g_hz6_linux_mmap_retained_bytes >= bytes) {
      g_hz6_linux_mmap_retained_bytes -= bytes;
    } else {
      g_hz6_linux_mmap_retained_bytes = 0;
    }
    pthread_mutex_unlock(&g_hz6_linux_mmap_retain_mutex);
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_64k_take_hit);
    return ptr;
  }
  if (bytes == HZ6_LINUX_MMAP_RETAIN_64K_BYTES) {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_64k_take_miss);
  }
#endif
  for (size_t i = HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT; i > 0; --i) {
    Hz6LinuxMmapRetainedEntry* entry = &g_hz6_linux_mmap_retained[i - 1u];
    if (entry->ptr && entry->bytes == bytes) {
      ptr = entry->ptr;
      entry->ptr = NULL;
      entry->bytes = 0;
      if (g_hz6_linux_mmap_retained_bytes >= bytes) {
        g_hz6_linux_mmap_retained_bytes -= bytes;
      } else {
        g_hz6_linux_mmap_retained_bytes = 0;
      }
      break;
    }
  }
  pthread_mutex_unlock(&g_hz6_linux_mmap_retain_mutex);
  if (ptr) {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_generic_take_hit);
  } else {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_generic_take_miss);
  }
  return ptr;
}

static int hz6_linux_mmap_retain_put_global(void* ptr, size_t bytes) {
  if (!ptr || bytes == 0 || HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT == 0 ||
      bytes > HZ6_LINUX_MMAP_RETAIN_BYTES_CAP) {
    return 0;
  }

  int retained = 0;
  pthread_mutex_lock(&g_hz6_linux_mmap_retain_mutex);
  if (g_hz6_linux_mmap_retained_bytes <=
      HZ6_LINUX_MMAP_RETAIN_BYTES_CAP - bytes) {
#if HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
    if (bytes == HZ6_LINUX_MMAP_RETAIN_64K_BYTES &&
        g_hz6_linux_mmap_retained_64k_count <
            HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT) {
      g_hz6_linux_mmap_retained_64k
          [g_hz6_linux_mmap_retained_64k_count++] = ptr;
      g_hz6_linux_mmap_retained_bytes += bytes;
      pthread_mutex_unlock(&g_hz6_linux_mmap_retain_mutex);
      hz6_linux_mmap_retain_counter_inc(
          &g_hz6_linux_mmap_retain_64k_put_hit);
      return 1;
    }
    if (bytes == HZ6_LINUX_MMAP_RETAIN_64K_BYTES) {
      hz6_linux_mmap_retain_counter_inc(
          &g_hz6_linux_mmap_retain_64k_put_full);
    }
#endif
    for (size_t i = 0; i < HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT; ++i) {
      Hz6LinuxMmapRetainedEntry* entry = &g_hz6_linux_mmap_retained[i];
      if (!entry->ptr) {
        entry->ptr = ptr;
        entry->bytes = bytes;
        g_hz6_linux_mmap_retained_bytes += bytes;
        retained = 1;
        break;
      }
    }
  }
  pthread_mutex_unlock(&g_hz6_linux_mmap_retain_mutex);
  if (retained) {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_generic_put_hit);
  } else {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_generic_put_full);
  }
  return retained;
}

static int hz6_linux_mmap_retain_put(void* ptr, size_t bytes) {
  if (!ptr || bytes == 0 || HZ6_LINUX_MMAP_RETAIN_SLOT_COUNT == 0 ||
      bytes > HZ6_LINUX_MMAP_RETAIN_BYTES_CAP) {
    return 0;
  }

#if HZ6_LINUX_MMAP_RETAIN_TLS_L1
  if (HZ6_LINUX_MMAP_RETAIN_TLS_SLOT_COUNT != 0 &&
      bytes <= HZ6_LINUX_MMAP_RETAIN_TLS_BYTES_CAP &&
      g_hz6_linux_mmap_tls_retained_bytes <=
          HZ6_LINUX_MMAP_RETAIN_TLS_BYTES_CAP - bytes) {
    Hz6LinuxMmapRetainedEntry* entry =
        &g_hz6_linux_mmap_tls_retained[hz6_linux_mmap_retain_tls_index(bytes)];
    if (!entry->ptr) {
      entry->ptr = ptr;
      entry->bytes = bytes;
      g_hz6_linux_mmap_tls_retained_bytes += bytes;
      return 1;
    }
  }
#endif

  return hz6_linux_mmap_retain_put_global(ptr, bytes);
}
#endif

Hz6LinuxMmapRetainStats hz6_linux_mmap_retain_stats_snapshot(void) {
  Hz6LinuxMmapRetainStats stats = {0};
#if HZ6_LINUX_MMAP_RETAIN_L1
  stats.reserve_calls = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_reserve_calls, memory_order_relaxed);
  stats.reserve_64k_calls = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_reserve_64k_calls, memory_order_relaxed);
  stats.retain_64k_take_hit = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_64k_take_hit, memory_order_relaxed);
  stats.retain_64k_take_miss = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_64k_take_miss, memory_order_relaxed);
  stats.retain_generic_take_hit = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_generic_take_hit, memory_order_relaxed);
  stats.retain_generic_take_miss = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_generic_take_miss, memory_order_relaxed);
  stats.mmap_fallback = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_mmap_fallback, memory_order_relaxed);
  stats.release_calls = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_release_calls, memory_order_relaxed);
  stats.release_64k_calls = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_release_64k_calls, memory_order_relaxed);
  stats.retain_64k_put_hit = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_64k_put_hit, memory_order_relaxed);
  stats.retain_64k_put_full = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_64k_put_full, memory_order_relaxed);
  stats.retain_generic_put_hit = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_generic_put_hit, memory_order_relaxed);
  stats.retain_generic_put_full = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_generic_put_full, memory_order_relaxed);
  stats.munmap_fallback = atomic_load_explicit(
      &g_hz6_linux_mmap_retain_munmap_fallback, memory_order_relaxed);
  pthread_mutex_lock(&g_hz6_linux_mmap_retain_mutex);
  stats.retained_bytes = g_hz6_linux_mmap_retained_bytes;
#if HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
  stats.retained_64k_count = g_hz6_linux_mmap_retained_64k_count;
#endif
  pthread_mutex_unlock(&g_hz6_linux_mmap_retain_mutex);
#endif
  return stats;
}

size_t hz6_linux_page_size(void) {
  long value = sysconf(_SC_PAGESIZE);
  return value > 0 ? (size_t)value : (size_t)4096;
}

void* hz6_linux_mmap_reserve(size_t bytes, size_t align) {
  size_t page_size = hz6_linux_page_size();
  size_t effective_align = align > page_size ? align : page_size;
  size_t rounded = hz6_source_round_up(bytes, effective_align);
  if (rounded == 0) {
    errno = EINVAL;
    return NULL;
  }

#if HZ6_LINUX_MMAP_RETAIN_L1
  hz6_linux_mmap_retain_counter_inc(
      &g_hz6_linux_mmap_retain_reserve_calls);
#if HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
  if (rounded == HZ6_LINUX_MMAP_RETAIN_64K_BYTES) {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_reserve_64k_calls);
  }
#endif
  void* retained = hz6_linux_mmap_retain_take(rounded);
  if (retained) {
    return retained;
  }
  hz6_linux_mmap_retain_counter_inc(
      &g_hz6_linux_mmap_retain_mmap_fallback);
#endif

  void* p = mmap(NULL, rounded, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return p == MAP_FAILED ? NULL : p;
}

int hz6_linux_mmap_commit(void* p, size_t bytes) {
  (void)p;
  return bytes != 0;
}

int hz6_linux_mmap_decommit(void* p, size_t bytes) {
  if (!p || bytes == 0) {
    return 0;
  }
  return madvise(p, bytes, MADV_DONTNEED) == 0;
}

int hz6_linux_mmap_release(void* p, size_t bytes) {
  if (!p || bytes == 0) {
    return 0;
  }
#if HZ6_LINUX_MMAP_RETAIN_L1
  hz6_linux_mmap_retain_counter_inc(
      &g_hz6_linux_mmap_retain_release_calls);
#if HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1
  if (bytes == HZ6_LINUX_MMAP_RETAIN_64K_BYTES) {
    hz6_linux_mmap_retain_counter_inc(
        &g_hz6_linux_mmap_retain_release_64k_calls);
  }
#endif
#if HZ6_LINUX_MMAP_RETAIN_PURGE_ON_RELEASE_L1
  (void)madvise(p, bytes, MADV_DONTNEED);
#endif
  if (hz6_linux_mmap_retain_put(p, bytes)) {
    return 1;
  }
  hz6_linux_mmap_retain_counter_inc(
      &g_hz6_linux_mmap_retain_munmap_fallback);
#endif
  return munmap(p, bytes) == 0;
}
