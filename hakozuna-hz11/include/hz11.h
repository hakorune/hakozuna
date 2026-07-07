#ifndef HZ11_H
#define HZ11_H

#include <stddef.h>
#include <stdint.h>

/* HZ11ThreadCacheFastPath-L0 public API.
 *
 * L0 is a system-allocator-backed front-cache SHIM, not the HZ11 allocator
 * body. malloc/free use a per-thread object cache + pointer->class token table;
 * everything else (calloc/realloc/usable_size/align*, large allocs, token-miss
 * frees) falls through to the system allocator (dlsym RTLD_NEXT). See
 * docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md.
 *
 * Speed mode (default): assumes a valid C program; no double-free / interior /
 * stale detection. Tokens are set on every malloc handout and NOT deleted on
 * free, so stale tokens are expected in stats. */

void* hz11_malloc(size_t size);
void hz11_free(void* ptr);
void* hz11_calloc(size_t count, size_t size);
void* hz11_realloc(void* ptr, size_t size);
size_t hz11_malloc_usable_size(void* ptr);
int hz11_posix_memalign(void** memptr, size_t alignment, size_t size);
void* hz11_aligned_alloc(size_t alignment, size_t size);
void* hz11_memalign(size_t alignment, size_t size);

typedef struct H11Stats {
  uint64_t malloc_count;
  uint64_t malloc_hit;
  uint64_t refill_count;
  uint64_t free_count;
  uint64_t token_hit;
  uint64_t token_miss;
  uint64_t direct_hit_count;
  uint64_t direct_miss_count;
  uint64_t span_create_count;
  uint64_t overflow_count;
  uint64_t flush_count;
  uint64_t flush_items;
  size_t cached_bytes;
  /* HZ11TransferCacheCentralSpan-L1 counters */
  uint64_t refill_from_transfer;
  uint64_t refill_from_central;
  uint64_t refill_from_span;
  uint64_t transfer_remove_hit;
  uint64_t transfer_remove_miss;
  uint64_t transfer_insert;
  uint64_t transfer_insert_spill;
  uint64_t central_remove_hit;
  uint64_t central_remove_miss;
  uint64_t central_insert;
} H11Stats;

void hz11_stats(H11Stats* out);

#endif /* HZ11_H */
