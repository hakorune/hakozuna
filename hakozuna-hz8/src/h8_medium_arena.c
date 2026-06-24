#include "h8_internal.h"
#include "h8_medium.h"

#include <sys/mman.h>

#if defined(H8_MEDIUM_CHUNK_CARVE)
#define H8_MEDIUM_CHUNK_BYTES (16u * 1024u * 1024u)
#endif

#if defined(H8_MEDIUM_CHUNK_CARVE)
typedef struct H8MediumChunk {
  uint8_t* base;
  size_t used;
  struct H8MediumChunk* next;
} H8MediumChunk;

static pthread_mutex_t h8_medium_arena_lock = PTHREAD_MUTEX_INITIALIZER;
static H8MediumChunk* h8_medium_chunks;
#endif

static void* h8_medium_mmap_aligned(size_t size, size_t align) {
  size_t reserve = size + align;
  uint8_t* raw = mmap(NULL, reserve, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS,
                      -1, 0);
  if (raw == MAP_FAILED) {
    return MAP_FAILED;
  }
  uintptr_t aligned =
      ((uintptr_t)raw + align - 1u) & ~(uintptr_t)(align - 1u);
  size_t prefix = (size_t)((uint8_t*)aligned - raw);
  size_t suffix = reserve - prefix - size;
  if (prefix != 0) {
    munmap(raw, prefix);
  }
  if (suffix != 0) {
    munmap((uint8_t*)aligned + size, suffix);
  }
  if (mprotect((void*)aligned, size, PROT_READ | PROT_WRITE) != 0) {
    munmap((void*)aligned, size);
    return MAP_FAILED;
  }
  return (void*)aligned;
}

#if defined(H8_MEDIUM_CHUNK_CARVE)
static H8MediumChunk* h8_medium_chunk_create(void) {
  void* payload =
      h8_medium_mmap_aligned(H8_MEDIUM_CHUNK_BYTES, H8_MEDIUM_CHUNK_BYTES);
  if (payload == MAP_FAILED) {
    return NULL;
  }
  H8MediumChunk* chunk = h8_sys_calloc(1, sizeof(*chunk));
  if (!chunk) {
    munmap(payload, H8_MEDIUM_CHUNK_BYTES);
    return NULL;
  }
  chunk->base = payload;
  return chunk;
}
#endif

void* h8_medium_payload_alloc(size_t run_size, bool* chunk_backed_out) {
  if (chunk_backed_out) {
    *chunk_backed_out = false;
  }
#if defined(H8_MEDIUM_CHUNK_CARVE)
  if (run_size == 0 || run_size > H8_MEDIUM_CHUNK_BYTES ||
      (run_size & (H8_MEDIUM_QUANTUM_BYTES - 1u)) != 0) {
    return NULL;
  }
  pthread_mutex_lock(&h8_medium_arena_lock);
  for (H8MediumChunk* chunk = h8_medium_chunks; chunk; chunk = chunk->next) {
    size_t used = h8_round_up_size(chunk->used, run_size);
    if (used + run_size <= H8_MEDIUM_CHUNK_BYTES) {
      chunk->used = used + run_size;
      pthread_mutex_unlock(&h8_medium_arena_lock);
      if (chunk_backed_out) {
        *chunk_backed_out = true;
      }
      return chunk->base + used;
    }
  }
  H8MediumChunk* chunk = h8_medium_chunk_create();
  if (!chunk) {
    pthread_mutex_unlock(&h8_medium_arena_lock);
    return NULL;
  }
  chunk->next = h8_medium_chunks;
  h8_medium_chunks = chunk;
  chunk->used = run_size;
  pthread_mutex_unlock(&h8_medium_arena_lock);
  if (chunk_backed_out) {
    *chunk_backed_out = true;
  }
  return chunk->base;
#else
  if (run_size == 0) {
    return NULL;
  }
  void* payload = h8_medium_mmap_aligned(run_size, run_size);
  return payload == MAP_FAILED ? NULL : payload;
#endif
}

void h8_medium_payload_free(void* ptr, size_t run_size, bool chunk_backed) {
  if (!ptr || run_size == 0) {
    return;
  }
  if (!chunk_backed) {
    munmap(ptr, run_size);
  }
}
