#include "hz6_allocator.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_route_shared_directory.h"
#include "hz6_allocator_same_owner_fast_inline.h"
#include "hz6_allocator_toy_small_diag.h"

#include "../fronts/hz6_front.h"
#include "../fronts/hz6_front_util.h"
#include "../fronts/midpage/hz6_midpage_front.h"
#include "../fronts/toy/hz6_toy_front.h"

#if HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_MIDPAGE_SKIP_TRANSFER_NOINLINE __attribute__((noinline))
#define HZ6_MIDPAGE_SKIP_TRANSFER_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_MIDPAGE_SKIP_TRANSFER_NOINLINE
#define HZ6_MIDPAGE_SKIP_TRANSFER_UNLIKELY(expr) (expr)
#endif
#endif

#if HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_TOY_DIRECT_CLASS_NOINLINE __attribute__((noinline))
#else
#define HZ6_PRELOAD_TOY_DIRECT_CLASS_NOINLINE
#endif
#endif

#if HZ6_TOY_PRECLASSIFIED_MALLOC_L1 || HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1
static uint16_t hz6_allocator_toy_class_for_small_size(size_t size) {
  if (size <= 16u) {
    return 0;
  }
  if (size <= 64u) {
    return 1;
  }
  if (size <= 256u) {
    return 2;
  }
  if (size <= 1024u) {
    return 3;
  }
  return 4;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 || HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static int hz6_allocator_direct_local_alloc_front_eligible(
    uint16_t front_id,
    uint16_t class_id) {
  if (class_id > HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS) {
    return 0;
  }
  return front_id == HZ6_FRONT_TOY || front_id == HZ6_FRONT_MIDPAGE ||
         front_id == HZ6_FRONT_LOCAL2P;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static int hz6_allocator_direct_local_reuse_pop(Hz6Allocator* allocator,
                                                uint16_t class_id,
                                                Hz6FrontCacheEntry* out) {
#if HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1 && !HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  Hz6FrontCacheBin* bin = &allocator->frontcache_bins[class_id];
  if (!bin->entries || bin->count == 0) {
    return 0;
  }
  *out = bin->entries[--bin->count];
  return 1;
#else
  return hz6_allocator_frontcache_pop(allocator, class_id, out);
#endif
}

static void* hz6_allocator_direct_local_reuse(Hz6Allocator* allocator,
                                              uint16_t front_id,
                                              uint16_t class_id,
                                              size_t requested_bytes,
                                              Hz6ObjectDescriptor**
                                                  out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  while (hz6_allocator_direct_local_reuse_pop(allocator, class_id, &entry)) {
    if (!entry.descriptor) {
      (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
      return NULL;
    }

    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (descriptor->class_id == class_id &&
#if HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
        hz6_allocator_activate_local_descriptor_trusted_owner(
            allocator, descriptor, entry.ptr, entry.generation)) {
#else
        hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            hz6_allocator_owner_token(allocator))) {
#endif
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_hit;
#endif
      if (out_descriptor) {
        *out_descriptor = descriptor;
      }
      return entry.ptr;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_reuse_invalid;
#endif
  }

#if HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 && HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (requested_bytes != 0 &&
      (front_id == HZ6_FRONT_TOY || front_id == HZ6_FRONT_MIDPAGE)) {
    ++allocator->stats.remote_pending_direct_gate_load;
    if (hz6_allocator_remote_pending_key_maybe_nonempty_raw(
            allocator, front_id, class_id)) {
      ++allocator->stats.remote_pending_direct_gate_hit;
      ++allocator->stats.remote_pending_direct_claim_attempt;
      void* pending_ptr = NULL;
      Hz6ObjectDescriptor* pending_descriptor = NULL;
      Hz6RemotePendingReuseStatus status =
          hz6_allocator_remote_pending_try_reuse(
              allocator, front_id, class_id, requested_bytes, &pending_ptr,
              &pending_descriptor);
      if (status == HZ6_REMOTE_PENDING_REUSE_OK && pending_ptr &&
          pending_descriptor) {
        ++allocator->stats.remote_pending_direct_claim_success;
        ++allocator->stats.remote_pending_direct_activate_success;
        if (out_descriptor) {
          *out_descriptor = pending_descriptor;
        }
        return pending_ptr;
      }
      if (status == HZ6_REMOTE_PENDING_REUSE_BUSY) {
        ++allocator->stats.remote_pending_direct_claim_busy;
      } else if (status == HZ6_REMOTE_PENDING_REUSE_EMPTY) {
        ++allocator->stats.remote_pending_direct_claim_empty_after_hint;
      } else {
        ++allocator->stats.remote_pending_direct_integrity_failure;
      }
    }
  }
#else
  (void)front_id;
  (void)requested_bytes;
#endif

#if HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  hz6_allocator_remote_pending_note_before_maintenance(allocator, front_id,
                                                       class_id);
  size_t pending_drained = hz6_allocator_remote_pending_maintenance_class(
      allocator, class_id, HZ6_REMOTE_PENDING_DRAIN_BUDGET);
  hz6_allocator_remote_pending_note_after_maintenance(allocator, front_id,
                                                      class_id);
  if (pending_drained != 0) {
    while (hz6_allocator_direct_local_reuse_pop(allocator, class_id,
                                                &entry)) {
      if (!entry.descriptor) {
        (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
        return NULL;
      }

      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)entry.descriptor;
      if (descriptor->class_id == class_id &&
#if HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
          hz6_allocator_activate_local_descriptor_trusted_owner(
              allocator, descriptor, entry.ptr, entry.generation)) {
#else
          hz6_allocator_activate_descriptor(
              allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr,
              entry.generation, hz6_allocator_owner_token(allocator))) {
#endif
#if HZ6_DIAGNOSTIC_PROBES
        ++allocator->stats.frontcache_reuse_hit;
#endif
        if (out_descriptor) {
          *out_descriptor = descriptor;
        }
        hz6_allocator_remote_pending_note_maintenance_reuse_success(
            allocator, pending_drained);
        return entry.ptr;
      }
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_invalid;
#endif
    }
  }
#endif

  return NULL;
}

#if HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1 && \
    HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1 &&      \
    HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 && HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static int hz6_allocator_midpage_trusted_class_reuse_enabled(
    uint16_t class_id) {
  return class_id >=
         HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_MIN_CLASS;
}

static void* hz6_allocator_midpage_direct_local_reuse_trusted_class(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested_bytes,
    Hz6ObjectDescriptor** out_descriptor) {
  *out_descriptor = NULL;
  (void)front_id;
  Hz6FrontCacheEntry entry;
  while (hz6_allocator_direct_local_reuse_pop(allocator, class_id, &entry)) {
    if (!entry.descriptor) {
      (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
      return NULL;
    }

    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (descriptor->class_id == class_id &&
        hz6_allocator_activate_local_descriptor_trusted_owner(
            allocator, descriptor, entry.ptr, entry.generation)) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_hit;
#endif
      *out_descriptor = descriptor;
      return entry.ptr;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_reuse_invalid;
#endif
  }

#if HZ6_REMOTE_PENDING_DIRECT_REUSE_L1 && HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (requested_bytes != 0) {
    ++allocator->stats.remote_pending_direct_gate_load;
    if (hz6_allocator_remote_pending_key_maybe_nonempty_raw(
            allocator, HZ6_FRONT_MIDPAGE, class_id)) {
      ++allocator->stats.remote_pending_direct_gate_hit;
      ++allocator->stats.remote_pending_direct_claim_attempt;
      void* pending_ptr = NULL;
      Hz6ObjectDescriptor* pending_descriptor = NULL;
      Hz6RemotePendingReuseStatus status =
          hz6_allocator_remote_pending_try_reuse(
              allocator, HZ6_FRONT_MIDPAGE, class_id, requested_bytes,
              &pending_ptr, &pending_descriptor);
      if (status == HZ6_REMOTE_PENDING_REUSE_OK && pending_ptr &&
          pending_descriptor) {
        ++allocator->stats.remote_pending_direct_claim_success;
        ++allocator->stats.remote_pending_direct_activate_success;
        *out_descriptor = pending_descriptor;
        return pending_ptr;
      }
      if (status == HZ6_REMOTE_PENDING_REUSE_BUSY) {
        ++allocator->stats.remote_pending_direct_claim_busy;
      } else if (status == HZ6_REMOTE_PENDING_REUSE_EMPTY) {
        ++allocator->stats.remote_pending_direct_claim_empty_after_hint;
      } else {
        ++allocator->stats.remote_pending_direct_integrity_failure;
      }
    }
  }
#else
  (void)requested_bytes;
#endif

#if HZ6_REMOTE_PENDING_OWNER_LOCAL_MAINTENANCE_L1 && \
    HZ6_REMOTE_PENDING_INBOX_CORE_L1
  hz6_allocator_remote_pending_note_before_maintenance(allocator, front_id,
                                                       class_id);
  size_t pending_drained = hz6_allocator_remote_pending_maintenance_class(
      allocator, class_id, HZ6_REMOTE_PENDING_DRAIN_BUDGET);
  hz6_allocator_remote_pending_note_after_maintenance(allocator, front_id,
                                                      class_id);
  if (pending_drained != 0) {
    while (hz6_allocator_direct_local_reuse_pop(allocator, class_id,
                                                &entry)) {
      if (!entry.descriptor) {
        (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
        return NULL;
      }

      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)entry.descriptor;
      if (descriptor->class_id == class_id &&
          hz6_allocator_activate_local_descriptor_trusted_owner(
              allocator, descriptor, entry.ptr, entry.generation)) {
#if HZ6_DIAGNOSTIC_PROBES
        ++allocator->stats.frontcache_reuse_hit;
#endif
        *out_descriptor = descriptor;
        hz6_allocator_remote_pending_note_maintenance_reuse_success(
            allocator, pending_drained);
        return entry.ptr;
      }
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_invalid;
#endif
    }
  }
#endif

  return NULL;
}
#endif
#endif

#if HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1
static void* hz6_allocator_preload_malloc_transfer_retry(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    Hz6ObjectDescriptor** out_descriptor) {
  if (!allocator ||
      allocator->stats.transfer_current <
          HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_MIN_TRANSFER_COUNT) {
    return NULL;
  }
  ++allocator->stats.preload_malloc_transfer_retry_eligible;
#if HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_STRIDE > 1
  if (allocator->stats.preload_malloc_transfer_retry_eligible %
          HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_STRIDE !=
      0) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.preload_malloc_transfer_retry_stride_skip;
#endif
    return NULL;
  }
#endif
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.preload_malloc_transfer_retry_attempt;
#endif
  void* ptr = hz6_front_reuse_transfer_with_descriptor(
      allocator, front_id, class_id, NULL, out_descriptor);
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  if (ptr) {
    ++allocator->stats.preload_malloc_transfer_retry_hit;
  }
#endif
  return ptr;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
static void* hz6_allocator_direct_local_alloc(Hz6Allocator* allocator,
                                              uint16_t front_id,
                                              uint16_t class_id,
                                              Hz6ObjectDescriptor**
                                                  out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  if (!hz6_allocator_direct_local_alloc_front_eligible(front_id, class_id)) {
    return NULL;
  }
  if (hz6_allocator_profile_transfer_first(allocator)) {
    /* Preserve transfer-first semantics before bypassing front dispatch. */
#if HZ6_DIAGNOSTIC_PROBES
    int midpage_transfer_probe =
        front_id == HZ6_FRONT_MIDPAGE &&
        (class_id == HZ6_MIDPAGE_8K_CLASS_ID ||
         class_id == HZ6_MIDPAGE_32K_CLASS_ID);
    if (midpage_transfer_probe) {
      ++allocator->stats.midpage_direct_transfer_probe_attempt;
      if (class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
        ++allocator->stats.midpage_8k_direct_transfer_probe_attempt;
      } else {
        ++allocator->stats.midpage_32k_direct_transfer_probe_attempt;
      }
    }
#endif
    void* transfer_ptr = hz6_front_reuse_transfer_with_descriptor(
        allocator, front_id, class_id, NULL, out_descriptor);
#if HZ6_DIAGNOSTIC_PROBES
    if (midpage_transfer_probe) {
      if (transfer_ptr) {
        ++allocator->stats.midpage_direct_transfer_probe_hit;
      } else {
        ++allocator->stats.midpage_direct_transfer_probe_empty;
      }
    }
#endif
    if (transfer_ptr) {
      return transfer_ptr;
    }
  }
#if HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
  return hz6_allocator_direct_local_reuse(allocator, front_id, class_id,
                                          0, out_descriptor);
#else
  (void)out_descriptor;
  return hz6_front_reuse_cached_or_transfer(allocator, front_id, class_id,
                                            NULL);
#endif
}
#endif

#if HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1 && \
    HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 && HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static HZ6_MIDPAGE_SKIP_TRANSFER_NOINLINE void*
hz6_allocator_midpage_direct_local_alloc_skip_transfer(
    Hz6Allocator* allocator,
    uint16_t class_id,
    Hz6ObjectDescriptor** out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  return hz6_allocator_direct_local_reuse(allocator, HZ6_FRONT_MIDPAGE,
                                          class_id, 0, out_descriptor);
}
#endif

#if HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1 && \
    HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 && HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
void* hz6_allocator_preload_midpage_malloc_skip_transfer(Hz6Allocator* allocator,
                                                         size_t size) {
  if (!allocator) {
    return NULL;
  }
#if !HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }
#endif

  uint16_t class_id = 0;
#if HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1
  if (size <= 4096 || size > HZ6_MIDPAGE_BYTES) {
    return hz6_malloc(allocator, size);
  }
  class_id = size <= HZ6_MIDPAGE_8K_BYTES ? HZ6_MIDPAGE_8K_CLASS_ID
                                          : HZ6_MIDPAGE_32K_CLASS_ID;
#else
  Hz6MidPageRunPolicy policy;
  if (!hz6_midpage_policy_for_size(size, 16, &policy)) {
    return hz6_malloc(allocator, size);
  }
  class_id = policy.class_id;
#endif

  Hz6ObjectDescriptor* descriptor = NULL;
  hz6_toy_small_hotpath_diag_malloc_fast_attempt(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
#if HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1
  void* ptr = hz6_allocator_midpage_trusted_class_reuse_enabled(class_id)
                  ? hz6_allocator_midpage_direct_local_reuse_trusted_class(
                        allocator, HZ6_FRONT_MIDPAGE, class_id, size,
                        &descriptor)
                  : hz6_allocator_direct_local_reuse(allocator,
                                                     HZ6_FRONT_MIDPAGE,
                                                     class_id, size,
                                                     &descriptor);
#else
  void* ptr = hz6_allocator_direct_local_reuse(allocator, HZ6_FRONT_MIDPAGE,
                                               class_id, size,
                                               &descriptor);
#endif
  if (ptr) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_direct;
#endif
    hz6_midpage_active_map_register(allocator, HZ6_FRONT_MIDPAGE,
                                    class_id, ptr, descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(
        allocator, HZ6_FRONT_MIDPAGE, class_id);
    return ptr;
  }
  hz6_allocator_remote_pending_note_frontcache_miss(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
#if HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1
  ptr = hz6_allocator_preload_malloc_transfer_retry(
      allocator, HZ6_FRONT_MIDPAGE, class_id, &descriptor);
  if (ptr) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_direct;
#endif
    hz6_midpage_active_map_register(allocator, HZ6_FRONT_MIDPAGE,
                                    class_id, ptr, descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(
        allocator, HZ6_FRONT_MIDPAGE, class_id);
    return ptr;
  }
#endif

  hz6_toy_small_hotpath_diag_malloc_front_dispatch(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
  hz6_allocator_remote_pending_note_front_dispatch(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
#if HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1
  ptr = hz6_midpage_alloc_with_descriptor(allocator, class_id, size,
                                          &descriptor);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }
  if (descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_front_alloc;
#endif
    hz6_midpage_active_map_register(allocator, HZ6_FRONT_MIDPAGE,
                                    class_id, ptr, descriptor);
  } else {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_route_fallback;
#endif
    hz6_midpage_active_map_register_route(allocator, HZ6_FRONT_MIDPAGE,
                                          class_id, ptr);
  }
  return ptr;
#else
  ptr = hz6_midpage_alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.midpage_active_map_register_route_fallback;
#endif
  hz6_midpage_active_map_register_route(allocator, HZ6_FRONT_MIDPAGE, class_id,
                                        ptr);
  return ptr;
#endif
}
#else
void* hz6_allocator_preload_midpage_malloc_skip_transfer(Hz6Allocator* allocator,
                                                         size_t size) {
  return hz6_malloc(allocator, size);
}
#endif

#if HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 && \
    HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1 && \
    HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 && HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
void* hz6_allocator_preload_midpage_malloc_class_skip_transfer(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t size) {
  if (!allocator) {
    return NULL;
  }
#if !HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }
#endif
  if (class_id != HZ6_MIDPAGE_8K_CLASS_ID &&
      class_id != HZ6_MIDPAGE_32K_CLASS_ID) {
    return hz6_malloc(allocator, size);
  }

  Hz6ObjectDescriptor* descriptor = NULL;
  hz6_toy_small_hotpath_diag_malloc_fast_attempt(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
#if HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1
  void* ptr = hz6_allocator_midpage_trusted_class_reuse_enabled(class_id)
                  ? hz6_allocator_midpage_direct_local_reuse_trusted_class(
                        allocator, HZ6_FRONT_MIDPAGE, class_id, size,
                        &descriptor)
                  : hz6_allocator_direct_local_reuse(allocator,
                                                     HZ6_FRONT_MIDPAGE,
                                                     class_id, size,
                                                     &descriptor);
#else
  void* ptr = hz6_allocator_direct_local_reuse(allocator, HZ6_FRONT_MIDPAGE,
                                               class_id, size,
                                               &descriptor);
#endif
  if (ptr) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_direct;
#endif
    hz6_midpage_active_map_register(allocator, HZ6_FRONT_MIDPAGE,
                                    class_id, ptr, descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(
        allocator, HZ6_FRONT_MIDPAGE, class_id);
    return ptr;
  }
  hz6_allocator_remote_pending_note_frontcache_miss(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
#if HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1
  ptr = hz6_allocator_preload_malloc_transfer_retry(
      allocator, HZ6_FRONT_MIDPAGE, class_id, &descriptor);
  if (ptr) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_direct;
#endif
    hz6_midpage_active_map_register(allocator, HZ6_FRONT_MIDPAGE,
                                    class_id, ptr, descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(
        allocator, HZ6_FRONT_MIDPAGE, class_id);
    return ptr;
  }
#endif

  hz6_toy_small_hotpath_diag_malloc_front_dispatch(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
  hz6_allocator_remote_pending_note_front_dispatch(
      allocator, HZ6_FRONT_MIDPAGE, class_id);
#if HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1
  ptr = hz6_midpage_alloc_with_descriptor(allocator, class_id, size,
                                          &descriptor);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }
  if (descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_front_alloc;
#endif
    hz6_midpage_active_map_register(allocator, HZ6_FRONT_MIDPAGE,
                                    class_id, ptr, descriptor);
  } else {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_route_fallback;
#endif
    hz6_midpage_active_map_register_route(allocator, HZ6_FRONT_MIDPAGE,
                                          class_id, ptr);
  }
  return ptr;
#else
  ptr = hz6_midpage_alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.midpage_active_map_register_route_fallback;
#endif
  hz6_midpage_active_map_register_route(allocator, HZ6_FRONT_MIDPAGE, class_id,
                                        ptr);
  return ptr;
#endif
}
#else
void* hz6_allocator_preload_midpage_malloc_class_skip_transfer(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t size) {
  (void)class_id;
  return hz6_allocator_preload_midpage_malloc_skip_transfer(allocator, size);
}
#endif

#if HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1 && \
    HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 && HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
HZ6_PRELOAD_TOY_DIRECT_CLASS_NOINLINE void*
hz6_allocator_preload_toy_malloc_direct_class(Hz6Allocator* allocator,
                                              size_t size) {
  if (!allocator) {
    return NULL;
  }
#if !HZ6_PRELOAD_BOUNDARY_TRUSTED_OWNER_L1
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }
#endif
  if (size == 0 || size > HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_MAX_BYTES ||
      size > 4096u) {
    return hz6_malloc(allocator, size);
  }

  uint16_t class_id = hz6_allocator_toy_class_for_small_size(size);
  Hz6ObjectDescriptor* descriptor = NULL;
  hz6_toy_small_hotpath_diag_malloc_fast_attempt(
      allocator, HZ6_FRONT_TOY, class_id);
#if HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1
  void* ptr = hz6_allocator_direct_local_reuse(allocator, HZ6_FRONT_TOY,
                                               class_id, size,
                                               &descriptor);
#else
  void* ptr = hz6_allocator_direct_local_alloc(
      allocator, HZ6_FRONT_TOY, class_id, &descriptor);
#endif
  if (ptr) {
    hz6_toy_small_active_map_register(allocator, HZ6_FRONT_TOY, class_id, ptr,
                                      descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(
        allocator, HZ6_FRONT_TOY, class_id);
    return ptr;
  }
  hz6_allocator_remote_pending_note_frontcache_miss(
      allocator, HZ6_FRONT_TOY, class_id);
#if HZ6_PRELOAD_MALLOC_TRANSFER_RETRY_L1
  ptr = hz6_allocator_preload_malloc_transfer_retry(
      allocator, HZ6_FRONT_TOY, class_id, &descriptor);
  if (ptr) {
    hz6_toy_small_active_map_register(allocator, HZ6_FRONT_TOY, class_id, ptr,
                                      descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(
        allocator, HZ6_FRONT_TOY, class_id);
    return ptr;
  }
#endif

  hz6_toy_small_hotpath_diag_malloc_front_dispatch(
      allocator, HZ6_FRONT_TOY, class_id);
  hz6_allocator_remote_pending_note_front_dispatch(
      allocator, HZ6_FRONT_TOY, class_id);
  ptr = hz6_toy_front_ops()->alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
  }
  return ptr;
}
#else
void* hz6_allocator_preload_toy_malloc_direct_class(Hz6Allocator* allocator,
                                                    size_t size) {
  return hz6_malloc(allocator, size);
}
#endif

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  if (!allocator) {
    return NULL;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && \
    HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE_MAINTENANCE_L1
  hz6_shared_route_directory_maintain_tombstones();
#endif
#if HZ6_ROUTE_DOMAIN_SYNC_L1 && HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1
  hz6_allocator_route_maintain_tombstones(allocator);
#endif

  uint16_t class_id = 0;
  const Hz6FrontOps* front = NULL;
#if HZ6_TOY_PRECLASSIFIED_MALLOC_L1
  if (size <= 4096u) {
    class_id = hz6_allocator_toy_class_for_small_size(size);
    front = hz6_toy_front_ops();
  } else
#endif
  {
    front = hz6_front_for_allocation(size, 16, &class_id);
  }
  if (!front || !front->alloc) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }

#if HZ6_SAME_OWNER_FAST_L1 || HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
  Hz6ObjectDescriptor* direct_descriptor = NULL;
  hz6_toy_small_hotpath_diag_malloc_fast_attempt(allocator, front->front_id,
                                                 class_id);
  void* direct_ptr = NULL;
#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
#if HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1 && \
    HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
  if (HZ6_MIDPAGE_SKIP_TRANSFER_UNLIKELY(front->front_id ==
                                         HZ6_FRONT_MIDPAGE)) {
    direct_ptr = hz6_allocator_midpage_direct_local_alloc_skip_transfer(
        allocator, class_id, &direct_descriptor);
  } else
#endif
  {
    direct_ptr = hz6_allocator_direct_local_alloc(
        allocator, front->front_id, class_id, &direct_descriptor);
  }
#endif
#if HZ6_SAME_OWNER_FAST_L1
  if (!direct_ptr) {
    direct_ptr = hz6_allocator_same_owner_fast_alloc_inline(
        allocator, front->front_id, class_id);
  }
#endif
  if (direct_ptr) {
    if (front->front_id == HZ6_FRONT_MIDPAGE) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.midpage_active_map_register_direct;
#endif
      hz6_midpage_active_map_register(
          allocator, front->front_id, class_id, direct_ptr, direct_descriptor);
    }
    hz6_toy_small_active_map_register(
        allocator, front->front_id, class_id, direct_ptr, direct_descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(allocator, front->front_id,
                                               class_id);
    return direct_ptr;
  }
  hz6_allocator_remote_pending_note_frontcache_miss(
      allocator, front->front_id, class_id);
#if HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1 && HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
  if (front->front_id == HZ6_FRONT_MIDPAGE &&
      hz6_midpage_prefill_run(allocator, class_id) != 0) {
    direct_descriptor = NULL;
    direct_ptr = hz6_allocator_direct_local_alloc(
        allocator, front->front_id, class_id, &direct_descriptor);
    if (direct_ptr) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.midpage_active_map_register_direct;
#endif
      hz6_midpage_active_map_register(
          allocator, front->front_id, class_id, direct_ptr,
          direct_descriptor);
      hz6_toy_small_hotpath_diag_malloc_fast_hit(allocator, front->front_id,
                                                 class_id);
      return direct_ptr;
    }
  }
#endif
#endif

  hz6_toy_small_hotpath_diag_malloc_front_dispatch(allocator, front->front_id,
                                                   class_id);
  hz6_allocator_remote_pending_note_front_dispatch(
      allocator, front->front_id, class_id);
#if HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1
  if (front->front_id == HZ6_FRONT_MIDPAGE) {
    Hz6ObjectDescriptor* midpage_descriptor = NULL;
    void* midpage_ptr = hz6_midpage_alloc_with_descriptor(
        allocator, class_id, size, &midpage_descriptor);
    if (!midpage_ptr) {
      ++allocator->stats.alloc_fail;
      return NULL;
    }
    if (midpage_descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.midpage_active_map_register_front_alloc;
#endif
      hz6_midpage_active_map_register(allocator, front->front_id, class_id,
                                      midpage_ptr, midpage_descriptor);
    } else {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.midpage_active_map_register_route_fallback;
#endif
      hz6_midpage_active_map_register_route(allocator, front->front_id,
                                            class_id, midpage_ptr);
    }
    return midpage_ptr;
  }
#endif
  void* ptr = front->alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
  } else if (front->front_id == HZ6_FRONT_MIDPAGE) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register_route_fallback;
#endif
    hz6_midpage_active_map_register_route(
        allocator, front->front_id, class_id, ptr);
  }
  return ptr;
}
