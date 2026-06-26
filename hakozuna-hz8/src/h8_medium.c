#include "h8_internal.h"
#include "h8_medium.h"

#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#if defined(H8_ENABLE_DEBUG_STATS)
#include <time.h>
#endif

static const H8MediumClassSpec k_h8_medium_classes[H8_MEDIUM_CLASS_COUNT] = {
    {8192u, H8_MEDIUM_RUN_BYTES, 13u, 8u, 1u},
    {16384u, H8_MEDIUM_RUN_BYTES, 14u, 4u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 15u, 2u, 1u},
#if defined(H8_MEDIUM_UPPER48_CLASS)
    {49152u, H8_MEDIUM_RUN_BYTES, 14u, 1u, 1u},
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#else
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#endif
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
#if defined(H8_MEDIUM_UPPER48_CLASS)
_Static_assert(H8_MEDIUM_CLASS_COUNT == 5u,
               "upper48 medium candidate expects five classes");
#else
_Static_assert(H8_MEDIUM_CLASS_COUNT == 4u,
               "default medium scaffold expects four classes");
#endif
_Static_assert((H8_MEDIUM_64K_RUN_BYTES % H8_MEDIUM_QUANTUM_BYTES) == 0u,
               "medium run size must be quantum-aligned");

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_medium_debug_class_inc(uint32_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k);

static uint64_t h8_medium_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

void h8_medium_debug_note_owner_medium_alloc(H8OwnerRecord* owner) {
  if (owner) {
    ++owner->debug_medium_alloc_epoch;
  }
}

void h8_medium_debug_note_active_miss_pending(H8ThreadCtx* ctx,
                                              uint32_t class_id,
                                              H8MediumRun* active) {
  H8_DEBUG_INC(medium_active_miss_total);
  h8_medium_debug_class_inc(class_id, &h8g.medium_active_miss_class_8k,
                            &h8g.medium_active_miss_class_16k,
                            &h8g.medium_active_miss_class_32k,
                            &h8g.medium_active_miss_class_64k);
  if (!ctx || !ctx->owner) {
    return;
  }
  bool owner_pending = h8_medium_owner_has_pending(ctx->owner);
  if (owner_pending) {
    H8_DEBUG_INC(medium_active_miss_owner_pending);
  } else {
    H8_DEBUG_INC(medium_active_miss_no_pending);
  }
  if (!active || active->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  uint64_t pending =
      atomic_load_explicit(&active->pending_bits[0], memory_order_acquire);
  if (active->slot_count < 64u) {
    pending &= (UINT64_C(1) << active->slot_count) - 1u;
  }
  if (pending) {
    H8_DEBUG_INC(medium_active_miss_active_pending);
    H8_DEBUG_ADD(medium_active_miss_active_pending_slots,
                 (size_t)__builtin_popcountll(pending));
    h8_medium_debug_class_inc(class_id, &h8g.medium_active_pending_class_8k,
                              &h8g.medium_active_pending_class_16k,
                              &h8g.medium_active_pending_class_32k,
                              &h8g.medium_active_pending_class_64k);
  } else {
    H8_DEBUG_INC(medium_active_miss_active_pending_zero);
  }
}

void h8_medium_debug_note_owner_list_hit_position(size_t position) {
  H8_DEBUG_INC(medium_active_miss_owner_list_hit);
  if (position <= 1u) {
    H8_DEBUG_INC(medium_active_miss_owner_list_pos1);
  } else if (position == 2u) {
    H8_DEBUG_INC(medium_active_miss_owner_list_pos2);
  } else if (position == 3u) {
    H8_DEBUG_INC(medium_active_miss_owner_list_pos3);
  } else if (position == 4u) {
    H8_DEBUG_INC(medium_active_miss_owner_list_pos4);
  } else {
    H8_DEBUG_INC(medium_active_miss_owner_list_pos5p);
  }
}

void h8_medium_debug_note_owner_list_hit_class(uint32_t class_id) {
  h8_medium_debug_class_inc(class_id, &h8g.medium_owner_list_hit_class_8k,
                            &h8g.medium_owner_list_hit_class_16k,
                            &h8g.medium_owner_list_hit_class_32k,
                            &h8g.medium_owner_list_hit_class_64k);
}

static void h8_medium_debug_collect_source_add(H8MediumCollectSource source,
                                               size_t runs,
                                               size_t slots) {
  switch (source) {
    case H8_MEDIUM_COLLECT_PERIODIC_ACTIVE:
      H8_DEBUG_ADD(medium_collect_src_periodic_active_run, runs);
      H8_DEBUG_ADD(medium_collect_src_periodic_active_slot, slots);
      break;
    case H8_MEDIUM_COLLECT_PERIODIC_OWNER_LIST:
      H8_DEBUG_ADD(medium_collect_src_periodic_owner_run, runs);
      H8_DEBUG_ADD(medium_collect_src_periodic_owner_slot, slots);
      break;
    case H8_MEDIUM_COLLECT_CAPACITY_MISS:
      H8_DEBUG_ADD(medium_collect_src_capacity_run, runs);
      H8_DEBUG_ADD(medium_collect_src_capacity_slot, slots);
      break;
    case H8_MEDIUM_COLLECT_OWNER_EXIT:
      H8_DEBUG_ADD(medium_collect_src_owner_exit_run, runs);
      H8_DEBUG_ADD(medium_collect_src_owner_exit_slot, slots);
      break;
    case H8_MEDIUM_COLLECT_ACTIVE_MISS_DEMAND:
      H8_DEBUG_ADD(medium_collect_src_demand_run, runs);
      H8_DEBUG_ADD(medium_collect_src_demand_slot, slots);
      break;
  }
}

static void h8_medium_debug_collect_full_to_nonfull_source(
    H8MediumCollectSource source) {
  switch (source) {
    case H8_MEDIUM_COLLECT_PERIODIC_ACTIVE:
      H8_DEBUG_INC(medium_collect_full_to_nonfull_periodic_active);
      break;
    case H8_MEDIUM_COLLECT_PERIODIC_OWNER_LIST:
      H8_DEBUG_INC(medium_collect_full_to_nonfull_periodic_owner);
      break;
    case H8_MEDIUM_COLLECT_CAPACITY_MISS:
      H8_DEBUG_INC(medium_collect_full_to_nonfull_capacity);
      break;
    case H8_MEDIUM_COLLECT_OWNER_EXIT:
      H8_DEBUG_INC(medium_collect_full_to_nonfull_owner_exit);
      break;
    case H8_MEDIUM_COLLECT_ACTIVE_MISS_DEMAND:
      H8_DEBUG_INC(medium_collect_full_to_nonfull_demand);
      break;
  }
}

void h8_medium_debug_note_collect_capacity(H8OwnerRecord* owner,
                                           H8MediumRun* run,
                                           uint64_t accepted,
                                           uint64_t old_free_mask,
                                           H8MediumCollectSource source) {
  size_t slots = (size_t)__builtin_popcountll(accepted);
  h8_medium_debug_collect_source_add(source, 1u, slots);
  if (slots == 0) {
    H8_DEBUG_INC(medium_collect_zero_slot_run);
  }
  if (!owner || !run || slots == 0) {
    return;
  }
  if (run->debug_collect_free_credits != 0) {
    H8_DEBUG_ADD(medium_collect_credit_outstanding_at_next_collect,
                 run->debug_collect_free_credits);
  }
  if (old_free_mask == 0) {
    H8_DEBUG_INC(medium_collect_full_to_nonfull_run);
    h8_medium_debug_class_inc(run->class_id,
                              &h8g.medium_collect_full_to_nonfull_class_8k,
                              &h8g.medium_collect_full_to_nonfull_class_16k,
                              &h8g.medium_collect_full_to_nonfull_class_32k,
                              &h8g.medium_collect_full_to_nonfull_class_64k);
    h8_medium_debug_collect_full_to_nonfull_source(source);
  }
  ++run->debug_collect_generation;
  run->debug_collect_free_credits += (uint32_t)slots;
  run->debug_collect_owner_alloc_epoch = owner->debug_medium_alloc_epoch;
  H8_DEBUG_ADD(medium_collect_credit_created, slots);
}

void h8_medium_debug_note_alloc_collect_credit(H8OwnerRecord* owner,
                                               H8MediumRun* run) {
  if (!owner || !run || run->debug_collect_free_credits == 0) {
    return;
  }
  --run->debug_collect_free_credits;
  H8_DEBUG_INC(medium_collect_credit_reused);
  uint64_t delta =
      owner->debug_medium_alloc_epoch - run->debug_collect_owner_alloc_epoch;
  bool quick = delta <= 7u;
  if (run->class_id == 0u) {
    H8_DEBUG_INC(medium_collect_credit_reused_class_8k);
    if (quick) {
      H8_DEBUG_INC(medium_collect_credit_quick_class_8k);
    }
  } else if (run->class_id == 1u) {
    H8_DEBUG_INC(medium_collect_credit_reused_class_16k);
    if (quick) {
      H8_DEBUG_INC(medium_collect_credit_quick_class_16k);
    }
  } else if (run->class_id == 2u) {
    H8_DEBUG_INC(medium_collect_credit_reused_class_32k);
    if (quick) {
      H8_DEBUG_INC(medium_collect_credit_quick_class_32k);
    }
  } else if (run->class_id + 1u == H8_MEDIUM_CLASS_COUNT) {
    H8_DEBUG_INC(medium_collect_credit_reused_class_64k);
    if (quick) {
      H8_DEBUG_INC(medium_collect_credit_quick_class_64k);
    }
  }
  if (delta <= 1u) {
    H8_DEBUG_INC(medium_collect_credit_reuse_d0_1);
  } else if (delta <= 3u) {
    H8_DEBUG_INC(medium_collect_credit_reuse_d2_3);
  } else if (delta <= 7u) {
    H8_DEBUG_INC(medium_collect_credit_reuse_d4_7);
  } else if (delta <= 31u) {
    H8_DEBUG_INC(medium_collect_credit_reuse_d8_31);
  } else {
    H8_DEBUG_INC(medium_collect_credit_reuse_d32p);
  }
}

void h8_medium_debug_discard_collect_credit(H8MediumRun* run) {
  if (!run || run->debug_collect_free_credits == 0) {
    return;
  }
  H8_DEBUG_ADD(medium_collect_credit_discarded,
               run->debug_collect_free_credits);
  run->debug_collect_free_credits = 0;
}

static void h8_medium_debug_class_inc(uint32_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k) {
  atomic_size_t* target = NULL;
  if (class_id == 0u) {
    target = class_8k;
  } else if (class_id == 1u) {
    target = class_16k;
  } else if (class_id == 2u) {
    target = class_32k;
  } else if (class_id + 1u == H8_MEDIUM_CLASS_COUNT) {
    target = class_64k;
  }
  if (target) {
    atomic_fetch_add_explicit(target, 1u, memory_order_relaxed);
  }
}
#else
static void h8_medium_debug_class_inc(uint32_t class_id,
                                      atomic_size_t* class_8k,
                                      atomic_size_t* class_16k,
                                      atomic_size_t* class_32k,
                                      atomic_size_t* class_64k) {
  (void)class_id;
  (void)class_8k;
  (void)class_16k;
  (void)class_32k;
  (void)class_64k;
}
#endif

static void h8_medium_lock_run(H8MediumRun* run) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  pthread_mutex_lock(&run->lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_run_lock_wait_ns,
               (size_t)(h8_medium_now_ns() - start));
#endif
}

static void h8_medium_unlock_run(H8MediumRun* run) {
  pthread_mutex_unlock(&run->lock);
}

static uint64_t h8_medium_owner_word_for(const H8OwnerRecord* owner) {
  if (!owner || owner->slot >= H8_OWNER_MAX) {
    return 0;
  }
  H8OwnerWord word = h8_owner_word_make((uint8_t)owner->slot,
                                        (uint16_t)owner->generation,
                                        H8_SPAN_OWNED_ACTIVE, 0);
  return h8_owner_word_pack(word);
}

static bool h8_medium_owner_word_matches_ctx(uint64_t owner_word,
                                             const H8ThreadCtx* ctx) {
  if (!ctx || !ctx->owner) {
    return false;
  }
  uint64_t expected = h8_medium_owner_word_for(ctx->owner);
  return expected != 0 && owner_word == expected;
}

static void h8_medium_debug_lock_elide_candidate(const H8MediumRun* run,
                                                 const H8ThreadCtx* ctx,
                                                 bool is_free) {
#if defined(H8_ENABLE_DEBUG_STATS)
  if (!run || !ctx || !ctx->owner) {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
    return;
  }
  if (h8_medium_run_owned_by_ctx(run, ctx)) {
    if (is_free) {
      H8_DEBUG_INC(medium_lock_elide_free_candidate);
    } else {
      H8_DEBUG_INC(medium_lock_elide_alloc_candidate);
    }
  } else {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
  }
#else
  (void)run;
  (void)ctx;
  (void)is_free;
#endif
}

static void h8_medium_debug_lock_elide_candidate_known(bool same_owner,
                                                       bool is_free) {
#if defined(H8_ENABLE_DEBUG_STATS)
  if (same_owner) {
    if (is_free) {
      H8_DEBUG_INC(medium_lock_elide_free_candidate);
    } else {
      H8_DEBUG_INC(medium_lock_elide_alloc_candidate);
    }
  } else {
    H8_DEBUG_INC(medium_lock_elide_owner_mismatch);
  }
#else
  (void)same_owner;
  (void)is_free;
#endif
}

bool h8_medium_size_supported(size_t size) {
  return h8_medium_size_supported_fast(size);
}

uint32_t h8_medium_class_for_size(size_t size) {
  return h8_medium_class_for_size_fast(size);
}

const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return &k_h8_medium_classes[class_id];
}

