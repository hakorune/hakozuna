#include "h8_internal.h"
#include "h8_medium.h"

#if defined(H8_ENABLE_DEBUG_STATS)

enum {
  H8_RETENTION_L3_M0 = 0,
  H8_RETENTION_L3_M1 = 1,
  H8_RETENTION_L3_M2 = 2,
  H8_RETENTION_L3_CLOCK = 3,
  H8_RETENTION_L3_MODEL_COUNT = 4
};

enum {
  H8_RETENTION_L3_LIVE = 0,
  H8_RETENTION_L3_PROBATION = 1,
  H8_RETENTION_L3_PROTECTED = 2,
  H8_RETENTION_L3_DECOMMITTED = 3
};

static const size_t k_h8_retention_l3_budget =
    (size_t)H8_OWNER_MAX * H8_MEDIUM_RESIDENT_BUDGET_CLASSES *
    H8_MEDIUM_RUN_BYTES;

static H8OwnerRecord* h8_retention_owner(H8MediumRun* run) {
  if (!run || !run->owner_attached) {
    return NULL;
  }
  uint64_t raw = atomic_load_explicit(&run->owner_word, memory_order_acquire);
  if (raw == 0) {
    return NULL;
  }
  H8OwnerWord word = h8_owner_word_unpack(raw);
  if (word.slot >= H8_OWNER_MAX) {
    return NULL;
  }
  H8OwnerRecord* owner = h8_owner_by_slot(word.slot);
  if (!owner || owner->slot != word.slot || owner->generation != word.generation) {
    return NULL;
  }
  return owner;
}

static void h8_retention_l3_update_peak(atomic_size_t* peak, size_t value) {
  size_t cur = atomic_load_explicit(peak, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             peak, &cur, value, memory_order_relaxed, memory_order_relaxed)) {
  }
}

static atomic_size_t* h8_retention_l3_bytes_field(uint8_t model) {
  switch (model) {
    case H8_RETENTION_L3_M1:
      return &h8g.medium_retention_l3_m1_bytes;
    case H8_RETENTION_L3_M2:
      return &h8g.medium_retention_l3_m2_bytes;
    case H8_RETENTION_L3_CLOCK:
      return &h8g.medium_retention_l3_clock_bytes;
    default:
      return &h8g.medium_retention_l3_m0_bytes;
  }
}

static atomic_size_t* h8_retention_l3_peak_field(uint8_t model) {
  switch (model) {
    case H8_RETENTION_L3_M1:
      return &h8g.medium_retention_l3_m1_peak;
    case H8_RETENTION_L3_M2:
      return &h8g.medium_retention_l3_m2_peak;
    case H8_RETENTION_L3_CLOCK:
      return &h8g.medium_retention_l3_clock_peak;
    default:
      return &h8g.medium_retention_l3_m0_peak;
  }
}

static void h8_retention_l3_inc_decommit(uint8_t model) {
  switch (model) {
    case H8_RETENTION_L3_M1:
      H8_DEBUG_INC(medium_retention_l3_m1_decommit);
      break;
    case H8_RETENTION_L3_M2:
      H8_DEBUG_INC(medium_retention_l3_m2_decommit);
      break;
    case H8_RETENTION_L3_CLOCK:
      H8_DEBUG_INC(medium_retention_l3_clock_decommit);
      break;
    default:
      H8_DEBUG_INC(medium_retention_l3_m0_decommit);
      break;
  }
}

static void h8_retention_l3_inc_refault(uint8_t model) {
  switch (model) {
    case H8_RETENTION_L3_M1:
      H8_DEBUG_INC(medium_retention_l3_m1_refault);
      break;
    case H8_RETENTION_L3_M2:
      H8_DEBUG_INC(medium_retention_l3_m2_refault);
      break;
    case H8_RETENTION_L3_CLOCK:
      H8_DEBUG_INC(medium_retention_l3_clock_refault);
      break;
    default:
      H8_DEBUG_INC(medium_retention_l3_m0_refault);
      break;
  }
}

