#include "../src/h8_internal.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if !defined(H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1)
#error "owner page API smoke requires H9_OWNER_LOCAL_PAGE_POOL_PURE_LOCAL_L1"
#endif

static size_t h9_delta(size_t after, size_t before) {
  return after >= before ? after - before : 0u;
}

static int exercise_same_owner_api(H8ThreadCtx* ctx) {
  H8DebugStats before = h8_debug_stats();
  void* a = h9_owner_page_try_alloc(ctx, 2u);
  void* b = h9_owner_page_try_alloc(ctx, 2u);
  if (!a || !b || a == b) {
    fprintf(stderr, "owner-page same-owner alloc setup failed\n");
    return 1;
  }
  memset(a, 0x9a, 4097u);
  memset(b, 0x9b, 4097u);

  bool owned = false;
  if (!h9_owner_page_try_free(ctx, a, &owned) || !owned) {
    fprintf(stderr, "owner-page same-owner free did not push\n");
    return 2;
  }
  if (h9_owner_page_try_free(ctx, a, &owned) || !owned) {
    fprintf(stderr, "owner-page double free was not fail-closed\n");
    return 3;
  }
  void* interior = (void*)((uintptr_t)b + 1u);
  owned = false;
  if (h9_owner_page_try_free(ctx, interior, &owned) || !owned) {
    fprintf(stderr, "owner-page interior pointer was not owned invalid\n");
    return 4;
  }
  void* again = h9_owner_page_try_alloc(ctx, 2u);
  if (again != a) {
    fprintf(stderr, "owner-page local pop did not return freed slot\n");
    return 5;
  }
  if (!h9_owner_page_try_free(ctx, again, &owned) || !owned ||
      !h9_owner_page_try_free(ctx, b, &owned) || !owned) {
    fprintf(stderr, "owner-page final local free failed\n");
    return 6;
  }
  H8DebugStats after = h8_debug_stats();
  if (h9_delta(after.h9_owner_page_api_alloc_hit,
               before.h9_owner_page_api_alloc_hit) < 3u ||
      h9_delta(after.h9_owner_page_api_free_push,
               before.h9_owner_page_api_free_push) < 3u ||
      h9_delta(after.h9_owner_page_api_free_double,
               before.h9_owner_page_api_free_double) == 0u ||
      h9_delta(after.h9_owner_page_api_route_invalid,
               before.h9_owner_page_api_route_invalid) == 0u) {
    fprintf(stderr, "owner-page same-owner counters did not advance\n");
    return 7;
  }
  return 0;
}

static int exercise_remote_pending_api(H8ThreadCtx* ctx) {
  H8DebugStats before = h8_debug_stats();
  void* a = h9_owner_page_try_alloc(ctx, 5u);
  void* b = h9_owner_page_try_alloc(ctx, 5u);
  if (!a || !b || a == b) {
    fprintf(stderr, "owner-page remote setup alloc failed\n");
    return 10;
  }

  bool owned = false;
  if (!h9_owner_page_try_free(NULL, a, &owned) || !owned) {
    fprintf(stderr, "owner-page remote-like free did not mark pending\n");
    return 11;
  }
  if (h9_owner_page_try_free(NULL, a, &owned) || !owned) {
    fprintf(stderr, "owner-page duplicate remote pending was not rejected\n");
    return 12;
  }
  if (h9_owner_page_try_alloc(ctx, 5u) != NULL) {
    fprintf(stderr, "owner-page remote-seen page kept allocating\n");
    return 13;
  }

  h9_owner_page_flush_thread(ctx);
  if (!h9_owner_page_try_free(ctx, b, &owned) || !owned) {
    fprintf(stderr, "owner-page detached final owner free failed\n");
    return 14;
  }
  owned = true;
  if (h9_owner_page_try_free(ctx, b, &owned) || owned) {
    fprintf(stderr, "owner-page released page stayed owned\n");
    return 15;
  }

  H8DebugStats after = h8_debug_stats();
  if (h9_delta(after.h9_owner_page_api_remote_pending_first,
               before.h9_owner_page_api_remote_pending_first) == 0u ||
      h9_delta(after.h9_owner_page_api_remote_pending_repeat,
               before.h9_owner_page_api_remote_pending_repeat) == 0u ||
      h9_delta(after.h9_owner_page_api_alloc_mode_block,
               before.h9_owner_page_api_alloc_mode_block) == 0u ||
      h9_delta(after.h9_owner_page_api_page_pending_collect,
               before.h9_owner_page_api_page_pending_collect) == 0u) {
    fprintf(stderr, "owner-page remote counters did not advance\n");
    return 16;
  }
  return 0;
}

int main(void) {
  h8_init();
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  if (!ctx || !ctx->owner) {
    fprintf(stderr, "owner-page API smoke could not create owner ctx\n");
    return 20;
  }
  int rc = exercise_same_owner_api(ctx);
  if (rc != 0) {
    return rc;
  }
  rc = exercise_remote_pending_api(ctx);
  if (rc != 0) {
    return rc;
  }
  h9_owner_page_flush_thread(ctx);
  H8DebugStats stats = h8_debug_stats();
  printf("hz9_owner_page_api_smoke hit=%zu push=%zu double=%zu "
         "invalid=%zu remote_first=%zu remote_repeat=%zu mode_block=%zu "
         "pending_collect=%zu release=%zu\n",
         stats.h9_owner_page_api_alloc_hit,
         stats.h9_owner_page_api_free_push,
         stats.h9_owner_page_api_free_double,
         stats.h9_owner_page_api_route_invalid,
         stats.h9_owner_page_api_remote_pending_first,
         stats.h9_owner_page_api_remote_pending_repeat,
         stats.h9_owner_page_api_alloc_mode_block,
         stats.h9_owner_page_api_page_pending_collect,
         stats.h9_owner_page_api_page_release);
  return stats.h9_owner_page_api_local_bits_mutation == 0u ? 0 : 21;
}
