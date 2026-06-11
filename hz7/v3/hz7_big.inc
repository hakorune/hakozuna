static size_t h7_big_user_offset(void) {
  return h7_align_up(sizeof(H7Direct), 16u);
}

static void* h7_big_user_ptr(H7Direct* direct) {
  return (unsigned char*)direct + h7_big_user_offset();
}

static int h7_big_is_user_ptr(H7Direct* direct, void* ptr) {
  return direct && ptr == h7_big_user_ptr(direct);
}

static int h7_big_is_active_user(H7Direct* direct, void* ptr) {
  return direct && direct->region.magic == H7_MAGIC &&
         direct->region.kind == H7_REGION_DIRECT &&
         (direct->region.flags & H7_REGION_ACTIVE) != 0 &&
         h7_big_is_user_ptr(direct, ptr);
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

static void h7_big_retire_locked(H7Direct* direct,
                                 H7PendingRelease* release) {
  if (h7_direct_retain_push(direct)) {
    h7_big_move_to_retained(direct);
    return;
  }
  h7_big_detach_for_release(direct, release);
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
  if (!h7_big_is_active_user(direct, ptr)) {
    return;
  }
  h7_big_retire_locked(direct, release);
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
  H7RouteResult route = h7_route_lookup_user_ptr(ptr);
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
  return h7_route_lookup_user_ptr(ptr).kind;
}

H7RouteKind h7_route(void* ptr) {
  H7RouteKind kind;
  h7_lock();
  kind = h7_route_unlocked(ptr);
  h7_unlock();
  return kind;
}

static H7Stats h7_stats_locked(void) {
  H7Stats stats = g_h7_stats;
  stats.route_capacity = H7_ROUTE_CAPACITY;
  stats.empty_span_cap = H7_EMPTY_SPAN_CAP;
  stats.direct_retain_cap = H7_DIRECT_RETAIN_CAP;
#ifdef H7_REMOTE_NATURAL_PRESET
  stats.remote_natural_preset = 1u;
#else
  stats.remote_natural_preset = 0u;
#endif
  return stats;
}

H7Stats h7_stats(void) {
  H7Stats stats;
  h7_lock();
  stats = h7_stats_locked();
  h7_unlock();
  return stats;
}
