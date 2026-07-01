/*
 * Experimental direct-large Hot/Cold cache support.
 *
 * This file is compiled only for opt-in LargeDirect probe targets. It is not
 * part of the HZ8 balanced default. The current global-lock HotColdCache-L1
 * shape is retained for diagnostics, but the lane is HOLD: measured data shows
 * high reuse with excessive cache-lock contention and hot/cold churn.
 */

static size_t h8_direct_large_hotcold_bucket(size_t size) {
  size_t rounded =
      (size + H8_DIRECT_LARGE_HOTCOLD_UNIT - 1u) &
      ~(size_t)(H8_DIRECT_LARGE_HOTCOLD_UNIT - 1u);
  if (rounded <= H8_DIRECT_LARGE_HOTCOLD_MIN) {
    return 0;
  }
  size_t bucket =
      (rounded - H8_DIRECT_LARGE_HOTCOLD_MIN) / H8_DIRECT_LARGE_HOTCOLD_UNIT;
  return bucket < H8_DIRECT_LARGE_HOTCOLD_CLASSES
             ? bucket
             : H8_DIRECT_LARGE_HOTCOLD_CLASSES - 1u;
}

static size_t h8_direct_large_hotcold_payload(size_t bucket) {
  return H8_DIRECT_LARGE_HOTCOLD_MIN +
         bucket * H8_DIRECT_LARGE_HOTCOLD_UNIT;
}

#if defined(H8_LARGE_DIRECT_HOTCOLD_SHADOW_L2)
static size_t h8_direct_large_hotcold_bytes(size_t bucket) {
  return h8_direct_large_hotcold_payload(bucket) +
         H8_DIRECT_LARGE_HEADER_BYTES;
}

static h8_platform_mutex_t h8_direct_large_shadow_lock =
    H8_PLATFORM_MUTEX_INIT;
static size_t h8_direct_large_shadow_hot_count[H8_DIRECT_LARGE_HOTCOLD_CLASSES];
static size_t h8_direct_large_shadow_cold_count[H8_DIRECT_LARGE_HOTCOLD_CLASSES];
static size_t h8_direct_large_shadow_hot_bytes;
static size_t h8_direct_large_shadow_cold_bytes;

static bool h8_direct_large_shadow_take_hot(size_t bucket, size_t request) {
  for (size_t i = bucket; i < H8_DIRECT_LARGE_HOTCOLD_CLASSES; ++i) {
    atomic_fetch_add_explicit(&h8g.direct_large_hot_scan_steps, 1,
                              memory_order_relaxed);
    if (h8_direct_large_shadow_hot_count[i] == 0) {
      continue;
    }
    size_t bytes = h8_direct_large_hotcold_bytes(i);
    --h8_direct_large_shadow_hot_count[i];
    h8_direct_large_shadow_hot_bytes -= bytes;
    atomic_fetch_sub_explicit(&h8g.direct_large_hot_bytes, bytes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.direct_large_hot_hit_bucket[i], 1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.direct_large_shadow_cache_hit_count, 1,
                              memory_order_relaxed);
    if (i == bucket) {
      atomic_fetch_add_explicit(&h8g.direct_large_hot_exact_hit, 1,
                                memory_order_relaxed);
    } else {
      size_t payload = h8_direct_large_hotcold_payload(i);
      atomic_fetch_add_explicit(&h8g.direct_large_hot_near_hit, 1,
                                memory_order_relaxed);
      atomic_fetch_add_explicit(&h8g.direct_large_hot_oversize_bytes,
                                payload > request ? payload - request : 0,
                                memory_order_relaxed);
    }
    return true;
  }
  return false;
}

static bool h8_direct_large_shadow_release_cold(size_t bucket) {
  if (h8_direct_large_shadow_cold_count[bucket] == 0) {
    return false;
  }
  size_t bytes = h8_direct_large_hotcold_bytes(bucket);
  --h8_direct_large_shadow_cold_count[bucket];
  h8_direct_large_shadow_cold_bytes -= bytes;
  atomic_fetch_sub_explicit(&h8g.direct_large_cold_bytes, bytes,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cold_release_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cold_release_bytes, bytes,
                            memory_order_relaxed);
  return true;
}

static bool h8_direct_large_shadow_release_any_cold(size_t preferred) {
  for (size_t n = 0; n < H8_DIRECT_LARGE_HOTCOLD_CLASSES; ++n) {
    size_t bucket = (preferred + n) % H8_DIRECT_LARGE_HOTCOLD_CLASSES;
    if (h8_direct_large_shadow_release_cold(bucket)) {
      return true;
    }
  }
  return false;
}

