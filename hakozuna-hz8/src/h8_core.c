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

void h8_owner_free_stack_push(H8OwnerRecord* owner) {
  pthread_mutex_lock(&h8g.owner_lock);
  owner->free_next = h8g.owner_free;
  h8g.owner_free = owner;
  pthread_mutex_unlock(&h8g.owner_lock);
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

void h8_owner_mark_alive(H8OwnerRecord* owner, uint32_t slot, uint16_t generation,
                         bool permanent) {
  owner->slot = slot;
  owner->generation = generation;
  owner->permanent = permanent;
  atomic_store_explicit(&owner->pending_head, NULL, memory_order_relaxed);
  owner->owned_head = NULL;
  owner->orphan_head = NULL;
  atomic_store_explicit(&owner->control,
                        h8_ctl_pack((H8CtlWord){.generation = generation,
                                                .state = H8_OWNER_ALIVE,
                                                .publish_closed = 0,
                                                .publish_refs = 0}),
                        memory_order_release);
  pthread_mutex_init(&owner->owned_lock, NULL);
}

void h8_owner_mark_dying(H8OwnerRecord* owner) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    ctl.state = H8_OWNER_DYING;
    ctl.publish_closed = 1;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      break;
    }
  }
}

void h8_owner_mark_dead(H8OwnerRecord* owner) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    ctl.state = H8_OWNER_DEAD;
    ctl.publish_closed = 1;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      break;
    }
  }
}

bool h8_owner_is_alive_and_open(H8OwnerRecord* owner) {
  H8CtlWord ctl = h8_ctl_unpack(atomic_load_explicit(&owner->control,
                                                    memory_order_acquire));
  return ctl.state == H8_OWNER_ALIVE && !ctl.publish_closed;
}

bool h8_owner_publish_enter(H8OwnerRecord* owner, uint16_t expected_generation) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    if (ctl.generation != expected_generation ||
        ctl.state != H8_OWNER_ALIVE ||
        ctl.publish_closed) {
      return false;
    }
    if (ctl.publish_refs == UINT16_MAX) {
      return false;
    }
    ctl.publish_refs++;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acquire,
                                              memory_order_relaxed)) {
      return true;
    }
  }
}

void h8_owner_publish_exit(H8OwnerRecord* owner) {
  uint64_t cur = atomic_load_explicit(&owner->control, memory_order_acquire);
  for (;;) {
    H8CtlWord ctl = h8_ctl_unpack(cur);
    if (ctl.publish_refs == 0) {
      abort();
    }
    ctl.publish_refs--;
    uint64_t next = h8_ctl_pack(ctl);
    if (atomic_compare_exchange_weak_explicit(&owner->control, &cur, next,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      return;
    }
  }
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
  out->arena_reserved_bytes = h8g.arena_bytes;
  out->arena_committed_bytes =
      atomic_load_explicit(&h8g.arena_committed_bytes, memory_order_acquire);
  out->small_span_count = atomic_load_explicit(&h8g.owner_count, memory_order_acquire);
  out->owner_count = atomic_load_explicit(&h8g.owner_count, memory_order_acquire);
  out->orphan_span_count = atomic_load_explicit(&h8g.orphan_span_count, memory_order_acquire);
  out->local_alloc_count = atomic_load_explicit(&h8g.local_alloc_count, memory_order_acquire);
  out->local_free_count = atomic_load_explicit(&h8g.local_free_count, memory_order_acquire);
  out->remote_publish_count = atomic_load_explicit(&h8g.remote_publish_count, memory_order_acquire);
  out->remote_collect_count = atomic_load_explicit(&h8g.remote_collect_count, memory_order_acquire);
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

void h8_owner_add_owned_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  span->next_owned = owner->owned_head;
  owner->owned_head = span;
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_remove_owned_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  H8Span** cur = &owner->owned_head;
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_owned;
      break;
    }
    cur = &(*cur)->next_owned;
  }
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_add_orphan_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  span->next_orphan = owner->orphan_head;
  owner->orphan_head = span;
  pthread_mutex_unlock(&owner->owned_lock);
}

void h8_owner_remove_orphan_span(H8OwnerRecord* owner, H8Span* span) {
  pthread_mutex_lock(&owner->owned_lock);
  H8Span** cur = &owner->orphan_head;
  while (*cur) {
    if (*cur == span) {
      *cur = span->next_orphan;
      break;
    }
    cur = &(*cur)->next_orphan;
  }
  pthread_mutex_unlock(&owner->owned_lock);
}

bool h8_owner_acquire_span_as_orphan(H8OwnerRecord* owner, H8Span* span) {
  if (!owner || !span) {
    return false;
  }
  if (span->span_state != H8_SPAN_ORPHAN_READY) {
    return false;
  }
  span->span_state = H8_SPAN_ADOPTING;
  span->owner_slot = owner->slot;
  span->owner_generation = owner->generation;
  atomic_fetch_add_explicit(&span->span_epoch, 1, memory_order_acq_rel);
  span->span_state = H8_SPAN_OWNED_ACTIVE;
  return true;
}
