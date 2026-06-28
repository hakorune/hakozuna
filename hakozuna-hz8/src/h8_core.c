#include "h8_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

H8Global h8g = {
    .once = H8_PLATFORM_ONCE_INIT,
};

_Thread_local H8ThreadCtx* h8_tls_ctx H8_TLS_FAST;
static const uint16_t kGenerationSeed = 1;

static void h8_init_once(void);

static bool h8_parse_env_bool(const char* value) {
  if (!value || !value[0]) {
    return false;
  }
  if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
      strcmp(value, "TRUE") == 0 || strcmp(value, "yes") == 0 ||
      strcmp(value, "YES") == 0 || strcmp(value, "on") == 0 ||
      strcmp(value, "ON") == 0) {
    return true;
  }
  return false;
}

#if defined(H8_ENABLE_UNSAFE_EVIDENCE_KNOBS)
static bool h8_env_is_one(const char* value) {
  return value && strcmp(value, "1") == 0;
}

static bool h8_parse_unsafe_evidence_env(const char* primary,
                                         const char* deprecated) {
  return h8_env_is_one(getenv(primary)) || h8_env_is_one(getenv(deprecated));
}
#endif

void h8_init(void) {
  h8_platform_once(&h8g.once, h8_init_once);
}

void h8_shutdown(void) {
}

static H8OwnerRecord* h8_owner_free_stack_pop(void) {
  h8_platform_mutex_lock(&h8g.owner_lock);
  H8OwnerRecord* owner = h8g.owner_free;
  if (owner) {
    h8g.owner_free = owner->free_next;
  }
  h8_platform_mutex_unlock(&h8g.owner_lock);
  return owner;
}

static void h8_init_once(void) {
  h8_system_init();
  h8_platform_mutex_init(&h8g.owner_lock);
  atomic_store_explicit(&h8g.regular_adoption_enabled,
                        h8_parse_env_bool(getenv("H8_ENABLE_REGULAR_ADOPTION")),
                        memory_order_relaxed);
#if defined(H8_ENABLE_UNSAFE_EVIDENCE_KNOBS)
  atomic_store_explicit(
      &h8g.remote_lease_elision_enabled,
      h8_parse_unsafe_evidence_env(
          "H8_UNSAFE_EVIDENCE_REMOTE_LEASE_ELISION",
          "H8_ENABLE_REMOTE_LEASE_ELISION"),
      memory_order_relaxed);
  atomic_store_explicit(
      &h8g.remote_pending_publish_elision_enabled,
      h8_parse_unsafe_evidence_env(
          "H8_UNSAFE_EVIDENCE_DROP_REMOTE_PENDING_PUBLISH",
          "H8_ENABLE_REMOTE_PENDING_PUBLISH_ELISION"),
      memory_order_relaxed);
#else
  atomic_store_explicit(&h8g.remote_lease_elision_enabled, false,
                        memory_order_relaxed);
  atomic_store_explicit(&h8g.remote_pending_publish_elision_enabled, false,
                        memory_order_relaxed);
#endif
  h8g.arena_bytes = H8_SMALL_ARENA_BYTES;
  h8g.span_count = h8g.arena_bytes / H8_SPAN_BYTES;
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
  mmap_flags |= MAP_NORESERVE;
#endif
  h8g.arena_base = mmap(NULL, h8g.arena_bytes, PROT_READ | PROT_WRITE,
                        mmap_flags, -1, 0);
  if (h8g.arena_base == MAP_FAILED) {
    fprintf(stderr, "HZ8 arena reservation failed\n");
    abort();
  }
  h8g.spans = h8_sys_calloc(h8g.span_count, sizeof(*h8g.spans));
  if (!h8g.spans) {
    fprintf(stderr, "HZ8 span table allocation failed\n");
    abort();
  }
  if (h8_platform_thread_key_create(&h8g.thread_key, h8_thread_shutdown) != 0) {
    fprintf(stderr, "HZ8 TLS key init failed\n");
    abort();
  }
  H8OwnerRecord* orphan = &h8g.owners[0];
  h8_platform_mutex_init(&orphan->owned_lock);
  h8_platform_mutex_init(&orphan->pending_lock);
  h8_owner_mark_alive(orphan, 0, kGenerationSeed, true);
  orphan->permanent = true;
  h8g.orphan_owner = orphan;
  h8g.current_owner = orphan;
  h8g.owner_free = NULL;
  for (uint32_t i = 1; i < H8_OWNER_MAX; ++i) {
    h8_platform_mutex_init(&h8g.owners[i].owned_lock);
    h8_platform_mutex_init(&h8g.owners[i].pending_lock);
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

H8ThreadCtx* h8_thread_ctx_get_slow(void) {
  h8_init();
  H8ThreadCtx* ctx = h8_platform_thread_getspecific(h8g.thread_key);
  if (ctx) {
    h8_tls_ctx = ctx;
    return ctx;
  }
  ctx = h8_thread_ctx_new();
  if (!ctx) {
    return NULL;
  }
  h8_tls_ctx = ctx;
  h8_platform_thread_setspecific(h8g.thread_key, ctx);
  return ctx;
}

H8ThreadCtx* h8_thread_ctx_get(void) {
  return h8_thread_ctx_fast();
}

H8OwnerRecord* h8_owner_current(void) {
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  return ctx ? ctx->owner : h8g.orphan_owner;
}

void h8_thread_shutdown(void* arg) {
  H8ThreadCtx* ctx = arg;
  if (!ctx) {
    return;
  }
  if (h8_tls_ctx == ctx) {
    h8_tls_ctx = NULL;
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
    H8RouteKind medium_route = h8_medium_route_inner(ptr);
    if (medium_route != H8_ROUTE_MISS) {
      return medium_route;
    }
    return H8_ROUTE_MISS;
  }
  H8Span* span = atomic_load_explicit(&h8g.spans[h8_span_index_from_ptr(ptr)],
                                      memory_order_acquire);
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return H8_ROUTE_INVALID;
  }
  size_t slot = 0;
  if (!h8_slot_index_from_ptr_checked(span, ptr, &slot)) {
    return H8_ROUTE_INVALID;
  }
  uint32_t state = h8_slot_state_load_hot(span, slot);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) ||
      h8_bitmap_test(span->pending_bits, slot)) {
    return H8_ROUTE_INVALID;
  }
  return H8_ROUTE_VALID;
}

H8RouteKind h8_route(void* ptr) {
  return h8_route_inner(ptr);
}
