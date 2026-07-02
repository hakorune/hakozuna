#include "h8_internal.h"

#if defined(H9_MEDIUM_LOCAL_SLAB_SHADOW)

#if !defined(H9_LOCAL_PAGE_SHADOW_CAP_BYTES)
#define H9_LOCAL_PAGE_SHADOW_CAP_BYTES (512u * 1024u)
#endif

#if !defined(H9_LOCAL_PAGE_SHADOW_PAGES_PER_CLASS)
#define H9_LOCAL_PAGE_SHADOW_PAGES_PER_CLASS 1u
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static void h9_slab_shadow_class_inc(atomic_size_t counters[6],
                                     uint32_t class_id) {
  if (class_id < 6u) {
    atomic_fetch_add_explicit(&counters[class_id], 1u, memory_order_relaxed);
  }
}
#endif

static size_t h9_slab_shadow_slot_bytes(uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  return spec ? (size_t)spec->slot_size : 0u;
}

static uint16_t h9_slab_shadow_capacity(uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_count == 0u) {
    return 0u;
  }
  size_t capacity =
      (size_t)spec->slot_count * (size_t)H9_LOCAL_PAGE_SHADOW_PAGES_PER_CLASS;
  return capacity > UINT16_MAX ? UINT16_MAX : (uint16_t)capacity;
}

#if defined(H8_ENABLE_DEBUG_STATS)
static void h9_slab_shadow_note_peak(size_t value) {
  size_t old = atomic_load_explicit(&h8g.h9_slab_shadow_retained_peak_bytes,
                                    memory_order_relaxed);
  while (old < value &&
         !atomic_compare_exchange_weak_explicit(
             &h8g.h9_slab_shadow_retained_peak_bytes, &old, value,
             memory_order_relaxed, memory_order_relaxed)) {
  }
}
#endif

static bool h9_slab_shadow_try_cache_bytes(H8ThreadCtx* ctx, size_t bytes) {
  if (bytes == 0u) {
    return false;
  }
  if (bytes > (size_t)H9_LOCAL_PAGE_SHADOW_CAP_BYTES) {
    return false;
  }
  if (ctx->h9_slab_shadow_cached_bytes >
      (size_t)H9_LOCAL_PAGE_SHADOW_CAP_BYTES - bytes) {
    return false;
  }
  ctx->h9_slab_shadow_cached_bytes += bytes;
#if defined(H8_ENABLE_DEBUG_STATS)
  size_t current = atomic_fetch_add_explicit(
                       &h8g.h9_slab_shadow_retained_bytes, bytes,
                       memory_order_relaxed) +
                   bytes;
  h9_slab_shadow_note_peak(current);
#endif
  return true;
}

static void h9_slab_shadow_drop_cached_bytes(H8ThreadCtx* ctx, size_t bytes) {
  if (bytes == 0u) {
    return;
  }
  size_t dropped = bytes;
  if (ctx->h9_slab_shadow_cached_bytes >= bytes) {
    ctx->h9_slab_shadow_cached_bytes -= bytes;
  } else {
    dropped = ctx->h9_slab_shadow_cached_bytes;
    ctx->h9_slab_shadow_cached_bytes = 0u;
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  if (dropped != 0u) {
    atomic_fetch_sub_explicit(&h8g.h9_slab_shadow_retained_bytes, dropped,
                              memory_order_relaxed);
  }
#else
  (void)dropped;
#endif
}

void h9_slab_shadow_note_alloc(H8ThreadCtx* ctx, uint32_t class_id) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8_DEBUG_INC(h9_slab_shadow_alloc_eligible);
#if defined(H8_ENABLE_DEBUG_STATS)
  h9_slab_shadow_class_inc(h8g.h9_slab_shadow_alloc_class, class_id);
#endif
  uint16_t* free_count = &ctx->h9_slab_shadow_free_count[class_id];
  if (*free_count == 0u) {
    H8_DEBUG_INC(h9_slab_shadow_page_switch);
    return;
  }
  --(*free_count);
  H8_DEBUG_INC(h9_slab_shadow_alloc_hit_possible);
#if defined(H8_ENABLE_DEBUG_STATS)
  h9_slab_shadow_class_inc(h8g.h9_slab_shadow_hit_class, class_id);
#endif
  if (*free_count == 0u) {
    H8_DEBUG_INC(h9_slab_shadow_page_empty);
  }
  size_t bytes = h9_slab_shadow_slot_bytes(class_id);
  h9_slab_shadow_drop_cached_bytes(ctx, bytes);
}

void h9_slab_shadow_note_local_free(H8ThreadCtx* ctx, H8MediumRun* run,
                                    size_t slot) {
  if (!ctx) {
    return;
  }
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT || slot >= run->slot_count) {
    H8_DEBUG_INC(h9_slab_shadow_invalid_owned_seen);
    return;
  }
  uint64_t bit = UINT64_C(1) << slot;
  uint64_t pending =
      atomic_load_explicit(&run->pending_bits[slot >> 6u], memory_order_acquire);
  if ((pending & bit) != 0u) {
    H8_DEBUG_INC(h9_slab_shadow_remote_pending_required);
    return;
  }
  H8_DEBUG_INC(h9_slab_shadow_free_eligible);
  uint32_t class_id = run->class_id;
#if defined(H8_ENABLE_DEBUG_STATS)
  h9_slab_shadow_class_inc(h8g.h9_slab_shadow_free_class, class_id);
#endif
  uint16_t cap = h9_slab_shadow_capacity(class_id);
  uint16_t* free_count = &ctx->h9_slab_shadow_free_count[class_id];
  if (cap == 0u || *free_count >= cap) {
    H8_DEBUG_INC(h9_slab_shadow_page_full);
    return;
  }
  size_t bytes = h9_slab_shadow_slot_bytes(class_id);
  if (!h9_slab_shadow_try_cache_bytes(ctx, bytes)) {
    H8_DEBUG_INC(h9_slab_shadow_cap_fallback);
    return;
  }
  ++(*free_count);
  if (*free_count == cap) {
    H8_DEBUG_INC(h9_slab_shadow_page_full);
  }
}

void h9_slab_shadow_note_remote_escape(H8ThreadCtx* ctx, H8MediumRun* run) {
  (void)ctx;
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    H8_DEBUG_INC(h9_slab_shadow_invalid_owned_seen);
    return;
  }
  H8_DEBUG_INC(h9_slab_shadow_remote_escape);
#if defined(H8_ENABLE_DEBUG_STATS)
  h9_slab_shadow_class_inc(h8g.h9_slab_shadow_remote_class, run->class_id);
#endif
}

void h9_slab_shadow_flush_all(H8ThreadCtx* ctx) {
  if (!ctx) {
    return;
  }
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    ctx->h9_slab_shadow_free_count[c] = 0u;
  }
  size_t bytes = ctx->h9_slab_shadow_cached_bytes;
  ctx->h9_slab_shadow_cached_bytes = 0u;
#if defined(H8_ENABLE_DEBUG_STATS)
  if (bytes != 0u) {
    atomic_fetch_sub_explicit(&h8g.h9_slab_shadow_retained_bytes, bytes,
                              memory_order_relaxed);
  }
#else
  (void)bytes;
#endif
}

#else
typedef int h9_slab_shadow_translation_unit_silence;
#endif