uint32_t h8_medium_rounded_size(size_t size) {
  if (!h8_medium_size_supported(size)) {
    return 0u;
  }
  return k_h8_medium_classes[h8_medium_class_for_size(size)].slot_size;
}

static void h8_medium_owner_add_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (!ctx || !ctx->owner || !run || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  h8_medium_detached_remove_locked(run);
  run->owner_attached = true;
  atomic_store_explicit(&run->owner_word,
                        h8_medium_owner_word_for(ctx->owner),
                        memory_order_release);
  run->next_owner = ctx->owner->medium_by_class[run->class_id];
  ctx->owner->medium_by_class[run->class_id] = run;
  h8_medium_available_shadow_attach(ctx->owner, run);
}

static bool h8_medium_run_usable_locked(const H8MediumRun* run,
                                        uint32_t class_id) {
  return run && run->class_id == class_id && run->free_mask != 0 &&
         atomic_load_explicit(&run->state, memory_order_acquire) ==
             H8_MEDIUM_RUN_ACTIVE;
}

#if defined(H8_ENABLE_DEBUG_STATS) || defined(H8_MEDIUM_ENABLE_AVAILABLE_INDEX)
static H8OwnerRecord* h8_medium_available_shadow_owner(const H8MediumRun* run) {
  if (!run || !run->owner_attached) {
    return NULL;
  }
  uint64_t raw = atomic_load_explicit(&run->owner_word, memory_order_acquire);
  if (raw == 0) {
    return NULL;
  }
  H8OwnerWord word = h8_owner_word_unpack(raw);
  H8OwnerRecord* owner = h8_owner_by_slot(word.slot);
  if (!owner || owner->generation != word.generation) {
    return NULL;
  }
  return owner;
}