static void h8_direct_large_shadow_demote_hot(size_t bucket) {
  if (h8_direct_large_shadow_hot_count[bucket] == 0) {
    return;
  }
  size_t bytes = h8_direct_large_hotcold_bytes(bucket);
  --h8_direct_large_shadow_hot_count[bucket];
  h8_direct_large_shadow_hot_bytes -= bytes;
  atomic_fetch_sub_explicit(&h8g.direct_large_hot_bytes, bytes,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_hot_to_cold_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_lazy_purge_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_lazy_purge_bytes, bytes,
                            memory_order_relaxed);
  while (h8_direct_large_shadow_cold_bytes + bytes >
         H8_DIRECT_LARGE_HOTCOLD_COLD_BYTES) {
    if (!h8_direct_large_shadow_release_any_cold(bucket)) {
      break;
    }
  }
  ++h8_direct_large_shadow_cold_count[bucket];
  h8_direct_large_shadow_cold_bytes += bytes;
  atomic_fetch_add_explicit(&h8g.direct_large_cold_bytes, bytes,
                            memory_order_relaxed);
  h8_direct_large_update_peak(&h8g.direct_large_cold_peak_bytes,
                              h8_direct_large_shadow_cold_bytes);
}

static void h8_direct_large_shadow_store_hot(size_t bucket) {
  size_t bytes = h8_direct_large_hotcold_bytes(bucket);
  while ((h8_direct_large_shadow_hot_bytes + bytes >
          H8_DIRECT_LARGE_HOTCOLD_HOT_BYTES) ||
         (h8_direct_large_shadow_hot_count[bucket] + 1u) * bytes >
             H8_DIRECT_LARGE_HOTCOLD_HOT_CLASS_BYTES) {
    if (h8_direct_large_shadow_hot_count[bucket] == 0) {
      atomic_fetch_add_explicit(&h8g.direct_large_hot_reject_bucket[bucket], 1,
                                memory_order_relaxed);
      atomic_fetch_add_explicit(&h8g.direct_large_free_to_release_count, 1,
                                memory_order_relaxed);
      return;
    }
    h8_direct_large_shadow_demote_hot(bucket);
  }
  ++h8_direct_large_shadow_hot_count[bucket];
  h8_direct_large_shadow_hot_bytes += bytes;
  atomic_fetch_add_explicit(&h8g.direct_large_hot_store_bucket[bucket], 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_hot_bytes, bytes,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_free_to_hot_count, 1,
                            memory_order_relaxed);
  h8_direct_large_update_peak(&h8g.direct_large_hot_peak_bytes,
                              h8_direct_large_shadow_hot_bytes);
}

static void h8_direct_large_shadow_record_alloc(size_t size) {
  size_t bucket = h8_direct_large_hotcold_bucket(size);
  h8_platform_mutex_lock(&h8_direct_large_shadow_lock);
  bool hit = h8_direct_large_shadow_take_hot(bucket, size);
  h8_platform_mutex_unlock(&h8_direct_large_shadow_lock);
  if (!hit) {
    atomic_fetch_add_explicit(&h8g.direct_large_raw_alloc_count, 1,
                              memory_order_relaxed);
  }
}

static void h8_direct_large_shadow_record_free(size_t size) {
  size_t bucket = h8_direct_large_hotcold_bucket(size);
  h8_platform_mutex_lock(&h8_direct_large_shadow_lock);
  h8_direct_large_shadow_store_hot(bucket);
  h8_platform_mutex_unlock(&h8_direct_large_shadow_lock);
}
#endif

#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
static H8DirectLarge* h8_direct_large_hot[H8_DIRECT_LARGE_HOTCOLD_CLASSES];
static H8DirectLarge* h8_direct_large_cold[H8_DIRECT_LARGE_HOTCOLD_CLASSES];
static size_t h8_direct_large_hot_class_bytes[H8_DIRECT_LARGE_HOTCOLD_CLASSES];
static size_t h8_direct_large_hot_bytes;
static size_t h8_direct_large_cold_bytes;

static uint64_t h8_direct_large_now_ns(void) {
  return h8_platform_now_ns();
}

static void h8_direct_large_lock_hotcold(void) {
  /* Diagnostic timing counters are intentionally probe-only. */
  uint64_t start = h8_direct_large_now_ns();
  h8_platform_mutex_lock(&h8_direct_large_lock);
  uint64_t end = h8_direct_large_now_ns();
  atomic_fetch_add_explicit(&h8g.direct_large_cache_lock_acquire_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cache_lock_wait_ns,
                            (size_t)(end - start), memory_order_relaxed);
}

