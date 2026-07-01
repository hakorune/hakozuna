#include "h8_internal.h"

#include <limits.h>

#if defined(H8_LARGE_DIRECT_OWNED_L1)

#define H8_DIRECT_LARGE_HEADER_BYTES 64u
#define H8_DIRECT_LARGE_BUCKETS 65536u
#define H8_DIRECT_LARGE_MAGIC_LIVE UINT64_C(0x6838444c41524745)
#define H8_DIRECT_LARGE_MAGIC_DEAD UINT64_C(0x6838444c44454144)
#define H8_DIRECT_LARGE_PAGE_BYTES 4096u

#if defined(H8_LARGE_DIRECT_RECYCLE_CACHE_L1) && \
    !defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
#error H8_LARGE_DIRECT_RECYCLE_CACHE_L1 requires H8_LARGE_DIRECT_MMAP_PAYLOAD_L1
#endif

#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1) && \
    !defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
#error H8_LARGE_DIRECT_HOTCOLD_CACHE_L1 requires H8_LARGE_DIRECT_MMAP_PAYLOAD_L1
#endif

#if defined(H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1) && \
    !defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
#error H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1 requires H8_LARGE_DIRECT_MMAP_PAYLOAD_L1
#endif

#if defined(H8_LARGE_DIRECT_PURGE_CACHE_L1) && \
    !defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
#error H8_LARGE_DIRECT_PURGE_CACHE_L1 requires H8_LARGE_DIRECT_MMAP_PAYLOAD_L1
#endif

#if (defined(H8_LARGE_DIRECT_PURGE_CACHE_L1) && \
     defined(H8_LARGE_DIRECT_RECYCLE_CACHE_L1)) || \
    (defined(H8_LARGE_DIRECT_PURGE_CACHE_L1) && \
     defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)) || \
    (defined(H8_LARGE_DIRECT_RECYCLE_CACHE_L1) && \
     defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)) || \
    (defined(H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1) && \
     (defined(H8_LARGE_DIRECT_PURGE_CACHE_L1) || \
      defined(H8_LARGE_DIRECT_RECYCLE_CACHE_L1) || \
      defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)))
#error choose only one direct-large cache policy
#endif

#if defined(H8_LARGE_DIRECT_PURGE_CACHE_L1) || \
    defined(H8_LARGE_DIRECT_RECYCLE_CACHE_L1)
#define H8_DIRECT_LARGE_CACHE_L1 1
#endif

#if defined(H8_LARGE_DIRECT_HOTCOLD_SHADOW_L2) || \
    defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1) || \
    defined(H8_LARGE_DIRECT_SHARDED_HOT_SHADOW_L1) || \
    defined(H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1)
#define H8_DIRECT_LARGE_HOTCOLD_L1 1
#endif

#if !defined(H8_DIRECT_LARGE_PURGE_CACHE_BYTES)
#define H8_DIRECT_LARGE_PURGE_CACHE_BYTES (64u * 1024u * 1024u)
#endif

#define H8_DIRECT_LARGE_HOTCOLD_CLASSES 8u
#define H8_DIRECT_LARGE_HOTCOLD_UNIT (8u * 1024u)
#define H8_DIRECT_LARGE_HOTCOLD_MIN (72u * 1024u)
#define H8_DIRECT_LARGE_HOTCOLD_HOT_BYTES (64u * 1024u * 1024u)
#define H8_DIRECT_LARGE_HOTCOLD_HOT_CLASS_BYTES (8u * 1024u * 1024u)
#define H8_DIRECT_LARGE_HOTCOLD_COLD_BYTES (64u * 1024u * 1024u)
#define H8_DIRECT_LARGE_SHARDED_HOT_SHARDS 8u
#if !defined(H8_DIRECT_LARGE_SHARDED_HOT_BYTES)
#define H8_DIRECT_LARGE_SHARDED_HOT_BYTES (64u * 1024u * 1024u)
#endif
#if !defined(H8_DIRECT_LARGE_SHARDED_HOT_SHARD_BYTES)
#define H8_DIRECT_LARGE_SHARDED_HOT_SHARD_BYTES (16u * 1024u * 1024u)
#endif

