#ifndef HZ12_H
#define HZ12_H

#include <stddef.h>
#include <stdint.h>

/* HZ12ThreadCacheFastPath-L0 public API.
 *
 * L0 is a system-allocator-backed front-cache SHIM, not the HZ12 allocator
 * body. malloc/free use a per-thread object cache + pointer->class token table;
 * everything else (calloc/realloc/usable_size/align*, large allocs, token-miss
 * frees) falls through to the system allocator (dlsym RTLD_NEXT). See
 * docs/HZ12_THREAD_CACHE_FAST_PATH_L0.md.
 *
 * Speed mode (default): assumes a valid C program; no double-free / interior /
 * stale detection. Tokens are set on every malloc handout and NOT deleted on
 * free, so stale tokens are expected in stats. */

void* hz12_malloc(size_t size);
void hz12_free(void* ptr);
void* hz12_calloc(size_t count, size_t size);
void* hz12_realloc(void* ptr, size_t size);
size_t hz12_malloc_usable_size(void* ptr);
int hz12_posix_memalign(void** memptr, size_t alignment, size_t size);
void* hz12_aligned_alloc(size_t alignment, size_t size);
void* hz12_memalign(size_t alignment, size_t size);

typedef struct H12Stats {
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
  /* HZ12TransferCacheCentralSpan-L1 counters */
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
  /* HZ12CentralFreeListSpanReturn-L1 counters */
  uint64_t span_return_count;
  uint64_t span_reuse_count;
  uint64_t central_full_span_count;
  uint64_t central_partial_span_count;
  uint64_t central_objects;
  /* Windows/span returned-object attribution counters. These stay zero unless
   * built with the opt-in diagnostic flag. */
  uint64_t returned_push;
  uint64_t returned_pop_hit;
  uint64_t returned_pop_miss;
} H12Stats;

void hz12_stats(H12Stats* out);

#endif /* HZ12_H */
