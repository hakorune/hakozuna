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
#if defined(H8_MEDIUM_V12_48K2_CLASS)
    {24576u, H8_MEDIUM_RUN_BYTES, 13u, 2u, 1u},
    {32768u, H8_MEDIUM_RUN_BYTES, 15u, 2u, 1u},
    {49152u, 2u * H8_MEDIUM_QUANTUM_BYTES, 14u, 2u, 1u},
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#else
    {32768u, H8_MEDIUM_RUN_BYTES, 15u, 2u, 1u},
#if defined(H8_MEDIUM_UPPER48_CLASS)
    {49152u, H8_MEDIUM_RUN_BYTES, 14u, 1u, 1u},
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#else
    {65536u, H8_MEDIUM_64K_RUN_BYTES, 16u, H8_MEDIUM_64K_SLOT_COUNT, 1u},
#endif
#endif
};

_Static_assert(H8_MEDIUM_MIN_SIZE == 4097u,
               "medium range must start immediately after small");
_Static_assert(H8_MEDIUM_MAX_SIZE == 65536u,
               "medium v1 scaffold currently ends at 64KiB");
#if defined(H8_MEDIUM_V12_48K2_CLASS)
_Static_assert(H8_MEDIUM_CLASS_COUNT == 6u,
               "medium v12 48k2 candidate expects six classes");
#elif defined(H8_MEDIUM_UPPER48_CLASS)
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
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  } else if (class_id == 2u || class_id == 3u) {
    target = class_32k;
  } else if (class_id == 4u || class_id == 5u) {
    target = class_64k;
#else
  } else if (class_id == 2u) {
    target = class_32k;
  } else if (class_id + 1u == H8_MEDIUM_CLASS_COUNT) {
    target = class_64k;
#endif
  }
  if (target) {
    atomic_fetch_add_explicit(target, 1u, memory_order_relaxed);
  }
}

#if defined(H8_MEDIUM_V12_48K2_CLASS)
static inline __attribute__((unused)) void h8_medium_debug_v12_class_inc(
    uint32_t class_id, atomic_size_t* class_24k, atomic_size_t* class_48k) {
  atomic_size_t* target = NULL;
  if (class_id == 2u) {
    target = class_24k;
  } else if (class_id == 4u) {
    target = class_48k;
  }
  if (target) {
    atomic_fetch_add_explicit(target, 1u, memory_order_relaxed);
  }
}
#else
static inline __attribute__((unused)) void h8_medium_debug_v12_class_inc(
    uint32_t class_id, atomic_size_t* class_24k, atomic_size_t* class_48k) {
  (void)class_id;
  (void)class_24k;
  (void)class_48k;
}
#endif
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

static inline __attribute__((unused)) void h8_medium_debug_v12_class_inc(
    uint32_t class_id, atomic_size_t* class_24k, atomic_size_t* class_48k) {
  (void)class_id;
  (void)class_24k;
  (void)class_48k;
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
