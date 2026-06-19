#include "hz6_allocator.h"

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_origin_transfer_audit_note_counts(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t* same_class_hit,
    size_t* any_hit,
    size_t* same_class_total,
    size_t* same_class_max,
    size_t* transfer_total,
    size_t* transfer_max) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }
  size_t transfer_count = hz6_allocator_transfer_count(allocator);
  size_t same_class_count =
      hz6_allocator_transfer_count_class(allocator, class_id);
  if (transfer_count > 0) {
    ++*any_hit;
    *transfer_total += transfer_count;
    if (transfer_count > *transfer_max) {
      *transfer_max = transfer_count;
    }
  }
  if (same_class_count > 0) {
    ++*same_class_hit;
    *same_class_total += same_class_count;
    if (same_class_count > *same_class_max) {
      *same_class_max = same_class_count;
    }
  }
}
#endif

void hz6_allocator_origin_transfer_audit_note_source_commit(
    Hz6Allocator* allocator,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES
  hz6_origin_transfer_audit_note_counts(
      allocator, class_id,
      &allocator->stats.origin_transfer_source_commit_with_same_class_transfer,
      &allocator->stats.origin_transfer_source_commit_with_any_transfer,
      &allocator->stats.origin_transfer_source_commit_same_class_count_total,
      &allocator->stats.origin_transfer_source_commit_same_class_count_max,
      &allocator->stats.origin_transfer_source_commit_transfer_count_total,
      &allocator->stats.origin_transfer_source_commit_transfer_count_max);
#else
  (void)allocator;
  (void)class_id;
#endif
}

void hz6_allocator_origin_transfer_audit_note_prefill_commit(
    Hz6Allocator* allocator,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES
  hz6_origin_transfer_audit_note_counts(
      allocator, class_id,
      &allocator->stats.origin_transfer_prefill_commit_with_same_class_transfer,
      &allocator->stats.origin_transfer_prefill_commit_with_any_transfer,
      &allocator->stats.origin_transfer_prefill_commit_same_class_count_total,
      &allocator->stats.origin_transfer_prefill_commit_same_class_count_max,
      &allocator->stats.origin_transfer_prefill_commit_transfer_count_total,
      &allocator->stats.origin_transfer_prefill_commit_transfer_count_max);
#else
  (void)allocator;
  (void)class_id;
#endif
}

void hz6_allocator_origin_transfer_audit_note_pop_attempt(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.origin_transfer_pop_loop_attempt;
  }
#else
  (void)allocator;
#endif
}

void hz6_allocator_origin_transfer_audit_note_pop_empty(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.origin_transfer_pop_loop_empty;
    size_t transfer_count = hz6_allocator_transfer_count(allocator);
    if (transfer_count > 0) {
      ++allocator->stats.origin_transfer_pop_empty_with_any_transfer;
      allocator->stats.origin_transfer_pop_empty_transfer_count_total +=
          transfer_count;
      if (transfer_count >
          allocator->stats.origin_transfer_pop_empty_transfer_count_max) {
        allocator->stats.origin_transfer_pop_empty_transfer_count_max =
            transfer_count;
      }
    }
  }
#else
  (void)allocator;
#endif
}

void hz6_allocator_origin_transfer_audit_note_origin_full(
    Hz6Allocator* allocator,
    const Hz6Allocator* origin,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !origin || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }
  size_t transfer_count = hz6_allocator_transfer_count(origin);
  size_t same_class_count =
      hz6_allocator_transfer_count_class(origin, class_id);
  if (transfer_count >
      allocator->stats.origin_transfer_full_transfer_count_max) {
    allocator->stats.origin_transfer_full_transfer_count_max =
        transfer_count;
  }
  if (same_class_count == 0) {
    ++allocator->stats.origin_transfer_full_same_class_zero;
    return;
  }
  size_t capacity = hz6_allocator_transfer_capacity(origin);
  if (capacity == 0) {
    return;
  }
  if (same_class_count * 4 < capacity) {
    ++allocator->stats.origin_transfer_full_same_class_lt_quarter;
  } else if (same_class_count * 2 < capacity) {
    ++allocator->stats.origin_transfer_full_same_class_lt_half;
  } else if (same_class_count * 4 < capacity * 3) {
    ++allocator->stats.origin_transfer_full_same_class_lt_3quarter;
  } else {
    ++allocator->stats.origin_transfer_full_same_class_ge_3quarter;
  }
#else
  (void)allocator;
  (void)origin;
  (void)class_id;
#endif
}

void hz6_allocator_origin_transfer_audit_note_pop_invalid(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.origin_transfer_pop_loop_invalid;
  }
#else
  (void)allocator;
#endif
}

void hz6_allocator_origin_transfer_audit_note_pop_hit(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.origin_transfer_pop_loop_hit;
  }
#else
  (void)allocator;
#endif
}
