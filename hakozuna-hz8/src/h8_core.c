#include "h8_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

H8Global h8g = {
    .once = PTHREAD_ONCE_INIT,
};

static const uint16_t kGenerationSeed = 1;

static void h8_init_once(void);

void h8_init(void) {
  pthread_once(&h8g.once, h8_init_once);
}

void h8_shutdown(void) {
}

static H8OwnerRecord* h8_owner_free_stack_pop(void) {
  pthread_mutex_lock(&h8g.owner_lock);
  H8OwnerRecord* owner = h8g.owner_free;
  if (owner) {
    h8g.owner_free = owner->free_next;
  }
  pthread_mutex_unlock(&h8g.owner_lock);
  return owner;
}

static void h8_init_once(void) {
  h8_system_init();
  pthread_mutex_init(&h8g.owner_lock, NULL);
  h8g.arena_bytes = H8_SMALL_ARENA_BYTES;
  h8g.span_count = h8g.arena_bytes / H8_SPAN_BYTES;
  h8g.arena_base = mmap(NULL, h8g.arena_bytes, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (h8g.arena_base == MAP_FAILED) {
    fprintf(stderr, "HZ8 arena reservation failed\n");
    abort();
  }
  h8g.spans = h8_sys_calloc(h8g.span_count, sizeof(*h8g.spans));
  if (!h8g.spans) {
    fprintf(stderr, "HZ8 span table allocation failed\n");
    abort();
  }
  if (pthread_key_create(&h8g.thread_key, h8_thread_shutdown) != 0) {
    fprintf(stderr, "HZ8 TLS key init failed\n");
    abort();
  }
  H8OwnerRecord* orphan = &h8g.owners[0];
  h8_owner_mark_alive(orphan, 0, kGenerationSeed, true);
  orphan->permanent = true;
  h8g.orphan_owner = orphan;
  h8g.current_owner = orphan;
  h8g.owner_free = NULL;
  for (uint32_t i = 1; i < H8_OWNER_MAX; ++i) {
    h8_owner_mark_dead(&h8g.owners[i]);
    h8g.owners[i].slot = i;
    h8g.owners[i].free_next = h8g.owner_free;
    h8g.owner_free = &h8g.owners[i];
  }
  atomic_store_explicit(&h8g.ready, true, memory_order_release);
}


H8OwnerRecord* h8_orphan_owner(void) {
  h8_init();
  return h8g.orphan_owner;
}

static H8ThreadCtx* h8_thread_ctx_new(void) {
  H8ThreadCtx* ctx = h8_sys_calloc(1, sizeof(*ctx));
  if (!ctx) {
    return NULL;
  }
  H8OwnerRecord* owner = h8_owner_free_stack_pop();
  if (!owner) {
    h8_sys_free(ctx);
    return NULL;
  }
  uint16_t generation = (uint16_t)(owner->generation + 1u);
  h8_owner_mark_alive(owner, owner->slot, generation, false);
  ctx->owner = owner;
  atomic_fetch_add_explicit(&h8g.owner_count, 1, memory_order_relaxed);
  return ctx;
}

H8ThreadCtx* h8_thread_ctx_get(void) {
  h8_init();
  H8ThreadCtx* ctx = pthread_getspecific(h8g.thread_key);
  if (ctx) {
    return ctx;
  }
  ctx = h8_thread_ctx_new();
  if (!ctx) {
    return NULL;
  }
  pthread_setspecific(h8g.thread_key, ctx);
  return ctx;
}

H8OwnerRecord* h8_owner_current(void) {
  H8ThreadCtx* ctx = h8_thread_ctx_get();
  return ctx ? ctx->owner : h8g.orphan_owner;
}

void h8_thread_shutdown(void* arg) {
  H8ThreadCtx* ctx = arg;
  if (!ctx) {
    return;
  }
  h8_owner_exit(ctx->owner);
  h8_sys_free(ctx);
}

H8RouteKind h8_route_inner(void* ptr) {
  h8_init();
  if (!ptr) {
    return H8_ROUTE_INVALID;
  }
  if (!h8_arena_contains(ptr)) {
    return H8_ROUTE_MISS;
  }
  H8Span* span = h8g.spans[h8_span_index_from_ptr(ptr)];
  if (!span || span->span_state == H8_SPAN_RETIRED) {
    return H8_ROUTE_INVALID;
  }
  size_t slot = h8_slot_index_from_ptr(span, ptr);
  if (slot >= span->slot_count) {
    return H8_ROUTE_INVALID;
  }
  if (!h8_bitmap_test((_Atomic uint64_t*)span->live_bits, slot)) {
    return H8_ROUTE_INVALID;
  }
  return H8_ROUTE_VALID;
}

H8RouteKind h8_route(void* ptr) {
  return h8_route_inner(ptr);
}

void h8_stats_snapshot(H8Stats* out) {
  size_t small_span_count = 0;
  for (size_t i = 0; i < h8g.span_count; ++i) {
    if (h8g.spans && h8g.spans[i]) {
      ++small_span_count;
    }
  }
  out->arena_reserved_bytes = h8g.arena_bytes;
  out->arena_committed_bytes =
      atomic_load_explicit(&h8g.arena_committed_bytes, memory_order_acquire);
  out->small_span_count = small_span_count;
  out->owner_count = atomic_load_explicit(&h8g.owner_count, memory_order_acquire);
  out->orphan_span_count = atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire);
  out->local_alloc_count = atomic_load_explicit(&h8g.local_alloc_count, memory_order_acquire);
  out->local_free_count = atomic_load_explicit(&h8g.local_free_count, memory_order_acquire);
  out->remote_publish_count = atomic_load_explicit(&h8g.remote_publish_count, memory_order_acquire);
  out->remote_collect_count = atomic_load_explicit(&h8g.remote_collect_count, memory_order_acquire);
  out->owner_publish_enter_count =
      atomic_load_explicit(&h8g.owner_publish_enter_count, memory_order_acquire);
  out->owner_publish_exit_count =
      atomic_load_explicit(&h8g.owner_publish_exit_count, memory_order_acquire);
  out->owner_exit_count = atomic_load_explicit(&h8g.owner_exit_count, memory_order_acquire);
  out->pending_enqueue_count =
      atomic_load_explicit(&h8g.pending_enqueue_count, memory_order_acquire);
  out->pending_dequeue_count =
      atomic_load_explicit(&h8g.pending_dequeue_count, memory_order_acquire);
  out->orphan_handoff_count =
      atomic_load_explicit(&h8g.orphan_handoff_count, memory_order_acquire);
  out->adopt_success_count =
      atomic_load_explicit(&h8g.adopt_success_count, memory_order_acquire);
  out->adopt_fail_count = atomic_load_explicit(&h8g.adopt_fail_count, memory_order_acquire);
  out->invalid_count = atomic_load_explicit(&h8g.invalid_count, memory_order_acquire);
  out->miss_count = atomic_load_explicit(&h8g.miss_count, memory_order_acquire);
  out->owner_transition_count =
      atomic_load_explicit(&h8g.owner_transition_count, memory_order_acquire);
}

H8Stats h8_stats(void) {
  H8Stats stats;
  memset(&stats, 0, sizeof(stats));
  h8_stats_snapshot(&stats);
  return stats;
}
