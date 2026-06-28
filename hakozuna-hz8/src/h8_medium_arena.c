#include "h8_internal.h"
#include "h8_medium.h"

#if defined(H8_MEDIUM_CHUNK_CARVE) || defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
#define H8_MEDIUM_CHUNK_BYTES (16u * 1024u * 1024u)
#endif

#if defined(H8_MEDIUM_CHUNK_CARVE) || defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
typedef struct H8MediumChunk {
  uint8_t* base;
  void* raw_base;
  size_t raw_bytes;
  size_t used;
  struct H8MediumChunk* next;
} H8MediumChunk;

static h8_platform_mutex_t h8_medium_arena_lock = H8_PLATFORM_MUTEX_INIT;
static H8MediumChunk* h8_medium_chunks;
#endif

#if defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
typedef struct H8MediumArenaShard {
  H8MediumChunk* current;
  size_t used;
} H8MediumArenaShard;

static H8MediumArenaShard h8_medium_shards[H8_OWNER_MAX];
#endif

typedef struct H8MediumAlignedReserve {
  void* aligned_base;
  void* raw_base;
  size_t raw_bytes;
} H8MediumAlignedReserve;

static H8MediumAlignedReserve h8_medium_reserve_aligned(size_t size,
                                                        size_t align) {
  H8MediumAlignedReserve result = {0};
  /*
   * Reserve an over-aligned window, then commit only the aligned payload
   * interval. This keeps Win64 and POSIX aligned-payload contracts symmetric:
   * the outer reservation stays exact for release while the inner committed
   * range stays tight for RSS.
   */
  if (size == 0 || align == 0) {
    return result;
  }
  size_t reserve = size + align;
  void* raw = h8_platform_reserve(reserve);
  if (!raw) {
    return result;
  }
  uintptr_t aligned =
      ((uintptr_t)raw + (uintptr_t)align - 1u) & ~((uintptr_t)align - 1u);
  if (h8_platform_commit((void*)aligned, size) != 0) {
    h8_platform_release(raw, reserve);
    return result;
  }
  result.aligned_base = (void*)aligned;
  result.raw_base = raw;
  result.raw_bytes = reserve;
  return result;
}

#if defined(H8_MEDIUM_CHUNK_CARVE) || defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
static H8MediumChunk* h8_medium_chunk_create(void) {
  H8MediumAlignedReserve reserve = h8_medium_reserve_aligned(
      H8_MEDIUM_CHUNK_BYTES, H8_MEDIUM_CHUNK_BYTES);
  if (!reserve.aligned_base) {
    return NULL;
  }
  H8MediumChunk* chunk = h8_sys_calloc(1, sizeof(*chunk));
  if (!chunk) {
    h8_platform_release(reserve.raw_base, reserve.raw_bytes);
    return NULL;
  }
  chunk->base = reserve.aligned_base;
  chunk->raw_base = reserve.raw_base;
  chunk->raw_bytes = reserve.raw_bytes;
  H8_DEBUG_INC(medium_chunk_create_count);
  H8_DEBUG_ADD(medium_chunk_reserved_bytes, H8_MEDIUM_CHUNK_BYTES);
  return chunk;
}
#endif

#if defined(H8_MEDIUM_CHUNK_CARVE) || defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
static void* h8_medium_payload_alloc_global_locked(size_t run_size,
                                                   bool* chunk_backed_out) {
  if (run_size == 0 || run_size > H8_MEDIUM_CHUNK_BYTES ||
      (run_size & (H8_MEDIUM_QUANTUM_BYTES - 1u)) != 0) {
    return NULL;
  }
  h8_platform_mutex_lock(&h8_medium_arena_lock);
  for (H8MediumChunk* chunk = h8_medium_chunks; chunk; chunk = chunk->next) {
    size_t used = h8_round_up_size(chunk->used, run_size);
    if (used + run_size <= H8_MEDIUM_CHUNK_BYTES) {
      chunk->used = used + run_size;
      h8_platform_mutex_unlock(&h8_medium_arena_lock);
      if (chunk_backed_out) {
        *chunk_backed_out = true;
      }
      H8_DEBUG_INC(medium_chunk_alloc_count);
      H8_DEBUG_ADD(medium_chunk_used_bytes, run_size);
      return chunk->base + used;
    }
  }
  H8MediumChunk* chunk = h8_medium_chunk_create();
  if (!chunk) {
    h8_platform_mutex_unlock(&h8_medium_arena_lock);
    return NULL;
  }
  chunk->next = h8_medium_chunks;
  h8_medium_chunks = chunk;
  chunk->used = run_size;
  h8_platform_mutex_unlock(&h8_medium_arena_lock);
  if (chunk_backed_out) {
    *chunk_backed_out = true;
  }
  H8_DEBUG_INC(medium_chunk_alloc_count);
  H8_DEBUG_ADD(medium_chunk_used_bytes, run_size);
  return chunk->base;
}
#endif

