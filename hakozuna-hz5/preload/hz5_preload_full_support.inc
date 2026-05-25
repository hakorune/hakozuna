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

__attribute__((destructor)) static void hz5_preload_full_stats_destructor(
    void) {
  hz5_preload_full_stats_print();
  hz5_ownerhub_stats_print();
}

static void hz5_preload_full_resolve(void) {
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
