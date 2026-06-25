#include "h8_internal.h"
#include "h8_medium.h"

#if defined(H8_ENABLE_DEBUG_STATS)

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
