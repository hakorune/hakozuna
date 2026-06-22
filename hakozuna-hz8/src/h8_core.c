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

static _Thread_local H8ThreadCtx* h8_tls_ctx;
static const uint16_t kGenerationSeed = 1;

static void h8_init_once(void);

static bool h8_env_is_one(const char* value) {
  return value && strcmp(value, "1") == 0;
}

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

static bool h8_parse_unsafe_evidence_env(const char* primary,
                                         const char* deprecated) {
  return h8_env_is_one(getenv(primary)) || h8_env_is_one(getenv(deprecated));
}

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
  atomic_store_explicit(&h8g.regular_adoption_enabled,
                        h8_parse_env_bool(getenv("H8_ENABLE_REGULAR_ADOPTION")),
                        memory_order_relaxed);
  atomic_store_explicit(
      &h8g.slot_state_authority_enabled,
      h8_parse_env_bool(getenv("H8_ENABLE_SLOT_STATE_AUTHORITY")),
      memory_order_relaxed);
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
  if (pthread_key_create(&h8g.thread_key, h8_thread_shutdown) != 0) {
    fprintf(stderr, "HZ8 TLS key init failed\n");
    abort();
  }
  H8OwnerRecord* orphan = &h8g.owners[0];
  pthread_mutex_init(&orphan->owned_lock, NULL);
  pthread_mutex_init(&orphan->pending_lock, NULL);
  h8_owner_mark_alive(orphan, 0, kGenerationSeed, true);
  orphan->permanent = true;
  h8g.orphan_owner = orphan;
  h8g.current_owner = orphan;
  h8g.owner_free = NULL;
  for (uint32_t i = 1; i < H8_OWNER_MAX; ++i) {
    pthread_mutex_init(&h8g.owners[i].owned_lock, NULL);
    pthread_mutex_init(&h8g.owners[i].pending_lock, NULL);
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
  H8ThreadCtx* ctx = h8_tls_ctx;
  if (ctx) {
    return ctx;
  }
  ctx = pthread_getspecific(h8g.thread_key);
  if (ctx) {
    h8_tls_ctx = ctx;
    return ctx;
  }
  ctx = h8_thread_ctx_new();
  if (!ctx) {
    return NULL;
  }
  h8_tls_ctx = ctx;
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
    return H8_ROUTE_MISS;
  }
  H8Span* span = atomic_load_explicit(&h8g.spans[h8_span_index_from_ptr(ptr)],
                                      memory_order_acquire);
  if (!span || h8_span_state_load(span) == H8_SPAN_RETIRED) {
    return H8_ROUTE_INVALID;
  }
  size_t slot = h8_slot_index_from_ptr(span, ptr);
  if (slot >= span->slot_count) {
    return H8_ROUTE_INVALID;
  }
  if (h8_slot_state_authority_enabled()) {
    uint32_t state = h8_slot_state_load_acquire(span, slot);
    if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) ||
        h8_bitmap_test(span->pending_bits, slot)) {
      return H8_ROUTE_INVALID;
    }
    return H8_ROUTE_VALID;
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
    if (h8g.spans &&
        atomic_load_explicit(&h8g.spans[i], memory_order_acquire)) {
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
  out->owner_exit_count = atomic_load_explicit(&h8g.owner_exit_count, memory_order_acquire);
  out->pending_enqueue_count =
      atomic_load_explicit(&h8g.pending_enqueue_count, memory_order_acquire);
  out->pending_dequeue_count =
      atomic_load_explicit(&h8g.pending_dequeue_count, memory_order_acquire);
  out->orphan_handoff_count =
      atomic_load_explicit(&h8g.orphan_handoff_count, memory_order_acquire);
  out->handoff_success_count =
      atomic_load_explicit(&h8g.handoff_success_count, memory_order_acquire);
}

H8Stats h8_stats(void) {
  H8Stats stats;
  memset(&stats, 0, sizeof(stats));
  h8_stats_snapshot(&stats);
  return stats;
}