#if defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
static bool h8_medium_current_owner_slot(uint32_t* slot_out) {
  H8ThreadCtx* ctx = h8_tls_ctx;
  if (!ctx || !ctx->owner || ctx->owner->slot >= H8_OWNER_MAX) {
    return false;
  }
  *slot_out = ctx->owner->slot;
  return true;
}

static H8MediumChunk* h8_medium_chunk_refill(void) {
  H8MediumChunk* chunk = h8_medium_chunk_create();
  if (!chunk) {
    return NULL;
  }
  h8_platform_mutex_lock(&h8_medium_arena_lock);
  chunk->next = h8_medium_chunks;
  h8_medium_chunks = chunk;
  h8_platform_mutex_unlock(&h8_medium_arena_lock);
  return chunk;
}

static void* h8_medium_payload_alloc_sharded(size_t run_size,
                                             bool* chunk_backed_out) {
  if (run_size == 0 || run_size > H8_MEDIUM_CHUNK_BYTES ||
      (run_size & (H8_MEDIUM_QUANTUM_BYTES - 1u)) != 0) {
    return NULL;
  }
  uint32_t owner_slot = 0;
  if (!h8_medium_current_owner_slot(&owner_slot)) {
    return h8_medium_payload_alloc_global_locked(run_size, chunk_backed_out);
  }
  H8MediumArenaShard* shard = &h8_medium_shards[owner_slot];
  size_t used = h8_round_up_size(shard->used, run_size);
  H8MediumChunk* chunk = shard->current;
  if (!chunk || used + run_size > H8_MEDIUM_CHUNK_BYTES) {
    chunk = h8_medium_chunk_refill();
    if (!chunk) {
      return NULL;
    }
    shard->current = chunk;
    used = 0;
  }
  shard->used = used + run_size;
  chunk->used = shard->used;
  if (chunk_backed_out) {
    *chunk_backed_out = true;
  }
  H8_DEBUG_INC(medium_chunk_alloc_count);
  H8_DEBUG_ADD(medium_chunk_used_bytes, run_size);
  return chunk->base + used;
}
#endif

void* h8_medium_payload_alloc(size_t run_size, bool* chunk_backed_out,
                              void** raw_base_out, size_t* raw_bytes_out) {
  if (chunk_backed_out) {
    *chunk_backed_out = false;
  }
  if (raw_base_out) {
    *raw_base_out = NULL;
  }
  if (raw_bytes_out) {
    *raw_bytes_out = 0;
  }
#if defined(H8_MEDIUM_CHUNK_SHARDED_CARVE)
  return h8_medium_payload_alloc_sharded(run_size, chunk_backed_out);
#elif defined(H8_MEDIUM_CHUNK_CARVE)
  return h8_medium_payload_alloc_global_locked(run_size, chunk_backed_out);
#else
  if (run_size == 0) {
    return NULL;
  }
  H8MediumAlignedReserve reserve =
      h8_medium_reserve_aligned(run_size, run_size);
  if (raw_base_out) {
    *raw_base_out = reserve.raw_base;
  }
  if (raw_bytes_out) {
    *raw_bytes_out = reserve.raw_bytes;
  }
  return reserve.aligned_base;
#endif
}

void h8_medium_payload_free(void* raw_base, size_t raw_bytes, bool chunk_backed) {
  if (!raw_base || raw_bytes == 0) {
    return;
  }
  if (!chunk_backed) {
    h8_platform_release(raw_base, raw_bytes);
  }
}