static bool h8_medium_available_shadow_should_index(const H8MediumRun* run) {
  return run && run->owner_attached && run->class_id < H8_MEDIUM_CLASS_COUNT &&
         run->free_mask != 0 &&
         atomic_load_explicit(&run->state, memory_order_acquire) ==
             H8_MEDIUM_RUN_ACTIVE;
}

void h8_medium_available_shadow_remove(H8OwnerRecord* owner,
                                       H8MediumRun* run) {
  if (!owner || !run || !run->debug_available_indexed) {
    return;
  }
  uint32_t class_id = run->class_id;
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    H8_DEBUG_INC(medium_available_class_mismatch);
    return;
  }
  if (run->debug_available_prev) {
    run->debug_available_prev->debug_available_next =
        run->debug_available_next;
  } else if (owner->medium_available_shadow[class_id] == run) {
    owner->medium_available_shadow[class_id] = run->debug_available_next;
  } else {
    H8_DEBUG_INC(medium_available_owner_mismatch);
  }
  if (run->debug_available_next) {
    run->debug_available_next->debug_available_prev =
        run->debug_available_prev;
  }
  run->debug_available_prev = NULL;
  run->debug_available_next = NULL;
  run->debug_available_indexed = false;
  H8_DEBUG_INC(medium_available_remove);
}

void h8_medium_available_shadow_attach(H8OwnerRecord* owner,
                                       H8MediumRun* run) {
  if (!owner || !run || !h8_medium_available_shadow_should_index(run)) {
    return;
  }
  if (run->debug_available_indexed) {
    return;
  }
  uint32_t class_id = run->class_id;
  H8MediumRun* head = owner->medium_available_shadow[class_id];
  run->debug_available_prev = NULL;
  run->debug_available_next = head;
  if (head) {
    head->debug_available_prev = run;
  }
  owner->medium_available_shadow[class_id] = run;
  run->debug_available_indexed = true;
  H8_DEBUG_INC(medium_available_add);
}

void h8_medium_available_shadow_after_mask_change(H8MediumRun* run) {
  H8OwnerRecord* owner = h8_medium_available_shadow_owner(run);
  if (!owner) {
    if (run && run->debug_available_indexed) {
      H8_DEBUG_INC(medium_available_owner_mismatch);
    }
    return;
  }
  if (h8_medium_available_shadow_should_index(run)) {
    h8_medium_available_shadow_attach(owner, run);
  } else {
    if (run && run->debug_available_indexed) {
      h8_medium_available_shadow_remove(owner, run);
    }
    if (run && run->free_mask == 0 && run->debug_available_indexed) {
      H8_DEBUG_INC(medium_available_indexed_full);
    }
  }
}

void h8_medium_available_shadow_after_mask_change_ctx(H8MediumRun* run,
                                                      H8ThreadCtx* ctx) {
  if (ctx && run && run->class_id < H8_MEDIUM_CLASS_COUNT &&
      ctx->owner && ctx->active_medium_runs[run->class_id] == run &&
      h8_medium_run_owned_by_ctx(run, ctx)) {
    h8_medium_available_shadow_remove(ctx->owner, run);
    return;
  }
  h8_medium_available_shadow_after_mask_change(run);
}