static bool h8_retention_l3_charge(uint8_t model, H8MediumRun* run) {
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  atomic_size_t* field = h8_retention_l3_bytes_field(model);
  size_t cur = atomic_load_explicit(field, memory_order_relaxed);
  for (;;) {
    if (cur > k_h8_retention_l3_budget ||
        bytes > k_h8_retention_l3_budget - cur) {
      return false;
    }
    size_t next = cur + bytes;
    if (atomic_compare_exchange_weak_explicit(
            field, &cur, next, memory_order_acq_rel, memory_order_relaxed)) {
      h8_retention_l3_update_peak(h8_retention_l3_peak_field(model), next);
      return true;
    }
  }
}

static void h8_retention_l3_release(uint8_t model, H8MediumRun* run) {
  size_t bytes = run->run_size ? run->run_size : H8_MEDIUM_RUN_BYTES;
  atomic_fetch_sub_explicit(h8_retention_l3_bytes_field(model), bytes,
                            memory_order_acq_rel);
}

static uint8_t h8_retention_stack_find(H8OwnerRecord* owner, uint32_t class_id,
                                       H8MediumRun* run) {
  if (!owner || class_id >= H8_MEDIUM_CLASS_COUNT || !run) {
    return H8_MEDIUM_RETENTION_STACK_DEPTH + 1u;
  }
  for (uint8_t i = 0; i < H8_MEDIUM_RETENTION_STACK_DEPTH; ++i) {
    if (owner->medium_retention_stack[class_id][i] == run) {
      return (uint8_t)(i + 1u);
    }
  }
  return H8_MEDIUM_RETENTION_STACK_DEPTH + 1u;
}

static void h8_retention_stack_remove(H8OwnerRecord* owner, uint32_t class_id,
                                      H8MediumRun* run) {
  if (!owner || class_id >= H8_MEDIUM_CLASS_COUNT || !run) {
    return;
  }
  H8MediumRun** stack = owner->medium_retention_stack[class_id];
  for (uint8_t i = 0; i < H8_MEDIUM_RETENTION_STACK_DEPTH; ++i) {
    if (stack[i] != run) {
      continue;
    }
    for (uint8_t j = i; j + 1u < H8_MEDIUM_RETENTION_STACK_DEPTH; ++j) {
      stack[j] = stack[j + 1u];
    }
    stack[H8_MEDIUM_RETENTION_STACK_DEPTH - 1u] = NULL;
    return;
  }
}

static void h8_retention_stack_push(H8OwnerRecord* owner, uint32_t class_id,
                                    H8MediumRun* run) {
  if (!owner || class_id >= H8_MEDIUM_CLASS_COUNT || !run) {
    return;
  }
  h8_retention_stack_remove(owner, class_id, run);
  H8MediumRun** stack = owner->medium_retention_stack[class_id];
  for (uint8_t i = H8_MEDIUM_RETENTION_STACK_DEPTH - 1u; i > 0; --i) {
    stack[i] = stack[i - 1u];
  }
  stack[0] = run;
}

static void h8_retention_l3_protected_remove(H8OwnerRecord* owner,
                                             uint32_t class_id,
                                             H8MediumRun* run) {
  if (!owner || class_id >= H8_MEDIUM_CLASS_COUNT || !run) {
    return;
  }
  if (owner->medium_retention_l3_protected1[class_id] == run) {
    owner->medium_retention_l3_protected1[class_id] = NULL;
  }
  for (uint8_t i = 0; i < 2u; ++i) {
    if (owner->medium_retention_l3_protected2[class_id][i] == run) {
      owner->medium_retention_l3_protected2[class_id][i] = NULL;
    }
  }
}

static void h8_retention_l3_demote_victim(uint8_t model, H8MediumRun* victim) {
  if (!victim || victim->debug_retention_l3_state[model] !=
                     H8_RETENTION_L3_PROTECTED) {
    return;
  }
  victim->debug_retention_l3_state[model] = H8_RETENTION_L3_PROBATION;
}