typedef struct H8DirectLarge {
  uint64_t magic;
  size_t requested_size;
  size_t usable_size;
  size_t mapped_size;
  void* user_ptr;
  struct H8DirectLarge* hash_next;
  struct H8DirectLarge* hash_prev;
  struct H8DirectLarge* cache_next;
} H8DirectLarge;

static h8_platform_mutex_t h8_direct_large_lock = H8_PLATFORM_MUTEX_INIT;
static H8DirectLarge* h8_direct_large_buckets[H8_DIRECT_LARGE_BUCKETS];
#if defined(H8_DIRECT_LARGE_CACHE_L1)
static H8DirectLarge* h8_direct_large_cache[4];
#endif

static void* h8_direct_large_alloc_raw(size_t bytes) {
#if defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
  return h8_platform_reserve_rw(bytes);
#else
  return h8_sys_malloc(bytes);
#endif
}

static void h8_direct_large_free_raw(void* ptr, size_t bytes) {
#if defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
  h8_platform_release(ptr, bytes);
#else
  (void)bytes;
  h8_sys_free(ptr);
#endif
}

static size_t h8_direct_large_payload_capacity(const H8DirectLarge* node) {
  if (node->mapped_size > H8_DIRECT_LARGE_HEADER_BYTES) {
    return node->mapped_size - H8_DIRECT_LARGE_HEADER_BYTES;
  }
  return node->usable_size;
}

static size_t h8_direct_large_mapped_size_for(size_t size) {
  size_t bytes = size + H8_DIRECT_LARGE_HEADER_BYTES;
#if defined(H8_LARGE_DIRECT_MMAP_PAYLOAD_L1)
  return (bytes + H8_DIRECT_LARGE_PAGE_BYTES - 1u) &
         ~(size_t)(H8_DIRECT_LARGE_PAGE_BYTES - 1u);
#else
  return bytes;
#endif
}

#if defined(H8_LARGE_DIRECT_PURGE_CACHE_L1) || \
    defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
static void h8_direct_large_purge_payload(H8DirectLarge* node) {
  uintptr_t begin = (uintptr_t)node->user_ptr;
  uintptr_t end = begin + h8_direct_large_payload_capacity(node);
  uintptr_t purge_begin =
      (begin + H8_DIRECT_LARGE_PAGE_BYTES - 1u) &
      ~(uintptr_t)(H8_DIRECT_LARGE_PAGE_BYTES - 1u);
  uintptr_t purge_end = end & ~(uintptr_t)(H8_DIRECT_LARGE_PAGE_BYTES - 1u);
  if (purge_end > purge_begin) {
    (void)h8_platform_purge((void*)purge_begin, (size_t)(purge_end - purge_begin));
  }
}
#endif

static size_t h8_direct_large_bucket(size_t size) {
  if (size <= 80u * 1024u) {
    return 0;
  }
  if (size <= 96u * 1024u) {
    return 1;
  }
  if (size <= 112u * 1024u) {
    return 2;
  }
  return 3;
}

static void h8_direct_large_update_live_peak(size_t live) {
  size_t peak = atomic_load_explicit(&h8g.direct_large_live_peak_bytes,
                                     memory_order_relaxed);
  while (live > peak &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.direct_large_live_peak_bytes, &peak, live,
             memory_order_release, memory_order_relaxed)) {
  }
}

static void h8_direct_large_remove_locked(H8DirectLarge* node);

#if defined(H8_DIRECT_LARGE_HOTCOLD_L1)
static void h8_direct_large_update_peak(atomic_size_t* peak_counter,
                                        size_t value) {
  size_t peak = atomic_load_explicit(peak_counter, memory_order_relaxed);
  while (value > peak &&
         !atomic_compare_exchange_weak_explicit(
             peak_counter, &peak, value, memory_order_release,
             memory_order_relaxed)) {
  }
}
#endif