static void h8_medium_available_shadow_probe(H8ThreadCtx* ctx,
                                             uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* run = ctx->owner->medium_available_shadow[class_id];
  if (!run) {
    return;
  }
  H8_DEBUG_INC(medium_available_head_attempt);
  if (ctx->active_medium_runs[class_id] == run) {
    H8_DEBUG_INC(medium_available_active_indexed);
    h8_medium_available_shadow_remove(ctx->owner, run);
    H8_DEBUG_INC(medium_available_head_unusable);
    return;
  }
  if (!h8_medium_run_owned_by_ctx(run, ctx)) {
    H8_DEBUG_INC(medium_available_owner_mismatch);
    H8_DEBUG_INC(medium_available_head_unusable);
    return;
  }
  if (run->class_id != class_id) {
    H8_DEBUG_INC(medium_available_class_mismatch);
    H8_DEBUG_INC(medium_available_head_unusable);
    return;
  }
  if (!run->owner_attached) {
    H8_DEBUG_INC(medium_available_indexed_detached);
    H8_DEBUG_INC(medium_available_head_unusable);
    return;
  }
  if (atomic_load_explicit(&run->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(medium_available_indexed_nonactive);
    H8_DEBUG_INC(medium_available_head_unusable);
    return;
  }
  if (run->free_mask == 0) {
    H8_DEBUG_INC(medium_available_indexed_full);
    H8_DEBUG_INC(medium_available_head_unusable);
    return;
  }
  H8_DEBUG_INC(medium_available_head_hit);
}

static void h8_medium_available_shadow_note_owner_hit(H8ThreadCtx* ctx,
                                                      uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* run = ctx->owner->medium_available_shadow[class_id];
  if (!run || !h8_medium_run_owned_by_ctx(run, ctx) ||
      run->class_id != class_id || !run->owner_attached ||
      run->free_mask == 0 ||
      atomic_load_explicit(&run->state, memory_order_acquire) !=
          H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(medium_available_owner_list_hit_without_available);
  }
}
#else
#define h8_medium_available_shadow_probe(ctx, class_id) \
  do {                                                  \
    (void)(ctx);                                        \
    (void)(class_id);                                   \
  } while (0)
#define h8_medium_available_shadow_note_owner_hit(ctx, class_id) \
  do {                                                           \
    (void)(ctx);                                                 \
    (void)(class_id);                                            \
  } while (0)
#endif

static void h8_medium_demote_active_empty_live(H8MediumRun* run) {
  if (!run || pthread_mutex_trylock(&run->lock) != 0) {
    return;
  }
  uint64_t pending =
      atomic_load_explicit(&run->pending_bits[0], memory_order_acquire);
  if (run->allocated_mask == 0 && pending == 0 &&
      run->payload_state == H8_MEDIUM_PAYLOAD_LIVE) {
    h8_medium_mark_empty_locked(run);
  }
  h8_medium_unlock_run(run);
}

static void h8_medium_set_active_run(H8ThreadCtx* ctx, uint32_t class_id,
                                     H8MediumRun* run) {
  if (!ctx || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* old = ctx->active_medium_runs[class_id];
  if (old != run) {
    h8_medium_debug_class_inc(class_id, &h8g.medium_active_switch_class_8k,
                              &h8g.medium_active_switch_class_16k,
                              &h8g.medium_active_switch_class_32k,
                              &h8g.medium_active_switch_class_64k);
    if (h8_medium_run_owned_by_ctx(old, ctx)) {
      h8_medium_demote_active_empty_live(old);
      h8_medium_available_shadow_after_mask_change(old);
    }
    if (run && h8_medium_run_owned_by_ctx(run, ctx)) {
      h8_medium_available_shadow_remove(ctx->owner, run);
    }
  }
  ctx->active_medium_runs[class_id] = run;
}

#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
static void h8_medium_record_alloc_run(H8ThreadCtx* ctx, H8MediumRun* run) {
  if (ctx) {
    ctx->medium_last_alloc_run = run;
  }
}
#else
#define h8_medium_record_alloc_run(ctx, run) \
  do {                                      \
    (void)(ctx);                            \
    (void)(run);                            \
  } while (0)
#endif

#if defined(H8_MEDIUM_ENABLE_AVAILABLE_INDEX)
static void* h8_medium_try_available_index(H8ThreadCtx* ctx,
                                           uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8MediumRun* run = ctx->owner->medium_available_shadow[class_id];
  if (!run) {
    return NULL;
  }
  if (ctx->active_medium_runs[class_id] == run) {
    H8_DEBUG_INC(medium_available_active_indexed);
    h8_medium_available_shadow_remove(ctx->owner, run);
    return NULL;
  }
  h8_medium_debug_writer_enter(run, ctx->owner,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t section_start = h8_medium_now_ns();
#endif
  h8_medium_lock_run(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_lock_ns,
               (size_t)(h8_medium_now_ns() - section_start));
#endif
  if (!h8_medium_run_owned_by_ctx(run, ctx) ||
      !h8_medium_run_usable_locked(run, class_id)) {
    h8_medium_unlock_run(run);
    h8_medium_debug_writer_exit(run);
    return NULL;
  }
  h8_medium_debug_lock_elide_candidate(run, ctx, false);
  H8_DEBUG_INC(medium_run_reuse_owner_list_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_run_reuse_owner_class_8k,
                            &h8g.medium_run_reuse_owner_class_16k,
                            &h8g.medium_run_reuse_owner_class_32k,
                            &h8g.medium_run_reuse_owner_class_64k);
#if defined(H8_ENABLE_DEBUG_STATS)
  section_start = h8_medium_now_ns();
#endif
  void* ptr = h8_medium_run_alloc_local_hot(run);
  if (ptr) {
    h8_medium_debug_note_alloc_collect_credit(ctx->owner, run);
  }
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_alloc_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  section_start = h8_medium_now_ns();
#endif
  h8_medium_set_active_run(ctx, class_id, run);
  h8_medium_record_alloc_run(ctx, run);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_active_ns,
               (size_t)(h8_medium_now_ns() - section_start));
#endif
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
#if defined(H8_ENABLE_DEBUG_STATS)
  section_start = h8_medium_now_ns();
#endif
  h8_medium_collect_owner_pending_periodic_owner_list(ctx);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_available_hit_collect_ns,
               (size_t)(h8_medium_now_ns() - section_start));
  H8_DEBUG_INC(medium_available_hit_reuse);
#endif
  return ptr;
}
#else
#define h8_medium_try_available_index(ctx, class_id) \
  ((void)(ctx), (void)(class_id), (void*)NULL)
#endif

#if defined(H8_ENABLE_DEBUG_STATS)
static void h8_medium_refill_candidate_shadow_probe(H8ThreadCtx* ctx,
                                                    uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  H8MediumRun* run = ctx->owner->medium_refill_candidate[class_id];
  if (!run) {
    return;
  }
  H8_DEBUG_INC(medium_refill_candidate_attempt);
  h8_medium_lock_run(run);
  bool owner_match = h8_medium_run_owned_by_ctx(run, ctx);
  bool usable = owner_match && h8_medium_run_usable_locked(run, class_id);
  h8_medium_unlock_run(run);
  if (!owner_match) {
    H8_DEBUG_INC(medium_refill_candidate_owner_mismatch);
  } else if (!usable) {
    H8_DEBUG_INC(medium_refill_candidate_unusable);
  } else {
    H8_DEBUG_INC(medium_refill_candidate_hit);
  }
}
#else
#define h8_medium_refill_candidate_shadow_probe(ctx, class_id) \
  do {                                                        \
    (void)(ctx);                                              \
    (void)(class_id);                                         \
  } while (0)
#endif

#if defined(H8_MEDIUM_ENABLE_REFILL_CANDIDATE)
static void* h8_medium_try_refill_candidate(H8ThreadCtx* ctx,
                                            uint32_t class_id) {
  if (!ctx || !ctx->owner || class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8MediumRun* run = ctx->owner->medium_refill_candidate[class_id];
  if (!run) {
    return NULL;
  }
  h8_medium_debug_writer_enter(run, ctx->owner,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
  h8_medium_lock_run(run);
  if (!h8_medium_run_owned_by_ctx(run, ctx) ||
      !h8_medium_run_usable_locked(run, class_id)) {
    h8_medium_unlock_run(run);
    h8_medium_debug_writer_exit(run);
    return NULL;
  }
  h8_medium_debug_lock_elide_candidate(run, ctx, false);
  void* ptr = h8_medium_run_alloc_local_hot(run);
  if (ptr) {
    h8_medium_debug_note_alloc_collect_credit(ctx->owner, run);
  }
  h8_medium_set_active_run(ctx, class_id, run);
  h8_medium_record_alloc_run(ctx, run);
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  H8_DEBUG_INC(medium_run_reuse_refill_candidate_count);
  h8_medium_collect_owner_pending_periodic_owner_list(ctx);
  return ptr;
}
#else
#define h8_medium_try_refill_candidate(ctx, class_id) \
  ((void)(ctx), (void)(class_id), (void*)NULL)
#endif

#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
static bool h8_medium_try_cached_local_free(H8ThreadCtx* ctx, void* ptr,
                                            bool* owned_out) {
  H8MediumRun* cached = ctx ? ctx->medium_last_alloc_run : NULL;
  if (!cached) {
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_attempt);

  uintptr_t base = (uintptr_t)cached->base;
  uintptr_t addr = (uintptr_t)ptr;
  size_t payload = (size_t)cached->slot_size * (size_t)cached->slot_count;
  if (addr < base || addr >= base + payload) {
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_range_hit);

  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked_fast(cached, ptr, &slot)) {
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_slot_hit);

  uint64_t owner_word =
      atomic_load_explicit(&cached->owner_word, memory_order_acquire);
  if (!h8_medium_owner_word_matches_ctx(owner_word, ctx)) {
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
  H8_DEBUG_INC(medium_free_cache_owner_hit);

  uint64_t bit = UINT64_C(1) << slot;
  if ((cached->allocated_mask & bit) == 0 || (cached->free_mask & bit) != 0) {
    H8_DEBUG_INC(medium_free_cache_state_block);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }

  uint64_t pending =
      atomic_load_explicit(&cached->pending_bits[0], memory_order_acquire);
  if ((pending & bit) != 0) {
    H8_DEBUG_INC(medium_free_cache_pending_block);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }

  uint32_t state =
      atomic_load_explicit(&cached->slot_state[slot], memory_order_acquire);
  if (h8_slot_state_tag(state) != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
    H8_DEBUG_INC(medium_free_cache_state_block);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }

#if defined(H8_ENABLE_DEBUG_STATS)
  H8MediumRun* directory_run = h8_medium_directory_find(ptr);
  if (directory_run != cached) {
    H8_DEBUG_INC(medium_free_cache_directory_mismatch);
    H8_DEBUG_INC(medium_free_cache_fallback);
    return false;
  }
#endif

  H8_DEBUG_INC(medium_free_cache_would_succeed);

  H8_DEBUG_INC(medium_local_free_owner_match);
  h8_medium_debug_lock_elide_candidate_known(true, true);
  h8_medium_debug_writer_enter(cached, ctx ? ctx->owner : NULL,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_FREE);
  if (owned_out) {
    *owned_out = true;
  }
  bool keep_empty_live = cached->class_id < H8_MEDIUM_CLASS_COUNT &&
                         ctx->active_medium_runs[cached->class_id] == cached;
  bool ok = h8_medium_run_free_local_scaffold(cached, ptr, keep_empty_live);
  if (!ok) {
    H8_DEBUG_INC(medium_invalid_owned_count);
  }
  if (ok && cached->class_id < H8_MEDIUM_CLASS_COUNT) {
    h8_medium_debug_class_inc(cached->class_id,
                              &h8g.medium_local_free_class_8k,
                              &h8g.medium_local_free_class_16k,
                              &h8g.medium_local_free_class_32k,
                              &h8g.medium_local_free_class_64k);
    h8_medium_set_active_run(ctx, cached->class_id, cached);
  }
  h8_medium_debug_writer_exit(cached);
  return ok;
}
#endif

H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec) {
    return NULL;
  }
  H8MediumRun* run = h8_sys_calloc(1, sizeof(*run));
  if (!run) {
    return NULL;
  }
  pthread_mutex_init(&run->lock, NULL);
  run->slot_state = h8_sys_calloc(spec->slot_count, sizeof(*run->slot_state));
  run->pending_bits = h8_sys_calloc(spec->bitmap_words, sizeof(*run->pending_bits));
  if (!run->slot_state || !run->pending_bits) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  bool chunk_backed = false;
  void* payload = h8_medium_payload_alloc(spec->run_size, &chunk_backed);
  if (!payload) {
    h8_medium_run_destroy_scaffold(run);
    return NULL;
  }
  run->base = payload;
  run->payload_chunk_backed = chunk_backed;
  run->class_id = (uint16_t)class_id;
  run->slot_size = spec->slot_size;
  run->slot_shift = spec->slot_shift;
  run->slot_count = spec->slot_count;
  run->run_size = spec->run_size;
  run->free_mask = (run->slot_count == 64u)
                       ? UINT64_MAX
                       : ((UINT64_C(1) << run->slot_count) - 1u);
  run->allocated_mask = 0;
  run->payload_state = H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED;
  atomic_store_explicit(&run->state, H8_MEDIUM_RUN_ACTIVE,
                        memory_order_relaxed);
  atomic_store_explicit(&run->qstate, H8_Q_IDLE, memory_order_relaxed);
  atomic_store_explicit(&run->pending_word_mask, 0, memory_order_relaxed);
  for (uint16_t i = 0; i < run->slot_count; ++i) {
    atomic_store_explicit(&run->slot_state[i], H8_SLOT_FREE | H8_SLOT_NONE,
                          memory_order_relaxed);
  }
  return run;
}

void h8_medium_run_destroy_scaffold(H8MediumRun* run) {
  if (!run) {
    return;
  }
  h8_medium_debug_discard_collect_credit(run);
  h8_medium_lock_global();
  h8_medium_unregister_locked(run);
  h8_medium_unlock_global();
  if (run->base) {
    h8_medium_release_empty_payload(run);
    if (run->lazy_purge_charge) {
      H8_DEBUG_INC(medium_lazy_drop_destroy);
    }
    h8_medium_lazy_purge_shadow_drop(run);
    h8_medium_payload_free(run->base,
                           run->run_size ? run->run_size
                                         : H8_MEDIUM_RUN_BYTES,
                           run->payload_chunk_backed);
  }
  h8_sys_free(run->pending_bits);
  h8_sys_free(run->slot_state);
  pthread_mutex_destroy(&run->lock);
  h8_sys_free(run);
}

static void* h8_medium_run_alloc_active_hit(H8MediumRun* run,
                                            uint32_t class_id) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_now_ns();
#endif
  if (!run || run->class_id != class_id) {
    return NULL;
  }
  if (run->free_mask == 0) {
    H8_DEBUG_INC(medium_alloc_free_mask_zero);
    return NULL;
  }
  if (atomic_load_explicit(&run->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    H8_DEBUG_INC(medium_alloc_state_check_fail);
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(run->free_mask);
  uint64_t bit = UINT64_C(1) << slot;
  h8_medium_mark_live_on_alloc_fast(run);
  run->free_mask &= ~bit;
  run->allocated_mask |= bit;
  atomic_store_explicit(&run->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_INC(medium_alloc_slot_count);
  H8_DEBUG_ADD(medium_alloc_slot_ns, (size_t)(h8_medium_now_ns() - start));
#endif
  return h8_medium_slot_ptr_known(run, slot);
}

#if defined(H8_MEDIUM_ENABLE_ACTIVE_MISS_DEMAND_COLLECT_64K)
static void* h8_medium_try_active_miss_demand_collect64(H8ThreadCtx* ctx,
                                                        uint32_t class_id,
                                                        H8MediumRun* active) {
  if (!ctx || !ctx->owner || !active ||
      class_id + 1u != H8_MEDIUM_CLASS_COUNT ||
      !h8_medium_run_owned_by_ctx(active, ctx)) {
    return NULL;
  }
  if (atomic_load_explicit(&active->state, memory_order_acquire) !=
      H8_MEDIUM_RUN_ACTIVE) {
    return NULL;
  }
  if (active->free_mask != 0) {
    return NULL;
  }
  uint64_t valid = active->slot_count == 64u
                       ? UINT64_MAX
                       : ((UINT64_C(1) << active->slot_count) - 1u);
  uint64_t pending =
      atomic_load_explicit(&active->pending_bits[0], memory_order_acquire) &
      valid;
  if (!pending) {
    return NULL;
  }
  uint8_t qstate = atomic_load_explicit(&active->qstate, memory_order_acquire);
  if (qstate != H8_Q_QUEUED) {
    H8_DEBUG_INC(medium_demand64_qstate_not_queued);
    return NULL;
  }
  if (!h8_medium_owner_has_pending(ctx->owner)) {
    return NULL;
  }

  H8_DEBUG_INC(medium_demand64_trigger);
  size_t processed_total = 0;
  for (unsigned i = 0; i < 2u; ++i) {
    size_t processed = h8_medium_collect_current_pending_budget_source(
        ctx, 1u, H8_MEDIUM_COLLECT_ACTIVE_MISS_DEMAND);
    processed_total += processed;
    if (processed == 0 || active->free_mask != 0) {
      break;
    }
  }
  if (processed_total == 0) {
    H8_DEBUG_INC(medium_demand64_processed_0);
  } else if (processed_total == 1u) {
    H8_DEBUG_INC(medium_demand64_processed_1);
  } else {
    H8_DEBUG_INC(medium_demand64_processed_2);
  }

  if (active->free_mask == 0) {
    H8_DEBUG_INC(medium_demand64_target_not_reached);
    H8_DEBUG_INC(medium_demand64_owner_list_fallback);
    return NULL;
  }

  H8_DEBUG_INC(medium_demand64_target_opened);
  H8_DEBUG_ADD(medium_demand64_active_slots_created,
               (size_t)__builtin_popcountll(active->free_mask & valid));
  h8_medium_debug_writer_enter(active, ctx->owner,
                               H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
  void* ptr = h8_medium_run_alloc_active_hit(active, class_id);
  h8_medium_debug_writer_exit(active);
  if (!ptr) {
    H8_DEBUG_INC(medium_demand64_owner_list_fallback);
    return NULL;
  }

  H8_DEBUG_INC(medium_demand64_retry_hit);
  H8_DEBUG_INC(medium_demand64_periodic_tick_replaced);
  h8_medium_debug_note_alloc_collect_credit(ctx->owner, active);
  h8_medium_record_alloc_run(ctx, active);
  H8_DEBUG_INC(medium_run_reuse_active_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_run_reuse_active_class_8k,
                            &h8g.medium_run_reuse_active_class_16k,
                            &h8g.medium_run_reuse_active_class_32k,
                            &h8g.medium_run_reuse_active_class_64k);
  return ptr;
}
#else
#define h8_medium_try_active_miss_demand_collect64(ctx, class_id, active) \
  ((void)(ctx), (void)(class_id), (void)(active), (void*)NULL)
#endif

void* h8_medium_malloc_class_inner(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H8_DEBUG_INC(medium_malloc_count);
  h8_medium_debug_class_inc(class_id, &h8g.medium_malloc_class_8k,
                            &h8g.medium_malloc_class_16k,
                            &h8g.medium_malloc_class_32k,
                            &h8g.medium_malloc_class_64k);
  H8ThreadCtx* ctx = h8_thread_ctx_fast();
  h8_medium_debug_note_owner_medium_alloc(ctx ? ctx->owner : NULL);
  bool did_capacity_collect = false;
  bool saw_active_miss = false;
retry_owner_capacity:
  H8MediumRun* active = ctx ? ctx->active_medium_runs[class_id] : NULL;
  uint64_t expected_owner_word =
      (ctx && ctx->owner) ? h8_medium_owner_word_for(ctx->owner) : 0;
  bool active_owned = false;
  if (!active) {
    H8_DEBUG_INC(medium_active_miss_null);
    saw_active_miss = true;
  } else {
    active_owned =
        expected_owner_word != 0 &&
        atomic_load_explicit(&active->owner_word, memory_order_acquire) ==
            expected_owner_word;
    if (!active_owned) {
      H8_DEBUG_INC(medium_active_alloc_owner_mismatch);
      H8_DEBUG_INC(medium_active_miss_owner);
      h8_medium_set_active_run(ctx, class_id, NULL);
      active = NULL;
      saw_active_miss = true;
    }
  }
  if (active) {
    if (active_owned) {
      h8_medium_debug_lock_elide_candidate_known(true, false);
      h8_medium_debug_writer_enter(active, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      void* ptr = h8_medium_run_alloc_active_hit(active, class_id);
      h8_medium_debug_writer_exit(active);
      if (ptr) {
        h8_medium_debug_note_alloc_collect_credit(ctx ? ctx->owner : NULL,
                                                  active);
        h8_medium_record_alloc_run(ctx, active);
        H8_DEBUG_INC(medium_run_reuse_active_count);
        h8_medium_debug_class_inc(class_id,
                                  &h8g.medium_run_reuse_active_class_8k,
                                  &h8g.medium_run_reuse_active_class_16k,
                                  &h8g.medium_run_reuse_active_class_32k,
                                  &h8g.medium_run_reuse_active_class_64k);
        h8_medium_collect_owner_pending_periodic(ctx);
        return ptr;
      }
    }
    H8_DEBUG_INC(medium_active_miss_unusable);
    saw_active_miss = true;
    h8_medium_debug_note_active_miss_pending(ctx, class_id, active);
    void* demand_ptr =
        h8_medium_try_active_miss_demand_collect64(ctx, class_id, active);
    if (demand_ptr) {
      return demand_ptr;
    }
  }
  h8_medium_refill_candidate_shadow_probe(ctx, class_id);
  void* refill_ptr = h8_medium_try_refill_candidate(ctx, class_id);
  if (refill_ptr) {
    return refill_ptr;
  }
  h8_medium_available_shadow_probe(ctx, class_id);
  void* available_ptr = h8_medium_try_available_index(ctx, class_id);
  if (available_ptr) {
    return available_ptr;
  }
  if (ctx && ctx->owner) {
    H8_DEBUG_INC(medium_owner_scan_count);
    size_t owner_scan_position = 0;
    for (H8MediumRun* run = ctx->owner->medium_by_class[class_id]; run;
         run = run->next_owner) {
      H8_DEBUG_INC(medium_owner_scan_step_count);
      ++owner_scan_position;
      if (!h8_medium_run_owned_by_ctx(run, ctx)) {
        H8_DEBUG_INC(medium_owner_list_owner_mismatch);
        continue;
      }
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_ALLOC);
      h8_medium_lock_run(run);
      if (h8_medium_run_owned_by_ctx(run, ctx) &&
          h8_medium_run_usable_locked(run, class_id)) {
        h8_medium_debug_lock_elide_candidate(run, ctx, false);
        h8_medium_available_shadow_note_owner_hit(ctx, class_id);
        H8_DEBUG_INC(medium_run_reuse_owner_list_count);
        h8_medium_debug_class_inc(class_id,
                                  &h8g.medium_run_reuse_owner_class_8k,
                                  &h8g.medium_run_reuse_owner_class_16k,
                                  &h8g.medium_run_reuse_owner_class_32k,
                                  &h8g.medium_run_reuse_owner_class_64k);
        void* ptr = h8_medium_run_alloc_local_hot(run);
        if (ptr) {
          h8_medium_debug_note_owner_list_hit_position(owner_scan_position);
          h8_medium_debug_note_owner_list_hit_class(class_id);
          h8_medium_debug_note_alloc_collect_credit(ctx->owner, run);
        }
        h8_medium_set_active_run(ctx, class_id, run);
        h8_medium_record_alloc_run(ctx, run);
        h8_medium_unlock_run(run);
        h8_medium_debug_writer_exit(run);
        h8_medium_collect_owner_pending_periodic_owner_list(ctx);
        return ptr;
      }
      if (!h8_medium_run_owned_by_ctx(run, ctx)) {
        H8_DEBUG_INC(medium_owner_list_owner_mismatch);
      }
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
    }
  }
  if (ctx && ctx->owner && !did_capacity_collect &&
      h8_medium_owner_has_pending(ctx->owner)) {
    did_capacity_collect = true;
    (void)h8_medium_collect_current_pending_budget(ctx, SIZE_MAX);
    goto retry_owner_capacity;
  }
  h8_medium_lock_global();
  H8_DEBUG_INC(medium_global_scan_count);
  for (H8MediumRun* run = h8_medium_detached_head_locked(class_id); run;
       run = run->next_detached) {
    H8_DEBUG_INC(medium_global_scan_step_count);
    h8_medium_lock_run(run);
    if (run->class_id != class_id || run->owner_attached ||
        atomic_load_explicit(&run->owner_word, memory_order_acquire) != 0) {
      H8_DEBUG_INC(medium_global_skip_foreign_attached);
      h8_medium_unlock_run(run);
      continue;
    }
    if (h8_medium_run_usable_locked(run, class_id)) {
      H8_DEBUG_INC(medium_run_reuse_global_count);
      if (saw_active_miss) {
        H8_DEBUG_INC(medium_active_miss_detached_reuse);
      }
      h8_medium_debug_class_inc(class_id,
                                &h8g.medium_run_reuse_global_class_8k,
                                &h8g.medium_run_reuse_global_class_16k,
                                &h8g.medium_run_reuse_global_class_32k,
                                &h8g.medium_run_reuse_global_class_64k);
      if (ctx && ctx->owner) {
        h8_medium_owner_add_run(ctx, run);
      }
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_GLOBAL_ATTACH);
      void* ptr = h8_medium_run_alloc_local_hot(run);
      h8_medium_debug_note_alloc_collect_credit(ctx ? ctx->owner : NULL, run);
      h8_medium_debug_writer_exit(run);
      if (ctx && h8_medium_run_owned_by_ctx(run, ctx)) {
        h8_medium_set_active_run(ctx, class_id, run);
        h8_medium_record_alloc_run(ctx, run);
      }
      h8_medium_unlock_run(run);
      h8_medium_unlock_global();
      return ptr;
    }
    h8_medium_unlock_run(run);
  }
  h8_medium_unlock_global();

  H8MediumRun* run = h8_medium_run_create_scaffold(class_id);
  if (!run) {
    return NULL;
  }
  H8_DEBUG_INC(medium_run_create_count);
  if (saw_active_miss) {
    H8_DEBUG_INC(medium_active_miss_create);
  }
  h8_medium_debug_class_inc(class_id, &h8g.medium_run_create_class_8k,
                            &h8g.medium_run_create_class_16k,
                            &h8g.medium_run_create_class_32k,
                            &h8g.medium_run_create_class_64k);
  h8_medium_lock_global();
  h8_medium_register_locked(run);
  h8_medium_owner_add_run(ctx, run);
  h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                               H8_MEDIUM_WRITER_GLOBAL_ATTACH);
  h8_medium_lock_run(run);
  void* ptr = h8_medium_run_alloc_local_hot(run);
  h8_medium_debug_note_alloc_collect_credit(ctx ? ctx->owner : NULL, run);
  if (ctx) {
    h8_medium_set_active_run(ctx, class_id, run);
    h8_medium_record_alloc_run(ctx, run);
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  h8_medium_unlock_global();
  return ptr;
}

void* h8_medium_malloc_inner(size_t size) {
  if (!h8_medium_size_supported_fast(size)) {
    return NULL;
  }
  return h8_medium_malloc_class_inner(h8_medium_class_for_size_fast(size));
}

bool h8_medium_free_inner(void* ptr, bool* owned_out) {
  if (owned_out) {
    *owned_out = false;
  }
  H8ThreadCtx* ctx = h8_tls_ctx;
#if defined(H8_MEDIUM_ENABLE_LOCAL_FREE_CACHE)
  if (h8_medium_try_cached_local_free(ctx, ptr, owned_out)) {
    return true;
  }
#else
  (void)ctx;
#endif

  H8_DEBUG_INC(medium_free_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    uint64_t owner_word =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    bool same_owner = h8_medium_owner_word_matches_ctx(owner_word, ctx);
    if (!same_owner && owner_word != 0) {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        sched_yield();
      }
    }
    h8_medium_debug_lock_elide_candidate_known(same_owner, true);
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
      h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                   H8_MEDIUM_WRITER_OWNER_LOCAL_FREE);
      if (owned_out) {
        *owned_out = true;
      }
      bool keep_empty_live =
          ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
          ctx->active_medium_runs[run->class_id] == run;
      bool ok = h8_medium_run_free_local_scaffold(run, ptr, keep_empty_live);
      if (!ok) {
        H8_DEBUG_INC(medium_invalid_owned_count);
      }
      if (ok) {
        h8_medium_debug_class_inc(run->class_id,
                                  &h8g.medium_local_free_class_8k,
                                  &h8g.medium_local_free_class_16k,
                                  &h8g.medium_local_free_class_32k,
                                  &h8g.medium_local_free_class_64k);
      }
      if (ok && ctx && run->class_id < H8_MEDIUM_CLASS_COUNT) {
        h8_medium_set_active_run(ctx, run->class_id, run);
      }
      h8_medium_debug_writer_exit(run);
      return ok;
    }
    H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    h8_medium_debug_writer_enter(run, ctx ? ctx->owner : NULL,
                                 H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    ctx = NULL;
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, false);
    if (!run) {
      h8_medium_unlock_global();
      return false;
    }
    uint64_t owner_word =
        atomic_load_explicit(&run->owner_word, memory_order_acquire);
    bool same_owner = h8_medium_owner_word_matches_ctx(owner_word, ctx);
    if (!same_owner && owner_word != 0) {
      h8_medium_unlock_global();
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
      if (owned_out) { *owned_out = true; }
      for (;;) {
        H8PublishResult res = h8_medium_remote_publish(run, ptr);
        if (res == H8_PUBLISH_OK) {
          return true;
        }
        if (res != H8_PUBLISH_OWNER_TRANSITION) {
          return false;
        }
        sched_yield();
      }
    }
    if (same_owner) {
      H8_DEBUG_INC(medium_local_free_owner_match);
    } else {
      H8_DEBUG_INC(medium_remote_free_owner_mismatch);
    }
    h8_medium_debug_writer_enter(
        run, ctx ? ctx->owner : NULL,
        same_owner ? H8_MEDIUM_WRITER_OWNER_LOCAL_FREE
                   : H8_MEDIUM_WRITER_DETACHED_DIRECT_FREE);
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
    if (!same_owner) {
      ctx = NULL;
    }
  }
  if (owned_out) {
    *owned_out = true;
  }
  bool keep_empty_live =
      ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
      ctx->active_medium_runs[run->class_id] == run;
  bool ok = h8_medium_run_free_local_scaffold(run, ptr, keep_empty_live);
  if (!ok) {
    H8_DEBUG_INC(medium_invalid_owned_count);
  }
  if (ok) {
    if (ctx && run->class_id < H8_MEDIUM_CLASS_COUNT &&
        ctx->active_medium_runs[run->class_id] == run) {
      h8_medium_debug_class_inc(run->class_id,
                                &h8g.medium_local_free_class_8k,
                                &h8g.medium_local_free_class_16k,
                                &h8g.medium_local_free_class_32k,
                                &h8g.medium_local_free_class_64k);
      h8_medium_set_active_run(ctx, run->class_id, run);
    }
  }
  h8_medium_unlock_run(run);
  h8_medium_debug_writer_exit(run);
  return ok;
}