static void h8_retention_l3_protect(H8OwnerRecord* owner, uint8_t model,
                                    H8MediumRun* run) {
  uint32_t c = run->class_id;
  if (model == H8_RETENTION_L3_M1) {
    H8MediumRun* victim = owner->medium_retention_l3_protected1[c];
    if (victim != run) {
      h8_retention_l3_demote_victim(model, victim);
    }
    owner->medium_retention_l3_protected1[c] = run;
  } else if (model == H8_RETENTION_L3_M2) {
    H8MediumRun** slots = owner->medium_retention_l3_protected2[c];
    if (slots[0] == run) {
      return;
    }
    if (slots[1] == run) {
      slots[1] = slots[0];
      slots[0] = run;
      return;
    }
    h8_retention_l3_demote_victim(model, slots[1]);
    slots[1] = slots[0];
    slots[0] = run;
  }
}

static void h8_retention_l3_empty_model(H8OwnerRecord* owner, H8MediumRun* run,
                                        uint8_t model) {
  bool wants_protected =
      (model == H8_RETENTION_L3_M1 || model == H8_RETENTION_L3_M2) &&
      run->debug_retention_l3_reuse_evidence;
  if (!h8_retention_l3_charge(model, run)) {
    run->debug_retention_l3_state[model] = H8_RETENTION_L3_DECOMMITTED;
    h8_retention_l3_inc_decommit(model);
    return;
  }
  if (wants_protected) {
    run->debug_retention_l3_state[model] = H8_RETENTION_L3_PROTECTED;
    h8_retention_l3_protect(owner, model, run);
  } else {
    run->debug_retention_l3_state[model] = H8_RETENTION_L3_PROBATION;
  }
}

static void h8_retention_l3_note_empty(H8OwnerRecord* owner, H8MediumRun* run) {
  bool m0_decommit = !h8_retention_l3_charge(H8_RETENTION_L3_M0, run);
  run->debug_retention_l3_m0_expect_decommit = m0_decommit;
  run->debug_retention_l3_state[H8_RETENTION_L3_M0] =
      m0_decommit ? H8_RETENTION_L3_DECOMMITTED : H8_RETENTION_L3_PROBATION;
  if (m0_decommit) {
    h8_retention_l3_inc_decommit(H8_RETENTION_L3_M0);
  }
  h8_retention_l3_empty_model(owner, run, H8_RETENTION_L3_M1);
  h8_retention_l3_empty_model(owner, run, H8_RETENTION_L3_M2);
  h8_retention_l3_empty_model(owner, run, H8_RETENTION_L3_CLOCK);
}

static void h8_retention_l3_force_decommit(H8MediumRun* run) {
  for (uint8_t m = 0; m < H8_RETENTION_L3_MODEL_COUNT; ++m) {
    uint8_t state = run->debug_retention_l3_state[m];
    if (state == H8_RETENTION_L3_PROBATION ||
        state == H8_RETENTION_L3_PROTECTED) {
      h8_retention_l3_release(m, run);
    }
    if (state != H8_RETENTION_L3_DECOMMITTED) {
      h8_retention_l3_inc_decommit(m);
    }
    run->debug_retention_l3_state[m] = H8_RETENTION_L3_DECOMMITTED;
  }
}

static void h8_retention_note_distance(uint8_t distance) {
  switch (distance) {
    case 1:
      H8_DEBUG_INC(medium_retention_pre_distance_1);
      break;
    case 2:
      H8_DEBUG_INC(medium_retention_pre_distance_2);
      break;
    case 3:
      H8_DEBUG_INC(medium_retention_pre_distance_3);
      break;
    case 4:
      H8_DEBUG_INC(medium_retention_pre_distance_4);
      break;
    default:
      H8_DEBUG_INC(medium_retention_pre_distance_5p);
      break;
  }
}