#if defined(H8_DIRECT_LARGE_HOTCOLD_L1)
#include "h8_direct_large_hotcold.inc"
#endif

static void h8_direct_large_record_alloc(size_t size) {
#if defined(H8_LARGE_DIRECT_HOTCOLD_SHADOW_L2)
  h8_direct_large_shadow_record_alloc(size);
#endif
#if defined(H8_LARGE_DIRECT_SHARDED_HOT_SHADOW_L1)
  h8_direct_large_sharded_hot_shadow_record_alloc(size);
#endif
  size_t bucket = h8_direct_large_bucket(size);
  size_t event =
      atomic_fetch_add_explicit(&h8g.direct_large_event_epoch, 1,
                                memory_order_relaxed) +
      1u;
  size_t last_free = atomic_load_explicit(
      &h8g.direct_large_last_free_epoch[bucket], memory_order_acquire);
  if (last_free != 0 && event > last_free) {
    size_t distance = event - last_free;
    if (distance <= 1u) {
      atomic_fetch_add_explicit(&h8g.direct_large_reuse_distance_0_1, 1,
                                memory_order_relaxed);
    } else if (distance <= 7u) {
      atomic_fetch_add_explicit(&h8g.direct_large_reuse_distance_2_7, 1,
                                memory_order_relaxed);
    } else if (distance <= 31u) {
      atomic_fetch_add_explicit(&h8g.direct_large_reuse_distance_8_31, 1,
                                memory_order_relaxed);
    } else {
      atomic_fetch_add_explicit(&h8g.direct_large_reuse_distance_32p, 1,
                                memory_order_relaxed);
    }
  }
  atomic_fetch_add_explicit(&h8g.direct_large_alloc_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_alloc_bytes, size,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_alloc_bucket[bucket], 1,
                            memory_order_relaxed);
  size_t live = atomic_fetch_add_explicit(&h8g.direct_large_live_bytes, size,
                                          memory_order_relaxed) +
                size;
  h8_direct_large_update_live_peak(live);
}

static void h8_direct_large_record_free(size_t size) {
#if defined(H8_LARGE_DIRECT_HOTCOLD_SHADOW_L2)
  h8_direct_large_shadow_record_free(size);
#endif
#if defined(H8_LARGE_DIRECT_SHARDED_HOT_SHADOW_L1)
  h8_direct_large_sharded_hot_shadow_record_free(size);
#endif
  size_t bucket = h8_direct_large_bucket(size);
  size_t event =
      atomic_fetch_add_explicit(&h8g.direct_large_event_epoch, 1,
                                memory_order_relaxed) +
      1u;
  atomic_store_explicit(&h8g.direct_large_last_free_epoch[bucket], event,
                        memory_order_release);
  atomic_fetch_add_explicit(&h8g.direct_large_free_count, 1,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_free_bytes, size,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_free_bucket[bucket], 1,
                            memory_order_relaxed);
  atomic_fetch_sub_explicit(&h8g.direct_large_live_bytes, size,
                            memory_order_relaxed);
}

static size_t h8_direct_large_hash(const void* ptr) {
  uintptr_t value = (uintptr_t)ptr >> 4;
  value ^= value >> 33;
  value *= UINT64_C(0xff51afd7ed558ccd);
  value ^= value >> 33;
  return (size_t)(value & (H8_DIRECT_LARGE_BUCKETS - 1u));
}