H8RouteKind h8_medium_route_inner(void* ptr) {
  H8_DEBUG_INC(medium_route_lookup_count);
  H8MediumRun* run = h8_medium_directory_find(ptr);
  if (run) {
    h8_medium_lock_run(run);
  } else {
    h8_medium_lock_global();
    run = h8_medium_find_run_locked(ptr, true);
    if (!run) {
      h8_medium_unlock_global();
      return H8_ROUTE_MISS;
    }
    h8_medium_lock_run(run);
    h8_medium_unlock_global();
  }
  size_t slot = 0;
  if (!h8_medium_slot_index_from_ptr_checked(run, ptr, &slot)) {
    h8_medium_unlock_run(run);
    return H8_ROUTE_INVALID;
  }
  uint64_t bit = UINT64_C(1) << slot;
  bool pending = (atomic_load_explicit(&run->pending_bits[0],
                                       memory_order_acquire) &
                  bit) != 0;
  uint32_t state =
      atomic_load_explicit(&run->slot_state[slot], memory_order_acquire);
  bool slot_authority_valid =
      h8_slot_state_tag(state) == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT) &&
      !pending;
#if defined(H8_ENABLE_DEBUG_STATS)
  bool mask_authority_valid =
      (run->allocated_mask & bit) != 0 && (run->free_mask & bit) == 0 &&
      !pending;
  if (slot_authority_valid != mask_authority_valid) {
    H8_DEBUG_INC(medium_route_authority_mismatch);
  }