static void h8_direct_large_unlock_hotcold(uint64_t hold_start) {
  uint64_t end = h8_direct_large_now_ns();
  atomic_fetch_add_explicit(&h8g.direct_large_cache_lock_hold_ns,
                            (size_t)(end - hold_start),
                            memory_order_relaxed);
  h8_platform_mutex_unlock(&h8_direct_large_lock);
}

static void h8_direct_large_cache_add_bytes(atomic_size_t* counter,
                                            size_t bytes) {
  atomic_fetch_add_explicit(counter, bytes, memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cache_bytes, bytes,
                            memory_order_relaxed);
}

static void h8_direct_large_cache_sub_bytes(atomic_size_t* counter,
                                            size_t bytes) {
  atomic_fetch_sub_explicit(counter, bytes, memory_order_relaxed);
  atomic_fetch_sub_explicit(&h8g.direct_large_cache_bytes, bytes,
                            memory_order_relaxed);
}

static H8DirectLarge* h8_direct_large_pop_list(H8DirectLarge** list) {
  H8DirectLarge* node = *list;
  if (node) {
    *list = node->cache_next;
    node->cache_next = NULL;
  }
  return node;
}

static void h8_direct_large_push_list(H8DirectLarge** list,
                                      H8DirectLarge* node) {
  node->cache_next = *list;
  *list = node;
}

static H8DirectLarge* h8_direct_large_take_fit(H8DirectLarge** list,
                                               size_t needed) {
  H8DirectLarge** link = list;
  while (*link) {
    H8DirectLarge* node = *link;
    if (node->mapped_size >= needed) {
      *link = node->cache_next;
      node->cache_next = NULL;
      return node;
    }
    link = &node->cache_next;
  }
  return NULL;
}