static void h8_direct_large_insert_locked(H8DirectLarge* node) {
  uintptr_t begin = (uintptr_t)node->user_ptr;
  uintptr_t end = begin + h8_direct_large_payload_capacity(node);
  uintptr_t min =
      atomic_load_explicit(&h8g.direct_large_min_addr, memory_order_relaxed);
  if (min == 0 || begin < min) {
    atomic_store_explicit(&h8g.direct_large_min_addr, begin,
                          memory_order_release);
  }
  uintptr_t max =
      atomic_load_explicit(&h8g.direct_large_max_addr, memory_order_relaxed);
  if (end > max) {
    atomic_store_explicit(&h8g.direct_large_max_addr, end,
                          memory_order_release);
  }
  size_t bucket = h8_direct_large_hash(node->user_ptr);
  H8DirectLarge* head = h8_direct_large_buckets[bucket];
  node->hash_prev = NULL;
  node->hash_next = head;
  node->cache_next = NULL;
  if (head) {
    head->hash_prev = node;
  }
  h8_direct_large_buckets[bucket] = node;
}

#if defined(H8_DIRECT_LARGE_CACHE_L1)
static H8DirectLarge* h8_direct_large_cache_pop_locked(size_t size) {
  size_t bucket = h8_direct_large_bucket(size);
  H8DirectLarge** link = &h8_direct_large_cache[bucket];
  while (*link) {
    H8DirectLarge* node = *link;
    if (node->mapped_size >= size + H8_DIRECT_LARGE_HEADER_BYTES) {
      *link = node->cache_next;
      node->cache_next = NULL;
      atomic_fetch_sub_explicit(&h8g.direct_large_cache_bytes,
                                node->mapped_size, memory_order_relaxed);
      atomic_fetch_add_explicit(&h8g.direct_large_cache_hit_count, 1,
                                memory_order_relaxed);
      return node;
    }
    link = &node->cache_next;
  }
  return NULL;
}

static bool h8_direct_large_cache_push_locked(H8DirectLarge* node) {
  size_t cache_bytes =
      atomic_load_explicit(&h8g.direct_large_cache_bytes, memory_order_relaxed);
  if (cache_bytes + node->mapped_size > H8_DIRECT_LARGE_PURGE_CACHE_BYTES) {
    return false;
  }
  size_t bucket = h8_direct_large_bucket(node->usable_size);
  node->magic = H8_DIRECT_LARGE_MAGIC_DEAD;
#if defined(H8_LARGE_DIRECT_PURGE_CACHE_L1)
  h8_direct_large_purge_payload(node);
#endif
  node->cache_next = h8_direct_large_cache[bucket];
  h8_direct_large_cache[bucket] = node;
  atomic_fetch_add_explicit(&h8g.direct_large_cache_bytes, node->mapped_size,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&h8g.direct_large_cache_store_count, 1,
                            memory_order_relaxed);
  return true;
}
#endif

static void h8_direct_large_remove_locked(H8DirectLarge* node) {
  size_t bucket = h8_direct_large_hash(node->user_ptr);
  if (node->hash_prev) {
    node->hash_prev->hash_next = node->hash_next;
  } else {
    h8_direct_large_buckets[bucket] = node->hash_next;
  }
  if (node->hash_next) {
    node->hash_next->hash_prev = node->hash_prev;
  }
  node->hash_next = NULL;
  node->hash_prev = NULL;
}

static bool h8_direct_large_contains(const H8DirectLarge* node,
                                     const void* ptr) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t begin = (uintptr_t)node->user_ptr;
  uintptr_t end = begin + h8_direct_large_payload_capacity(node);
  return addr >= begin && addr < end;
}

static bool h8_direct_large_maybe_contains(const void* ptr) {
  return h8_direct_large_maybe_contains_hot(ptr);
}

static H8DirectLarge* h8_direct_large_find_locked(const void* ptr,
                                                  bool* exact_out) {
  size_t bucket = h8_direct_large_hash(ptr);
  for (H8DirectLarge* node = h8_direct_large_buckets[bucket]; node;
       node = node->hash_next) {
    if (node->user_ptr == ptr) {
      *exact_out = true;
      return node;
    }
  }
  for (size_t i = 0; i < H8_DIRECT_LARGE_BUCKETS; ++i) {
    for (H8DirectLarge* node = h8_direct_large_buckets[i]; node;
         node = node->hash_next) {
      if (h8_direct_large_contains(node, ptr)) {
        *exact_out = false;
        return node;
      }
    }
  }
  *exact_out = false;
  return NULL;
}