#endif
  H8RouteKind route = slot_authority_valid ? H8_ROUTE_VALID : H8_ROUTE_INVALID;
  h8_medium_unlock_run(run);
  return route;
}

void h8_medium_owner_detach_all(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  h8_medium_collect_owner_pending(owner);
  h8_medium_lock_global();
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    H8MediumRun* run = owner->medium_by_class[c];
    owner->medium_by_class[c] = NULL;
    while (run) {
      H8MediumRun* next = run->next_owner;
      h8_medium_debug_writer_enter(run, owner, H8_MEDIUM_WRITER_OWNER_DETACH);
      h8_medium_lock_run(run);
      if (run->allocated_mask == 0 &&
          run->payload_state != H8_MEDIUM_PAYLOAD_EMPTY_DECOMMITTED) {
        H8_DEBUG_INC(medium_owner_exit_drain_count);
        h8_medium_decommit_empty_owner_exit_locked(run);
        if (run->active_live_empty_charge) {
          H8_DEBUG_INC(medium_owner_exit_active_live_remaining);
        }
      }
      if (run->lazy_purge_charge) {
        H8_DEBUG_INC(medium_lazy_drop_detach);
      }
      h8_medium_debug_discard_collect_credit(run);
      h8_medium_available_shadow_remove(owner, run);
      h8_medium_lazy_purge_shadow_drop(run);
      run->owner_attached = false;
      atomic_store_explicit(&run->owner_word, 0, memory_order_release);
      h8_medium_unlock_run(run);
      h8_medium_debug_writer_exit(run);
      run->next_owner = NULL;
      h8_medium_detached_add_locked(run);
      run = next;
    }
  }
  h8_medium_unlock_global();
}
