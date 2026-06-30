#include "h8_internal.h"

#include <limits.h>

#if defined(H8_LARGE_DIRECT_OWNED_L1)

#define H8_DIRECT_LARGE_HEADER_BYTES 64u
#define H8_DIRECT_LARGE_BUCKETS 65536u
#define H8_DIRECT_LARGE_MAGIC_LIVE UINT64_C(0x6838444c41524745)
#define H8_DIRECT_LARGE_MAGIC_DEAD UINT64_C(0x6838444c44454144)

typedef struct H8DirectLarge {
  uint64_t magic;
  size_t requested_size;
  size_t usable_size;
  void* user_ptr;
  struct H8DirectLarge* hash_next;
  struct H8DirectLarge* hash_prev;
} H8DirectLarge;

static h8_platform_mutex_t h8_direct_large_lock = H8_PLATFORM_MUTEX_INIT;
static H8DirectLarge* h8_direct_large_buckets[H8_DIRECT_LARGE_BUCKETS];

static size_t h8_direct_large_hash(const void* ptr) {
  uintptr_t value = (uintptr_t)ptr >> 4;
  value ^= value >> 33;
  value *= UINT64_C(0xff51afd7ed558ccd);
  value ^= value >> 33;
  return (size_t)(value & (H8_DIRECT_LARGE_BUCKETS - 1u));
}

static void h8_direct_large_insert_locked(H8DirectLarge* node) {
  size_t bucket = h8_direct_large_hash(node->user_ptr);
  H8DirectLarge* head = h8_direct_large_buckets[bucket];
  node->hash_prev = NULL;
  node->hash_next = head;
  if (head) {
    head->hash_prev = node;
  }
  h8_direct_large_buckets[bucket] = node;
}

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
  uintptr_t end = begin + node->usable_size;
  return addr >= begin && addr < end;
}

static H8DirectLarge* h8_direct_large_find_locked(const void* ptr,
                                                  bool* exact_out) {
  size_t bucket = h8_direct_large_hash(ptr);
  for (H8DirectLarge* node = h8_direct_large_buckets[bucket]; node;
       node = node->hash_next) {
    if (node->magic == H8_DIRECT_LARGE_MAGIC_LIVE && node->user_ptr == ptr) {
      *exact_out = true;
      return node;
    }
  }
  for (size_t i = 0; i < H8_DIRECT_LARGE_BUCKETS; ++i) {
    for (H8DirectLarge* node = h8_direct_large_buckets[i]; node;
         node = node->hash_next) {
      if (node->magic == H8_DIRECT_LARGE_MAGIC_LIVE &&
          h8_direct_large_contains(node, ptr)) {
        *exact_out = false;
        return node;
      }
    }
  }
  *exact_out = false;
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
  uint8_t* raw = h8_sys_malloc(size + H8_DIRECT_LARGE_HEADER_BYTES);
  if (!raw) {
    return NULL;
  }
  H8DirectLarge* node = (H8DirectLarge*)raw;
  void* user = raw + H8_DIRECT_LARGE_HEADER_BYTES;
  node->magic = H8_DIRECT_LARGE_MAGIC_LIVE;
  node->requested_size = size;
  node->usable_size = size;
  node->user_ptr = user;
  node->hash_next = NULL;
  node->hash_prev = NULL;
  h8_platform_mutex_lock(&h8_direct_large_lock);
  h8_direct_large_insert_locked(node);
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  return user;
}

bool h8_direct_large_free_inner(void* ptr, bool* owned_out) {
  *owned_out = false;
  if (!ptr) {
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
  if (!exact) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  h8_direct_large_remove_locked(node);
  node->magic = H8_DIRECT_LARGE_MAGIC_DEAD;
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  h8_sys_free(node);
  return true;
}

bool h8_direct_large_usable_size_inner(void* ptr, size_t* usable_out,
                                       bool* owned_out) {
  *owned_out = false;
  h8_platform_mutex_lock(&h8_direct_large_lock);
  bool exact = false;
  H8DirectLarge* node = h8_direct_large_find_locked(ptr, &exact);
  if (!node) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  *owned_out = true;
  if (!exact) {
    h8_platform_mutex_unlock(&h8_direct_large_lock);
    return false;
  }
  *usable_out = node->requested_size;
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  return true;
}

H8RouteKind h8_direct_large_route_inner(void* ptr) {
  h8_platform_mutex_lock(&h8_direct_large_lock);
  bool exact = false;
  H8DirectLarge* node = h8_direct_large_find_locked(ptr, &exact);
  h8_platform_mutex_unlock(&h8_direct_large_lock);
  if (!node) {
    return H8_ROUTE_MISS;
  }
  return exact ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
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

bool h8_direct_large_usable_size_inner(void* ptr, size_t* usable_out,
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

#endif