static H8DirectLarge* h8_direct_large_find_exact_locked(const void* ptr) {
  size_t bucket = h8_direct_large_hash(ptr);
  for (H8DirectLarge* node = h8_direct_large_buckets[bucket]; node;
       node = node->hash_next) {
    if (node->user_ptr == ptr) {
      return node;
    }
  }
  return NULL;
}

bool h8_direct_large_size_supported(size_t size) {
  return size > H8_MEDIUM_MAX_SIZE && size <= H8_DIRECT_FALLBACK_LIMIT;
}

void* h8_direct_large_malloc(size_t size) {
  if (!h8_direct_large_size_supported(size) ||
      size > SIZE_MAX - H8_DIRECT_LARGE_HEADER_BYTES) {
    return NULL;
  }
  size_t mapped_size = h8_direct_large_mapped_size_for(size);
#if defined(H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1)
  H8DirectLarge* sharded = h8_direct_large_sharded_hot_cache_pop(size);
  if (sharded) {
    sharded->requested_size = size;
    sharded->usable_size = size;
    h8_direct_large_record_alloc(size);
    return sharded->user_ptr;
  }
  atomic_fetch_add_explicit(&h8g.direct_large_raw_alloc_count, 1,
                            memory_order_relaxed);
#endif
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  h8_direct_large_lock_hotcold();
  uint64_t hold_start = h8_direct_large_now_ns();
  H8DirectLarge* hotcold = h8_direct_large_hotcold_pop_locked(size);
  if (hotcold) {
    hotcold->requested_size = size;
    hotcold->usable_size = size;
    h8_direct_large_unlock_hotcold(hold_start);
    h8_direct_large_record_alloc(size);
    return hotcold->user_ptr;
  }
  h8_direct_large_unlock_hotcold(hold_start);
  atomic_fetch_add_explicit(&h8g.direct_large_raw_alloc_count, 1,
                            memory_order_relaxed);
#endif
#if defined(H8_DIRECT_LARGE_CACHE_L1)
  h8_platform_mutex_lock(&h8_direct_large_lock);
  H8DirectLarge* cached = h8_direct_large_cache_pop_locked(size);
  if (cached) {
    cached->magic = H8_DIRECT_LARGE_MAGIC_LIVE;
    cached->requested_size = size;
    cached->usable_size = size;
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    h8_direct_large_record_alloc(size);
    return cached->user_ptr;
  }
  h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  uint64_t raw_start = h8_platform_now_ns();
#endif
  uint8_t* raw = h8_direct_large_alloc_raw(mapped_size);
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  atomic_fetch_add_explicit(&h8g.direct_large_raw_alloc_ns,
                            (size_t)(h8_platform_now_ns() - raw_start),
                            memory_order_relaxed);
#endif
  if (!raw) {
    return NULL;
  }
  H8DirectLarge* node = (H8DirectLarge*)raw;
  void* user = raw + H8_DIRECT_LARGE_HEADER_BYTES;
  node->magic = H8_DIRECT_LARGE_MAGIC_LIVE;
  node->requested_size = size;
  node->usable_size = size;
  node->mapped_size = mapped_size;
  node->user_ptr = user;
  node->hash_next = NULL;
  node->hash_prev = NULL;
  node->cache_next = NULL;
  h8_platform_mutex_lock(&h8_direct_large_lock);
  h8_direct_large_insert_locked(node);
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  h8_direct_large_record_alloc(size);
  return user;
}