void h8_debug_stats_snapshot(H8DebugStats* out) {
#if !defined(H8_ENABLE_DEBUG_STATS)
  memset(out, 0, sizeof(*out));
  return;
#else
  out->owner_lifecycle_enter_count =
      atomic_load_explicit(&h8g.owner_lifecycle_enter_count, memory_order_acquire);
  out->owner_lifecycle_exit_count =
      atomic_load_explicit(&h8g.owner_lifecycle_exit_count, memory_order_acquire);
  out->owner_publish_enter_count =
      atomic_load_explicit(&h8g.owner_publish_enter_count, memory_order_acquire);
  out->owner_publish_exit_count =
      atomic_load_explicit(&h8g.owner_publish_exit_count, memory_order_acquire);
  out->span_publish_enter_count =
      atomic_load_explicit(&h8g.span_publish_enter_count, memory_order_acquire);
  out->span_publish_exit_count =
      atomic_load_explicit(&h8g.span_publish_exit_count, memory_order_acquire);
  out->remote_regular_admission_count =
      atomic_load_explicit(&h8g.remote_regular_admission_count, memory_order_acquire);
  out->remote_orphan_admission_count =
      atomic_load_explicit(&h8g.remote_orphan_admission_count, memory_order_acquire);
  out->remote_stage_enter =
      atomic_load_explicit(&h8g.remote_stage_enter, memory_order_acquire);
  out->remote_stage_span_miss =
      atomic_load_explicit(&h8g.remote_stage_span_miss, memory_order_acquire);
  out->remote_stage_owner_missing =
      atomic_load_explicit(&h8g.remote_stage_owner_missing, memory_order_acquire);
  out->remote_stage_regular_lease_ok =
      atomic_load_explicit(&h8g.remote_stage_regular_lease_ok, memory_order_acquire);
  out->remote_stage_regular_lease_fail = atomic_load_explicit(
      &h8g.remote_stage_regular_lease_fail, memory_order_acquire);
  out->remote_stage_regular_lease_elided = atomic_load_explicit(
      &h8g.remote_stage_regular_lease_elided, memory_order_acquire);
  out->remote_stage_orphan_lease_ok =
      atomic_load_explicit(&h8g.remote_stage_orphan_lease_ok, memory_order_acquire);
  out->remote_stage_orphan_lease_fail = atomic_load_explicit(
      &h8g.remote_stage_orphan_lease_fail, memory_order_acquire);
  out->remote_stage_pending_claim_ok =
      atomic_load_explicit(&h8g.remote_stage_pending_claim_ok, memory_order_acquire);
  out->remote_stage_validate_fail =
      atomic_load_explicit(&h8g.remote_stage_validate_fail, memory_order_acquire);
  out->remote_stage_notify_first =
      atomic_load_explicit(&h8g.remote_stage_notify_first, memory_order_acquire);
  out->remote_stage_publish_ok =
      atomic_load_explicit(&h8g.remote_stage_publish_ok, memory_order_acquire);
  out->remote_stage_pending_publish_elided = atomic_load_explicit(
      &h8g.remote_stage_pending_publish_elided, memory_order_acquire);
  out->remote_lookup_enter =
      atomic_load_explicit(&h8g.remote_lookup_enter, memory_order_acquire);
  out->remote_lookup_arena_miss =
      atomic_load_explicit(&h8g.remote_lookup_arena_miss, memory_order_acquire);
  out->remote_lookup_span_miss =
      atomic_load_explicit(&h8g.remote_lookup_span_miss, memory_order_acquire);
  out->remote_lookup_retired =
      atomic_load_explicit(&h8g.remote_lookup_retired, memory_order_acquire);
  out->remote_lookup_slot_oob =
      atomic_load_explicit(&h8g.remote_lookup_slot_oob, memory_order_acquire);
  out->remote_lookup_ok =
      atomic_load_explicit(&h8g.remote_lookup_ok, memory_order_acquire);
  out->remote_owner_word_load =
      atomic_load_explicit(&h8g.remote_owner_word_load, memory_order_acquire);
  out->pending_notify_count =
      atomic_load_explicit(&h8g.pending_notify_count, memory_order_acquire);
  out->qstate_notify_attempt_count =
      atomic_load_explicit(&h8g.qstate_notify_attempt_count, memory_order_acquire);
  out->qstate_notify_success_count =
      atomic_load_explicit(&h8g.qstate_notify_success_count, memory_order_acquire);
  out->qstate_notify_skip_nonidle_count = atomic_load_explicit(
      &h8g.qstate_notify_skip_nonidle_count, memory_order_acquire);
  out->pending_head_push_attempt_count = atomic_load_explicit(
      &h8g.pending_head_push_attempt_count, memory_order_acquire);
  out->pending_head_push_retry_count =
      atomic_load_explicit(&h8g.pending_head_push_retry_count, memory_order_acquire);
  out->pending_head_push_success_count = atomic_load_explicit(
      &h8g.pending_head_push_success_count, memory_order_acquire);
  out->owner_lifecycle_enter_cas_retry_count = atomic_load_explicit(
      &h8g.owner_lifecycle_enter_cas_retry_count, memory_order_acquire);
  out->owner_lifecycle_exit_cas_retry_count = atomic_load_explicit(
      &h8g.owner_lifecycle_exit_cas_retry_count, memory_order_acquire);
  out->remote_publish_pending_claim_duplicate_count = atomic_load_explicit(
      &h8g.remote_publish_pending_claim_duplicate_count, memory_order_acquire);
  out->pending_collect_call_count =
      atomic_load_explicit(&h8g.pending_collect_call_count, memory_order_acquire);
  out->pending_collect_carry_hit_count = atomic_load_explicit(
      &h8g.pending_collect_carry_hit_count, memory_order_acquire);
  out->pending_collect_requeue_count =
      atomic_load_explicit(&h8g.pending_collect_requeue_count, memory_order_acquire);
  out->pending_word_summary_set =
      atomic_load_explicit(&h8g.pending_word_summary_set, memory_order_acquire);
  out->pending_word_summary_shadow_hit = atomic_load_explicit(
      &h8g.pending_word_summary_shadow_hit, memory_order_acquire);
  out->pending_word_summary_false_positive = atomic_load_explicit(
      &h8g.pending_word_summary_false_positive, memory_order_acquire);
  out->pending_word_summary_false_negative = atomic_load_explicit(
      &h8g.pending_word_summary_false_negative, memory_order_acquire);
  out->pending_word_summary_rearm =
      atomic_load_explicit(&h8g.pending_word_summary_rearm, memory_order_acquire);
  out->pending_word_summary_repair =
      atomic_load_explicit(&h8g.pending_word_summary_repair, memory_order_acquire);
  out->pending_word_drain_count =
      atomic_load_explicit(&h8g.pending_word_drain_count, memory_order_acquire);
  out->pending_word_popcount_1 =
      atomic_load_explicit(&h8g.pending_word_popcount_1, memory_order_acquire);
  out->pending_word_popcount_2 =
      atomic_load_explicit(&h8g.pending_word_popcount_2, memory_order_acquire);
  out->pending_word_popcount_3_4 =
      atomic_load_explicit(&h8g.pending_word_popcount_3_4, memory_order_acquire);
  out->pending_word_popcount_5_8 =
      atomic_load_explicit(&h8g.pending_word_popcount_5_8, memory_order_acquire);
  out->pending_word_popcount_9_16 =
      atomic_load_explicit(&h8g.pending_word_popcount_9_16, memory_order_acquire);
  out->pending_word_popcount_17_plus =
      atomic_load_explicit(&h8g.pending_word_popcount_17_plus, memory_order_acquire);
  out->pending_slots_drained =
      atomic_load_explicit(&h8g.pending_slots_drained, memory_order_acquire);
  out->pending_words_rearmed =
      atomic_load_explicit(&h8g.pending_words_rearmed, memory_order_acquire);
  out->pending_word_new_publish_during_drain = atomic_load_explicit(
      &h8g.pending_word_new_publish_during_drain, memory_order_acquire);
  out->local_alloc_pending_nonzero =
      atomic_load_explicit(&h8g.local_alloc_pending_nonzero, memory_order_acquire);
  out->local_free_pending_nonzero =
      atomic_load_explicit(&h8g.local_free_pending_nonzero, memory_order_acquire);
  out->owner_live_set_already_live =
      atomic_load_explicit(&h8g.owner_live_set_already_live, memory_order_acquire);
  out->owner_live_clear_already_free =
      atomic_load_explicit(&h8g.owner_live_clear_already_free, memory_order_acquire);
  out->local_active_hit =
      atomic_load_explicit(&h8g.local_active_hit, memory_order_acquire);
  out->local_active_miss =
      atomic_load_explicit(&h8g.local_active_miss, memory_order_acquire);
  out->local_freelist_pop =
      atomic_load_explicit(&h8g.local_freelist_pop, memory_order_acquire);
  out->local_bump_alloc =
      atomic_load_explicit(&h8g.local_bump_alloc, memory_order_acquire);
  out->local_slow_collect =
      atomic_load_explicit(&h8g.local_slow_collect, memory_order_acquire);
  out->local_span_commit =
      atomic_load_explicit(&h8g.local_span_commit, memory_order_acquire);
  out->local_find_scan =
      atomic_load_explicit(&h8g.local_find_scan, memory_order_acquire);
  out->local_find_scan_span =
      atomic_load_explicit(&h8g.local_find_scan_span, memory_order_acquire);
  out->local_active_hint_null =
      atomic_load_explicit(&h8g.local_active_hint_null, memory_order_acquire);
  out->local_active_hint_full =
      atomic_load_explicit(&h8g.local_active_hint_full, memory_order_acquire);
  out->local_active_hint_state_blocked = atomic_load_explicit(
      &h8g.local_active_hint_state_blocked, memory_order_acquire);
  out->local_find_scan_span_usable = atomic_load_explicit(
      &h8g.local_find_scan_span_usable, memory_order_acquire);
  out->local_find_scan_span_full =
      atomic_load_explicit(&h8g.local_find_scan_span_full, memory_order_acquire);
  out->local_find_scan_span_state_blocked = atomic_load_explicit(
      &h8g.local_find_scan_span_state_blocked, memory_order_acquire);
  out->local_find_skip_scan_no_pending = atomic_load_explicit(
      &h8g.local_find_skip_scan_no_pending, memory_order_acquire);
  out->local_free_hit =
      atomic_load_explicit(&h8g.local_free_hit, memory_order_acquire);
  out->local_free_reject_owner =
      atomic_load_explicit(&h8g.local_free_reject_owner, memory_order_acquire);
  out->local_free_reject_state =
      atomic_load_explicit(&h8g.local_free_reject_state, memory_order_acquire);
  out->local_free_reject_live =
      atomic_load_explicit(&h8g.local_free_reject_live, memory_order_acquire);
  out->local_live_touch_alloc =
      atomic_load_explicit(&h8g.local_live_touch_alloc, memory_order_acquire);
  out->local_live_touch_free =
      atomic_load_explicit(&h8g.local_live_touch_free, memory_order_acquire);
  out->local_live_word_0 =
      atomic_load_explicit(&h8g.local_live_word_0, memory_order_acquire);
  out->local_live_word_1 =
      atomic_load_explicit(&h8g.local_live_word_1, memory_order_acquire);
  out->local_live_word_2_7 =
      atomic_load_explicit(&h8g.local_live_word_2_7, memory_order_acquire);
  out->local_live_word_8_31 =
      atomic_load_explicit(&h8g.local_live_word_8_31, memory_order_acquire);
  out->local_live_word_32_63 =
      atomic_load_explicit(&h8g.local_live_word_32_63, memory_order_acquire);
  out->local_free_head_touch_alloc = atomic_load_explicit(
      &h8g.local_free_head_touch_alloc, memory_order_acquire);
  out->local_free_head_touch_free = atomic_load_explicit(
      &h8g.local_free_head_touch_free, memory_order_acquire);
  out->local_pending_check_alloc =
      atomic_load_explicit(&h8g.local_pending_check_alloc, memory_order_acquire);
  out->local_pending_check_free =
      atomic_load_explicit(&h8g.local_pending_check_free, memory_order_acquire);
  out->local_used_touch_alloc =
      atomic_load_explicit(&h8g.local_used_touch_alloc, memory_order_acquire);
  out->local_used_touch_free =
      atomic_load_explicit(&h8g.local_used_touch_free, memory_order_acquire);
  out->span_commit_total_ns =
      atomic_load_explicit(&h8g.span_commit_total_ns, memory_order_acquire);
  out->span_commit_lock_wait_ns =
      atomic_load_explicit(&h8g.span_commit_lock_wait_ns, memory_order_acquire);
  out->span_commit_table_scan_ns =
      atomic_load_explicit(&h8g.span_commit_table_scan_ns, memory_order_acquire);
  out->span_commit_meta_ns =
      atomic_load_explicit(&h8g.span_commit_meta_ns, memory_order_acquire);
  out->span_commit_mprotect_ns =
      atomic_load_explicit(&h8g.span_commit_mprotect_ns, memory_order_acquire);
  out->slot_shadow_valid_mismatch =
      atomic_load_explicit(&h8g.slot_shadow_valid_mismatch, memory_order_acquire);
  out->slot_shadow_invalid_mismatch =
      atomic_load_explicit(&h8g.slot_shadow_invalid_mismatch, memory_order_acquire);
  out->slot_shadow_pending_nonallocated = atomic_load_explicit(
      &h8g.slot_shadow_pending_nonallocated, memory_order_acquire);
  out->slot_shadow_free_unreachable =
      atomic_load_explicit(&h8g.slot_shadow_free_unreachable, memory_order_acquire);
  out->slot_shadow_free_duplicate =
      atomic_load_explicit(&h8g.slot_shadow_free_duplicate, memory_order_acquire);
  out->slot_shadow_free_cycle =
      atomic_load_explicit(&h8g.slot_shadow_free_cycle, memory_order_acquire);
  out->slot_shadow_bad_next =
      atomic_load_explicit(&h8g.slot_shadow_bad_next, memory_order_acquire);
  out->slot_shadow_never_used_below_bump = atomic_load_explicit(
      &h8g.slot_shadow_never_used_below_bump, memory_order_acquire);
  out->slot_shadow_nonvirgin_above_bump = atomic_load_explicit(
      &h8g.slot_shadow_nonvirgin_above_bump, memory_order_acquire);
  out->slot_shadow_used_mismatch =
      atomic_load_explicit(&h8g.slot_shadow_used_mismatch, memory_order_acquire);
  out->slot_shadow_reserved_quiescent = atomic_load_explicit(
      &h8g.slot_shadow_reserved_quiescent, memory_order_acquire);
  out->pending_collect_word_count =
      atomic_load_explicit(&h8g.pending_collect_word_count, memory_order_acquire);
  out->pending_collect_word_nonzero_count = atomic_load_explicit(
      &h8g.pending_collect_word_nonzero_count, memory_order_acquire);
  out->pending_collect_bit_count =
      atomic_load_explicit(&h8g.pending_collect_bit_count, memory_order_acquire);
  out->pending_mask_notify_without_count = atomic_load_explicit(
      &h8g.pending_mask_notify_without_count, memory_order_acquire);
  out->pending_count_notify_without_mask = atomic_load_explicit(
      &h8g.pending_count_notify_without_mask, memory_order_acquire);
  out->pending_mask_requeue_without_count = atomic_load_explicit(
      &h8g.pending_mask_requeue_without_count, memory_order_acquire);
  out->pending_count_requeue_without_mask = atomic_load_explicit(
      &h8g.pending_count_requeue_without_mask, memory_order_acquire);
  out->pending_finish_count_mask_zero_bitmap_zero = atomic_load_explicit(
      &h8g.pending_finish_count_mask_zero_bitmap_zero, memory_order_acquire);
  out->pending_finish_count_mask_zero_bitmap_nonzero = atomic_load_explicit(
      &h8g.pending_finish_count_mask_zero_bitmap_nonzero, memory_order_acquire);
  out->pending_finish_mask_nonzero_bitmap_zero = atomic_load_explicit(
      &h8g.pending_finish_mask_nonzero_bitmap_zero, memory_order_acquire);
  out->pending_finish_mask_nonzero_bitmap_nonzero = atomic_load_explicit(
      &h8g.pending_finish_mask_nonzero_bitmap_nonzero, memory_order_acquire);
  out->pending_publish_mask_arm_raced_nonempty = atomic_load_explicit(
      &h8g.pending_publish_mask_arm_raced_nonempty, memory_order_acquire);
  out->qstate_dirty_set =
      atomic_load_explicit(&h8g.qstate_dirty_set, memory_order_acquire);
  out->qstate_dirty_self_set =
      atomic_load_explicit(&h8g.qstate_dirty_self_set, memory_order_acquire);
  out->qstate_dirty_requeue =
      atomic_load_explicit(&h8g.qstate_dirty_requeue, memory_order_acquire);
  out->quiescent_pending_bitmap_nonzero = atomic_load_explicit(
      &h8g.quiescent_pending_bitmap_nonzero, memory_order_acquire);
  out->quiescent_pending_repair =
      atomic_load_explicit(&h8g.quiescent_pending_repair, memory_order_acquire);
  out->orphan_quiesce_count =
      atomic_load_explicit(&h8g.orphan_quiesce_count, memory_order_acquire);
  out->orphan_ready_count =
      atomic_load_explicit(&h8g.orphan_ready_count, memory_order_acquire);
  out->adoption_dry_run_scan_count =
      atomic_load_explicit(&h8g.adoption_dry_run_scan_count, memory_order_acquire);
  out->adoption_dry_run_candidate_count =
      atomic_load_explicit(&h8g.adoption_dry_run_candidate_count, memory_order_acquire);
  out->adoption_dry_run_block_state_count =
      atomic_load_explicit(&h8g.adoption_dry_run_block_state_count, memory_order_acquire);
  out->adoption_dry_run_block_quiesce_count =
      atomic_load_explicit(&h8g.adoption_dry_run_block_quiesce_count, memory_order_acquire);
  out->adoption_dry_run_empty_count =
      atomic_load_explicit(&h8g.adoption_dry_run_empty_count, memory_order_acquire);
  out->adoption_dry_run_target_closed_count =
      atomic_load_explicit(&h8g.adoption_dry_run_target_closed_count, memory_order_acquire);
  out->adoption_dry_run_would_adopt_count =
      atomic_load_explicit(&h8g.adoption_dry_run_would_adopt_count, memory_order_acquire);
  out->handoff_fail_count = atomic_load_explicit(&h8g.handoff_fail_count, memory_order_acquire);
  out->invalid_count = atomic_load_explicit(&h8g.invalid_count, memory_order_acquire);
  out->miss_count = atomic_load_explicit(&h8g.miss_count, memory_order_acquire);
  out->owner_transition_count =
      atomic_load_explicit(&h8g.owner_transition_count, memory_order_acquire);
  out->adoption_scan_count =
      atomic_load_explicit(&h8g.adoption_scan_count, memory_order_acquire);
  out->adoption_candidate_count =
      atomic_load_explicit(&h8g.adoption_candidate_count, memory_order_acquire);
  out->adoption_block_state_count =
      atomic_load_explicit(&h8g.adoption_block_state_count, memory_order_acquire);
  out->adoption_block_quiesce_count =
      atomic_load_explicit(&h8g.adoption_block_quiesce_count, memory_order_acquire);
  out->adoption_empty_count =
      atomic_load_explicit(&h8g.adoption_empty_count, memory_order_acquire);
  out->adoption_target_closed_count =
      atomic_load_explicit(&h8g.adoption_target_closed_count, memory_order_acquire);
  out->adoption_success_count =
      atomic_load_explicit(&h8g.adoption_success_count, memory_order_acquire);
#endif
}

H8DebugStats h8_debug_stats(void) {
  H8DebugStats stats;
  memset(&stats, 0, sizeof(stats));
  h8_debug_stats_snapshot(&stats);
  return stats;
}
