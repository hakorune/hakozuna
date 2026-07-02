#include "../include/h8.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { H8_STRESS_PTRS = 4096, H8_STRESS_THREADS = 4 };

static void* g_ptrs[H8_STRESS_PTRS];

static int debug_hard_gates_clean(const char* label) {
  H8DebugStats d = h8_debug_stats();
  if (d.remote_stage_validate_fail != 0 ||
      d.quiescent_pending_bitmap_nonzero != 0 ||
      d.quiescent_pending_repair != 0 ||
      d.local_alloc_pending_nonzero != 0 ||
      d.local_free_pending_nonzero != 0 ||
      d.owner_live_set_already_live != 0 ||
      d.owner_live_clear_already_free != 0 ||
      d.slot_shadow_valid_mismatch != 0 ||
      d.slot_shadow_invalid_mismatch != 0 ||
      d.slot_shadow_pending_nonallocated != 0 ||
      d.slot_shadow_free_unreachable != 0 ||
      d.slot_shadow_free_duplicate != 0 ||
      d.slot_shadow_free_cycle != 0 ||
      d.slot_shadow_bad_next != 0 ||
      d.slot_shadow_never_used_below_bump != 0 ||
      d.slot_shadow_nonvirgin_above_bump != 0 ||
      d.slot_shadow_used_mismatch != 0 ||
      d.slot_shadow_reserved_quiescent != 0 ||
      d.medium_invalid_owned_count != 0 ||
      d.medium_active_alloc_owner_mismatch != 0 ||
      d.medium_owner_list_owner_mismatch != 0 ||
      d.medium_route_authority_mismatch != 0 ||
      d.medium_attached_foreign_mask_writer != 0 ||
      d.medium_owner_token_changed_during_mutation != 0 ||
      d.medium_collect_wrong_owner != 0 ||
      d.medium_detached_direct_free_while_attached != 0) {
    fprintf(stderr, "%s: hard gate failed\n", label);
    fprintf(stderr,
            "validate=%zu qpending=%zu qrepair=%zu local_pending=%zu/%zu "
            "live=%zu/%zu slot_mismatch=%zu/%zu/%zu free=%zu/%zu/%zu/%zu "
            "bump=%zu/%zu used=%zu reserved=%zu medium=%zu/%zu/%zu/%zu "
            "writer=%zu/%zu/%zu/%zu\n",
            d.remote_stage_validate_fail,
            d.quiescent_pending_bitmap_nonzero,
            d.quiescent_pending_repair,
            d.local_alloc_pending_nonzero,
            d.local_free_pending_nonzero,
            d.owner_live_set_already_live,
            d.owner_live_clear_already_free,
            d.slot_shadow_valid_mismatch,
            d.slot_shadow_invalid_mismatch,
            d.slot_shadow_pending_nonallocated,
            d.slot_shadow_free_unreachable,
            d.slot_shadow_free_duplicate,
            d.slot_shadow_free_cycle,
            d.slot_shadow_bad_next,
            d.slot_shadow_never_used_below_bump,
            d.slot_shadow_nonvirgin_above_bump,
            d.slot_shadow_used_mismatch,
            d.slot_shadow_reserved_quiescent,
            d.medium_invalid_owned_count,
            d.medium_active_alloc_owner_mismatch,
            d.medium_owner_list_owner_mismatch,
            d.medium_route_authority_mismatch,
            d.medium_attached_foreign_mask_writer,
            d.medium_owner_token_changed_during_mutation,
            d.medium_collect_wrong_owner,
            d.medium_detached_direct_free_while_attached);
    return 0;
  }
  return 1;
}

static int interior_invalid_stress(void) {
  static const size_t sizes[] = {16, 32, 128, 1536, 2048, 4096};
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
    size_t size = sizes[i];
    void* p = h8_malloc(size);
    if (!p) {
      fprintf(stderr, "interior: alloc failed size=%zu\n", size);
      return 0;
    }
    memset(p, 0xA7, size);
    if (h8_route(p) != H8_ROUTE_VALID ||
        h8_route((char*)p + 1) != H8_ROUTE_INVALID ||
        h8_route((char*)p + size - 1) != H8_ROUTE_INVALID) {
      fprintf(stderr, "interior: route mismatch size=%zu\n", size);
      return 0;
    }
    h8_free((char*)p + 1);
    if (h8_route(p) != H8_ROUTE_VALID) {
      fprintf(stderr, "interior: invalid free consumed base size=%zu\n", size);
      return 0;
    }
    h8_free(p);
    if (h8_route(p) == H8_ROUTE_VALID) {
      fprintf(stderr, "interior: base remained valid size=%zu\n", size);
      return 0;
    }
  }
  return 1;
}