bool h8_direct_large_free_inner(void* ptr, bool* owned_out) {
  *owned_out = false;
  if (!ptr || !h8_direct_large_maybe_contains(ptr)) {
    return false;
  }
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  h8_direct_large_lock_hotcold();
  uint64_t hold_start = h8_direct_large_now_ns();
#else
  h8_platform_mutex_lock(&h8_direct_large_lock);
#endif
  bool exact = false;
  H8DirectLarge* node = h8_direct_large_find_locked(ptr, &exact);
  if (!node) {
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
    h8_direct_large_unlock_hotcold(hold_start);
#else
    h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
    return false;
  }
  *owned_out = true;
  if (node->magic != H8_DIRECT_LARGE_MAGIC_LIVE || !exact) {
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
    h8_direct_large_unlock_hotcold(hold_start);
#else
    h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
    return false;
  }
  size_t size = node->usable_size;
#if defined(H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1)
  if (h8_direct_large_sharded_hot_cache_push(node)) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    h8_direct_large_record_free(size);
    return true;
  }
#endif
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  if (h8_direct_large_hotcold_push_locked(node)) {
    h8_direct_large_unlock_hotcold(hold_start);
    h8_direct_large_record_free(size);
    return true;
  }
#endif
#if defined(H8_DIRECT_LARGE_CACHE_L1)
  if (h8_direct_large_cache_push_locked(node)) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    h8_direct_large_record_free(size);
    return true;
  }
#endif
  h8_direct_large_remove_locked(node);
  size_t mapped_size = node->mapped_size;
  node->magic = H8_DIRECT_LARGE_MAGIC_DEAD;
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  h8_direct_large_unlock_hotcold(hold_start);
#else
  h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
  h8_direct_large_record_free(size);
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  uint64_t free_start = h8_platform_now_ns();
#endif
  h8_direct_large_free_raw(node, mapped_size);
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  atomic_fetch_add_explicit(&h8g.direct_large_release_ns,
                            (size_t)(h8_platform_now_ns() - free_start),
                            memory_order_relaxed);
#endif
  return true;
}

bool h8_direct_large_free_exact_inner(void* ptr, bool* owned_out) {
  *owned_out = false;
  if (!ptr || !h8_direct_large_maybe_contains(ptr)) {
    return false;
  }
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  h8_direct_large_lock_hotcold();
  uint64_t hold_start = h8_direct_large_now_ns();
#else
  h8_platform_mutex_lock(&h8_direct_large_lock);
#endif
  H8DirectLarge* node = h8_direct_large_find_exact_locked(ptr);
  if (!node) {
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
    h8_direct_large_unlock_hotcold(hold_start);
#else
    h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
    return false;
  }
  *owned_out = true;
  if (node->magic != H8_DIRECT_LARGE_MAGIC_LIVE) {
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
    h8_direct_large_unlock_hotcold(hold_start);
#else
    h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
    return false;
  }
  size_t size = node->usable_size;
#if defined(H8_LARGE_DIRECT_SHARDED_HOT_CACHE_L1)
  if (h8_direct_large_sharded_hot_cache_push(node)) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    h8_direct_large_record_free(size);
    return true;
  }
#endif
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  if (h8_direct_large_hotcold_push_locked(node)) {
    h8_direct_large_unlock_hotcold(hold_start);
    h8_direct_large_record_free(size);
    return true;
  }
#endif
#if defined(H8_DIRECT_LARGE_CACHE_L1)
  if (h8_direct_large_cache_push_locked(node)) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    h8_direct_large_record_free(size);
    return true;
  }
#endif
  h8_direct_large_remove_locked(node);
  size_t mapped_size = node->mapped_size;
  node->magic = H8_DIRECT_LARGE_MAGIC_DEAD;
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  h8_direct_large_unlock_hotcold(hold_start);
#else
  h8_platform_mutex_unlock(&h8_direct_large_lock);
#endif
  h8_direct_large_record_free(size);
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  uint64_t free_start = h8_platform_now_ns();
#endif
  h8_direct_large_free_raw(node, mapped_size);