static void h8_retention_note_ghost_distance(uint8_t distance) {
  switch (distance) {
    case 1:
      H8_DEBUG_INC(medium_retention_ghost_distance_1);
      break;
    case 2:
      H8_DEBUG_INC(medium_retention_ghost_distance_2);
      break;
    case 3:
      H8_DEBUG_INC(medium_retention_ghost_distance_3);
      break;
    case 4:
      H8_DEBUG_INC(medium_retention_ghost_distance_4);
      break;
    default:
      H8_DEBUG_INC(medium_retention_ghost_distance_5p);
      break;
  }
}

static void h8_retention_note_ghost_epoch(uint64_t delta) {
  if (delta <= 1u) {
    H8_DEBUG_INC(medium_retention_ghost_epoch_0_1);
  } else if (delta <= 3u) {
    H8_DEBUG_INC(medium_retention_ghost_epoch_2_3);
  } else if (delta <= 7u) {
    H8_DEBUG_INC(medium_retention_ghost_epoch_4_7);
  } else {
    H8_DEBUG_INC(medium_retention_ghost_epoch_8p);
  }
}

static bool h8_retention_model_would_decommit(uint8_t distance, uint8_t depth) {
  return depth == 0u || distance > depth;
}

static void h8_retention_note_model_decommit(uint8_t distance) {
  H8_DEBUG_INC(medium_retention_model_decommit_n0);
  if (h8_retention_model_would_decommit(distance, 1u)) {
    H8_DEBUG_INC(medium_retention_model_decommit_n1);
  }
  if (h8_retention_model_would_decommit(distance, 2u)) {
    H8_DEBUG_INC(medium_retention_model_decommit_n2);
  }
  if (h8_retention_model_would_decommit(distance, 3u)) {
    H8_DEBUG_INC(medium_retention_model_decommit_n3);
  }
  if (h8_retention_model_would_decommit(distance, 4u)) {
    H8_DEBUG_INC(medium_retention_model_decommit_n4);
  }
}

static void h8_retention_note_model_refault(uint8_t distance) {
  H8_DEBUG_INC(medium_retention_model_refault_n0);
  if (h8_retention_model_would_decommit(distance, 1u)) {
    H8_DEBUG_INC(medium_retention_model_refault_n1);
  }
  if (h8_retention_model_would_decommit(distance, 2u)) {
    H8_DEBUG_INC(medium_retention_model_refault_n2);
  }
  if (h8_retention_model_would_decommit(distance, 3u)) {
    H8_DEBUG_INC(medium_retention_model_refault_n3);
  }
  if (h8_retention_model_would_decommit(distance, 4u)) {
    H8_DEBUG_INC(medium_retention_model_refault_n4);
  }
}

void h8_medium_retention_shadow_owner_init(H8OwnerRecord* owner) {
  if (!owner) {
    return;
  }
  for (uint32_t c = 0; c < H8_MEDIUM_CLASS_COUNT; ++c) {
    owner->medium_retention_epoch[c] = 0;
    for (uint8_t i = 0; i < H8_MEDIUM_RETENTION_STACK_DEPTH; ++i) {
      owner->medium_retention_stack[c][i] = NULL;
    }
    owner->medium_retention_l3_protected1[c] = NULL;
    owner->medium_retention_l3_protected2[c][0] = NULL;
    owner->medium_retention_l3_protected2[c][1] = NULL;
  }
}