static H8DirectLarge* h8_direct_large_hotcold_take(size_t size, bool hot) {
  size_t first = h8_direct_large_hotcold_bucket(size);
  H8DirectLarge** lists = hot ? h8_direct_large_hot : h8_direct_large_cold;
  size_t needed = size + H8_DIRECT_LARGE_HEADER_BYTES;
  bool saw_node = false;
  for (size_t i = first; i < H8_DIRECT_LARGE_HOTCOLD_CLASSES; ++i) {
    atomic_fetch_add_explicit(&h8g.direct_large_hot_scan_steps, 1,
                              memory_order_relaxed);
    if (lists[i]) {
      saw_node = true;
    }
    H8DirectLarge* node = h8_direct_large_take_fit(&lists[i], needed);
    if (!node) {
      continue;
    }
    size_t bytes = node->mapped_size;
    if (hot) {
      h8_direct_large_hot_bytes -= bytes;
      h8_direct_large_hot_class_bytes[i] -= bytes;
      h8_direct_large_cache_sub_bytes(&h8g.direct_large_hot_bytes, bytes);
    } else {
      h8_direct_large_cold_bytes -= bytes;
      h8_direct_large_cache_sub_bytes(&h8g.direct_large_cold_bytes, bytes);
    }
    atomic_fetch_add_explicit(&h8g.direct_large_hot_hit_bucket[i], 1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(
        hot ? &h8g.direct_large_hot_hit_by_bucket[i]
            : &h8g.direct_large_cold_hit_by_bucket[i],
        1, memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.direct_large_cache_hit_count, 1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&h8g.direct_large_shadow_cache_hit_count, 1,
                              memory_order_relaxed);
    if (i == first) {
      atomic_fetch_add_explicit(&h8g.direct_large_hot_exact_hit, 1,
                                memory_order_relaxed);
    } else {
      size_t payload = h8_direct_large_hotcold_payload(i);
      atomic_fetch_add_explicit(&h8g.direct_large_hot_near_hit, 1,
                                memory_order_relaxed);
      atomic_fetch_add_explicit(&h8g.direct_large_hot_oversize_bytes,
                                payload > size ? payload - size : 0,
                                memory_order_relaxed);
    }
    return node;
  }
  atomic_fetch_add_explicit(
      hot ? (saw_node ? &h8g.direct_large_hot_miss_too_small
                      : &h8g.direct_large_hot_miss_empty)
          : (saw_node ? &h8g.direct_large_cold_miss_too_small
                      : &h8g.direct_large_cold_miss_empty),
      1, memory_order_relaxed);
  return NULL;
}

static bool h8_direct_large_hotcold_release_cold(size_t bucket) {
  H8DirectLarge* node = h8_direct_large_pop_list(&h8_direct_large_cold[bucket]);
  if (!node) {
    return false;
  }
  size_t bytes = node->mapped_size;
  h8_direct_large_cold_bytes -= bytes;
  h8_direct_large_cache_sub_bytes(&h8g.direct_large_cold_bytes, bytes);
  atomic_fetch_add_explicit(&h8g.direct_large_cold_release_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_release_by_bucket[bucket], 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cold_release_bytes, bytes,
                            memory_order_relaxed);
  h8_direct_large_remove_locked(node);
  uint64_t start = h8_direct_large_now_ns();
  h8_direct_large_free_raw(node, bytes);
  atomic_fetch_add_explicit(&h8g.direct_large_release_ns,
                            (size_t)(h8_direct_large_now_ns() - start),
                            memory_order_relaxed);
  return true;
}

static bool h8_direct_large_hotcold_release_any_cold(size_t preferred) {
  for (size_t n = 0; n < H8_DIRECT_LARGE_HOTCOLD_CLASSES; ++n) {
    size_t bucket = (preferred + n) % H8_DIRECT_LARGE_HOTCOLD_CLASSES;
    if (h8_direct_large_hotcold_release_cold(bucket)) {
      return true;
    }
  }
  return false;
}

static void h8_direct_large_hotcold_demote(size_t bucket) {
  H8DirectLarge* node = h8_direct_large_pop_list(&h8_direct_large_hot[bucket]);
  if (!node) {
    return;
  }
  size_t bytes = node->mapped_size;
  h8_direct_large_hot_bytes -= bytes;
  h8_direct_large_hot_class_bytes[bucket] -= bytes;
  h8_direct_large_cache_sub_bytes(&h8g.direct_large_hot_bytes, bytes);
  uint64_t purge_start = h8_direct_large_now_ns();
  h8_direct_large_purge_payload(node);
  atomic_fetch_add_explicit(&h8g.direct_large_purge_ns,
                            (size_t)(h8_direct_large_now_ns() - purge_start),
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_hot_to_cold_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_demote_by_bucket[bucket], 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_lazy_purge_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_lazy_purge_bytes, bytes,
                            memory_order_relaxed);
  while (h8_direct_large_cold_bytes + bytes >
         H8_DIRECT_LARGE_HOTCOLD_COLD_BYTES) {
    if (!h8_direct_large_hotcold_release_any_cold(bucket)) {
      break;
    }
  }
  h8_direct_large_push_list(&h8_direct_large_cold[bucket], node);
  h8_direct_large_cold_bytes += bytes;
  h8_direct_large_cache_add_bytes(&h8g.direct_large_cold_bytes, bytes);
  h8_direct_large_update_peak(&h8g.direct_large_cold_peak_bytes,
                              h8_direct_large_cold_bytes);
}

static H8DirectLarge* h8_direct_large_hotcold_pop_locked(size_t size) {
  H8DirectLarge* node = h8_direct_large_hotcold_take(size, true);
  if (!node) {
    node = h8_direct_large_hotcold_take(size, false);
  }
  if (node) {
    node->magic = H8_DIRECT_LARGE_MAGIC_LIVE;
  }
  return node;
}

static bool h8_direct_large_hotcold_push_locked(H8DirectLarge* node) {
  size_t bucket = h8_direct_large_hotcold_bucket(node->usable_size);
  size_t bytes = node->mapped_size;
  node->magic = H8_DIRECT_LARGE_MAGIC_DEAD;
  while ((h8_direct_large_hot_bytes + bytes >
          H8_DIRECT_LARGE_HOTCOLD_HOT_BYTES) ||
         (h8_direct_large_hot_class_bytes[bucket] + bytes >
          H8_DIRECT_LARGE_HOTCOLD_HOT_CLASS_BYTES) ||
         (bytes > H8_DIRECT_LARGE_HOTCOLD_HOT_CLASS_BYTES)) {
    if (!h8_direct_large_hot[bucket]) {
      atomic_fetch_add_explicit(&h8g.direct_large_hot_reject_bucket[bucket], 1,
                                memory_order_relaxed);
      atomic_fetch_add_explicit(&h8g.direct_large_free_to_release_count, 1,
                                memory_order_relaxed);
      return false;
    }
    h8_direct_large_hotcold_demote(bucket);
  }
  h8_direct_large_push_list(&h8_direct_large_hot[bucket], node);
  h8_direct_large_hot_bytes += bytes;
  h8_direct_large_hot_class_bytes[bucket] += bytes;
  h8_direct_large_cache_add_bytes(&h8g.direct_large_hot_bytes, bytes);
  atomic_fetch_add_explicit(&h8g.direct_large_hot_store_bucket[bucket], 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cache_store_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_free_to_hot_count, 1,
                            memory_order_relaxed);
  h8_direct_large_update_peak(&h8g.direct_large_hot_peak_bytes,
                              h8_direct_large_hot_bytes);
  return true;
}
#endif