#if defined(H8_LARGE_DIRECT_HOTCOLD_CACHE_L1)
  atomic_fetch_add_explicit(&h8g.direct_large_release_ns,
                            (size_t)(h8_platform_now_ns() - free_start),
                            memory_order_relaxed);
#endif
  return true;
}

bool h8_direct_large_usable_size_inner(void* ptr, size_t* usable_out,
                                       bool* owned_out) {
  *owned_out = false;
  if (!h8_direct_large_maybe_contains(ptr)) {
    return false;
  }
  h8_platform_mutex_lock(&h8_direct_large_lock);
  bool exact = false;
  H8DirectLarge* node = h8_direct_large_find_locked(ptr, &exact);
  if (!node) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  *owned_out = true;
  if (node->magic != H8_DIRECT_LARGE_MAGIC_LIVE || !exact) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  *usable_out = node->requested_size;
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  return true;
}

bool h8_direct_large_usable_size_exact_inner(void* ptr, size_t* usable_out,
                                             bool* owned_out) {
  *owned_out = false;
  if (!h8_direct_large_maybe_contains(ptr)) {
    return false;
  }
  h8_platform_mutex_lock(&h8_direct_large_lock);
  H8DirectLarge* node = h8_direct_large_find_exact_locked(ptr);
  if (!node) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  *owned_out = true;
  if (node->magic != H8_DIRECT_LARGE_MAGIC_LIVE) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  *usable_out = node->requested_size;
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  return true;
}

H8RouteKind h8_direct_large_route_inner(void* ptr) {
  if (!h8_direct_large_maybe_contains(ptr)) {
    return H8_ROUTE_MISS;
  }
  h8_platform_mutex_lock(&h8_direct_large_lock);
  bool exact = false;
  H8DirectLarge* node = h8_direct_large_find_locked(ptr, &exact);
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  if (!node) {
    return H8_ROUTE_MISS;
  }
  if (node->magic != H8_DIRECT_LARGE_MAGIC_LIVE) {
    return H8_ROUTE_INVALID;
  }
  return exact ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
}

H8RouteKind h8_direct_large_route_exact_inner(void* ptr) {
  if (!h8_direct_large_maybe_contains(ptr)) {
    return H8_ROUTE_MISS;
  }
  h8_platform_mutex_lock(&h8_direct_large_lock);
  H8DirectLarge* node = h8_direct_large_find_exact_locked(ptr);
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  if (!node) {
    return H8_ROUTE_MISS;
  }
  return node->magic == H8_DIRECT_LARGE_MAGIC_LIVE ? H8_ROUTE_VALID
                                                   : H8_ROUTE_INVALID;
}

#else

bool h8_direct_large_size_supported(size_t size) {
  (void)size;
  return false;
}

void* h8_direct_large_malloc(size_t size) {
  (void)size;
  return NULL;
}

bool h8_direct_large_free_inner(void* ptr, bool* owned_out) {
  (void)ptr;
  *owned_out = false;
  return false;
}

bool h8_direct_large_free_exact_inner(void* ptr, bool* owned_out) {
  (void)ptr;
  *owned_out = false;
  return false;
}

bool h8_direct_large_usable_size_inner(void* ptr, size_t* usable_out,
                                       bool* owned_out) {
  (void)ptr;
  (void)usable_out;
  *owned_out = false;
  return false;
}

bool h8_direct_large_usable_size_exact_inner(void* ptr, size_t* usable_out,
                                             bool* owned_out) {
  (void)ptr;
  (void)usable_out;
  *owned_out = false;
  return false;
}

H8RouteKind h8_direct_large_route_inner(void* ptr) {
  (void)ptr;
  return H8_ROUTE_MISS;
}

H8RouteKind h8_direct_large_route_exact_inner(void* ptr) {
  (void)ptr;
  return H8_ROUTE_MISS;
}

#endif
