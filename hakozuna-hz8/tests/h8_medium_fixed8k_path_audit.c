#include "../include/h8.h"

#include <stdio.h>

static size_t delta(size_t after, size_t before) {
  return after >= before ? after - before : 0u;
}

int main(void) {
  const size_t warmup = 4096u;
  const size_t iterations = 250000u;
  h8_init();

  for (size_t i = 0; i < warmup; ++i) {
    void* ptr = h8_malloc(8192u);
    if (!ptr) {
      fprintf(stderr, "fixed8k path audit: warmup allocation failed\n");
      return 1;
    }
    h8_free(ptr);
  }

  H8DebugStats before = h8_debug_stats();
  for (size_t i = 0; i < iterations; ++i) {
    void* ptr = h8_malloc(8192u);
    if (!ptr) {
      fprintf(stderr, "fixed8k path audit: allocation failed at %zu\n", i);
      return 1;
    }
    h8_free(ptr);
  }
  H8DebugStats after = h8_debug_stats();

  size_t allocs = delta(after.medium_malloc_count, before.medium_malloc_count);
  size_t frees = delta(after.medium_free_scaffold_count,
                       before.medium_free_scaffold_count);
  size_t active_hits = delta(after.medium_active_alloc_hit_count,
                             before.medium_active_alloc_hit_count);
  size_t owner_free_hits = delta(after.medium_local_free_owner_match,
                                 before.medium_local_free_owner_match);
  if (allocs != iterations || frees != iterations ||
      active_hits == 0u || owner_free_hits != iterations) {
    fprintf(stderr,
            "fixed8k path audit: unexpected path allocs=%zu frees=%zu "
            "active_hits=%zu owner_free_hits=%zu\n",
            allocs, frees, active_hits, owner_free_hits);
    return 1;
  }

  printf("fixed8k_path_audit iterations=%zu allocs=%zu frees=%zu "
         "active_hits=%zu owner_free_hits=%zu active_miss=%zu "
         "alloc_precheck_ns=%zu alloc_mark_live_ns=%zu "
         "alloc_mask_ns=%zu alloc_slot_store_ns=%zu alloc_ptr_ns=%zu "
         "free_decode_ns=%zu free_state_ns=%zu free_pending_ns=%zu "
         "free_slot_store_ns=%zu free_mask_ns=%zu free_empty_ns=%zu\n",
         iterations, allocs, frees, active_hits, owner_free_hits,
         delta(after.medium_active_miss_unusable,
               before.medium_active_miss_unusable),
         delta(after.medium_active_alloc_precheck_ns,
               before.medium_active_alloc_precheck_ns),
         delta(after.medium_active_alloc_mark_live_ns,
               before.medium_active_alloc_mark_live_ns),
         delta(after.medium_active_alloc_mask_ns,
               before.medium_active_alloc_mask_ns),
         delta(after.medium_active_alloc_slot_store_ns,
               before.medium_active_alloc_slot_store_ns),
         delta(after.medium_active_alloc_ptr_ns,
               before.medium_active_alloc_ptr_ns),
         delta(after.medium_free_decode_ns, before.medium_free_decode_ns),
         delta(after.medium_free_state_ns, before.medium_free_state_ns),
         delta(after.medium_free_pending_ns, before.medium_free_pending_ns),
         delta(after.medium_free_slot_store_ns,
               before.medium_free_slot_store_ns),
         delta(after.medium_free_mask_ns, before.medium_free_mask_ns),
         delta(after.medium_free_empty_ns, before.medium_free_empty_ns));
  return 0;
}