static void* owner_exit_source(void* arg) {
  (void)arg;
  for (size_t i = 0; i < H8_STRESS_PTRS; ++i) {
    size_t size = 16u << (i % 8u);
    g_ptrs[i] = h8_malloc(size);
    if (!g_ptrs[i]) {
      return (void*)1;
    }
    memset(g_ptrs[i], (int)(0x30u + (i & 31u)), size);
  }
  return NULL;
}

typedef struct FreeRange {
  size_t begin;
  size_t end;
} FreeRange;

static void* free_range_worker(void* arg) {
  FreeRange* range = (FreeRange*)arg;
  for (size_t i = range->begin; i < range->end; ++i) {
    h8_free(g_ptrs[i]);
  }
  return NULL;
}

static int owner_exit_foreign_free_stress(void) {
  pthread_t src;
  if (pthread_create(&src, NULL, owner_exit_source, NULL) != 0) {
    perror("pthread_create source");
    return 0;
  }
  void* rc = NULL;
  if (pthread_join(src, &rc) != 0 || rc != NULL) {
    fprintf(stderr, "source failed\n");
    return 0;
  }
  for (size_t i = 0; i < H8_STRESS_PTRS; ++i) {
    if (h8_route(g_ptrs[i]) != H8_ROUTE_VALID) {
      fprintf(stderr, "post-exit route invalid index=%zu\n", i);
      return 0;
    }
  }

  pthread_t tids[H8_STRESS_THREADS];
  FreeRange ranges[H8_STRESS_THREADS];
  size_t step = H8_STRESS_PTRS / H8_STRESS_THREADS;
  for (size_t t = 0; t < H8_STRESS_THREADS; ++t) {
    ranges[t].begin = t * step;
    ranges[t].end = (t + 1u == H8_STRESS_THREADS) ? H8_STRESS_PTRS : (t + 1u) * step;
    if (pthread_create(&tids[t], NULL, free_range_worker, &ranges[t]) != 0) {
      perror("pthread_create freer");
      return 0;
    }
  }
  for (size_t t = 0; t < H8_STRESS_THREADS; ++t) {
    if (pthread_join(tids[t], NULL) != 0) {
      perror("pthread_join freer");
      return 0;
    }
  }
  for (size_t i = 0; i < H8_STRESS_PTRS; ++i) {
    if (h8_route(g_ptrs[i]) == H8_ROUTE_VALID) {
      fprintf(stderr, "post-foreign-free route valid index=%zu\n", i);
      return 0;
    }
  }
  return 1;
}

static void* duplicate_free_worker(void* arg) {
  h8_free(arg);
  return NULL;
}

static int remote_duplicate_stress(void) {
  pthread_t src;
  if (pthread_create(&src, NULL, owner_exit_source, NULL) != 0) {
    perror("pthread_create duplicate source");
    return 0;
  }
  void* rc = NULL;
  if (pthread_join(src, &rc) != 0 || rc != NULL) {
    fprintf(stderr, "duplicate source failed\n");
    return 0;
  }
  void* p = g_ptrs[0];
  pthread_t a;
  pthread_t b;
  if (pthread_create(&a, NULL, duplicate_free_worker, p) != 0 ||
      pthread_create(&b, NULL, duplicate_free_worker, p) != 0) {
    perror("pthread_create duplicate freer");
    return 0;
  }
  pthread_join(a, NULL);
  pthread_join(b, NULL);
  if (h8_route(p) == H8_ROUTE_VALID) {
    fprintf(stderr, "duplicate free left pointer valid\n");
    return 0;
  }
  for (size_t i = 1; i < H8_STRESS_PTRS; ++i) {
    h8_free(g_ptrs[i]);
  }
  return 1;
}

int main(void) {
  h8_init();
  if (!interior_invalid_stress() || !debug_hard_gates_clean("interior")) {
    return 1;
  }
  if (!owner_exit_foreign_free_stress() ||
      !debug_hard_gates_clean("owner-exit-foreign-free")) {
    return 2;
  }
  if (!remote_duplicate_stress() || !debug_hard_gates_clean("remote-duplicate")) {
    return 3;
  }
  H8Stats s = h8_stats();
  H8DebugStats d = h8_debug_stats();
  printf("safety_stress owners=%zu owner_exit=%zu handoff=%zu remote=%zu "
         "collect=%zu duplicate_claim=%zu invalid=%zu\n",
         s.owner_count, s.owner_exit_count, s.orphan_handoff_count,
         s.remote_publish_count, s.remote_collect_count,
         d.remote_publish_pending_claim_duplicate_count, d.invalid_count);
  return 0;
}