void h8_medium_retention_shadow_note_empty(H8MediumRun* run) {
  H8OwnerRecord* owner = h8_retention_owner(run);
  if (!owner || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  uint32_t c = run->class_id;
  uint8_t distance = h8_retention_stack_find(owner, c, run);
  h8_retention_note_distance(distance);
  H8_DEBUG_INC(medium_retention_empty_seen);
  owner->medium_retention_epoch[c]++;
  run->debug_retention_empty_epoch = owner->medium_retention_epoch[c];
  run->debug_retention_empty_distance = distance;
  run->debug_retention_decommitted_ghost = false;
  h8_retention_stack_push(owner, c, run);
  h8_retention_l3_note_empty(owner, run);
}

void h8_medium_retention_shadow_note_retain(H8MediumRun* run) {
  if (run && run->debug_retention_l3_m0_expect_decommit) {
    H8_DEBUG_INC(medium_retention_l3_m0_mismatch);
  }
}

void h8_medium_retention_shadow_note_decommit(H8MediumRun* run,
                                              H8MediumDecommitReason reason) {
  H8OwnerRecord* owner = h8_retention_owner(run);
  if (!run || !owner || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  uint8_t distance = run->debug_retention_empty_distance;
  if (distance == 0) {
    distance = h8_retention_stack_find(owner, run->class_id, run);
  }
  switch (reason) {
    case H8_MEDIUM_DECOMMIT_BUDGET_REJECT:
      H8_DEBUG_INC(medium_retention_decommit_budget);
      break;
    case H8_MEDIUM_DECOMMIT_OWNER_EXIT:
      H8_DEBUG_INC(medium_retention_decommit_owner_exit);
      break;
    default:
      H8_DEBUG_INC(medium_retention_decommit_cold);
      break;
  }
  if (reason == H8_MEDIUM_DECOMMIT_BUDGET_REJECT &&
      !run->debug_retention_l3_m0_expect_decommit) {
    H8_DEBUG_INC(medium_retention_l3_m0_mismatch);
  }
  if (reason == H8_MEDIUM_DECOMMIT_OWNER_EXIT ||
      reason == H8_MEDIUM_DECOMMIT_COLD) {
    h8_retention_l3_force_decommit(run);
  }
  h8_retention_note_model_decommit(distance);
  run->debug_retention_decommit_epoch = owner->medium_retention_epoch[run->class_id];
  run->debug_retention_decommit_distance = distance;
  run->debug_retention_decommit_reason = (uint8_t)reason;
  run->debug_retention_decommitted_ghost = true;
}

void h8_medium_retention_shadow_note_alloc(H8MediumRun* run) {
  H8OwnerRecord* owner = h8_retention_owner(run);
  if (!run || !owner || run->class_id >= H8_MEDIUM_CLASS_COUNT) {
    return;
  }
  h8_retention_l3_protected_remove(owner, run->class_id, run);
  for (uint8_t m = 0; m < H8_RETENTION_L3_MODEL_COUNT; ++m) {
    uint8_t state = run->debug_retention_l3_state[m];
    if (state == H8_RETENTION_L3_DECOMMITTED) {
      h8_retention_l3_inc_refault(m);
      run->debug_retention_l3_reuse_evidence = true;
    } else if (state == H8_RETENTION_L3_PROBATION ||
               state == H8_RETENTION_L3_PROTECTED) {
      h8_retention_l3_release(m, run);
      run->debug_retention_l3_reuse_evidence = true;
    }
    run->debug_retention_l3_state[m] = H8_RETENTION_L3_LIVE;
  }
  if (!run->debug_retention_decommitted_ghost) {
    return;
  }
  switch ((H8MediumDecommitReason)run->debug_retention_decommit_reason) {
    case H8_MEDIUM_DECOMMIT_BUDGET_REJECT:
      H8_DEBUG_INC(medium_retention_ghost_reuse_budget);
      break;
    case H8_MEDIUM_DECOMMIT_OWNER_EXIT:
      H8_DEBUG_INC(medium_retention_ghost_reuse_owner_exit);
      break;
    default:
      H8_DEBUG_INC(medium_retention_ghost_reuse_cold);
      break;
  }
  uint64_t now = owner->medium_retention_epoch[run->class_id];
  uint64_t delta = now >= run->debug_retention_decommit_epoch
                       ? now - run->debug_retention_decommit_epoch
                       : 0;
  h8_retention_note_ghost_distance(run->debug_retention_decommit_distance);
  h8_retention_note_ghost_epoch(delta);
  h8_retention_note_model_refault(run->debug_retention_decommit_distance);
  run->debug_retention_decommitted_ghost = false;
}

#endif
