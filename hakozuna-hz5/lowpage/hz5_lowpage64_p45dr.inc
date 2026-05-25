#if HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN
static void hz5_lowpage64_p45dr_scan_stage1(size_t* bucket0,
                                            size_t* bucket1,
                                            size_t* bucket2_4,
                                            size_t* bucket_old,
                                            size_t* oldest_age) {
  *bucket0 = 0;
  *bucket1 = 0;
  *bucket2_4 = 0;
  *bucket_old = 0;
  *oldest_age = 0;
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  uint32_t current_epoch = atomic_load_explicit(
      &g_hz5_lowpage64_p40_global_epoch, memory_order_relaxed);
  void* node = (void*)atomic_load_explicit(
      &g_hz5_lowpage64_p43p_stage1_head, memory_order_acquire);
  while (node) {
    uint32_t node_epoch = hz5_lowpage64_node_epoch_load(node);
    size_t age = current_epoch >= node_epoch
                     ? (size_t)(current_epoch - node_epoch)
                     : 0;
    if (age == 0) {
      (*bucket0)++;
    } else if (age == 1) {
      (*bucket1)++;
    } else if (age <= 4u) {
      (*bucket2_4)++;
    } else {
      (*bucket_old)++;
    }
    if (age > *oldest_age) {
      *oldest_age = age;
    }
    node = hz5_lowpage64_link_next_load(node);
  }
#endif
}

static void hz5_lowpage64_p45dr_note(Hz5Lowpage64P45Reason reason) {
  size_t current = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_stage1_count, memory_order_acquire);
  size_t hot_bridge = hz5_lowpage64_p45_bridge_hot_current(
      atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                           memory_order_acquire));
  size_t total_bridge = hot_bridge + current;
  size_t state = atomic_load_explicit(&g_hz5_lowpage64_p45_runtime_state,
                                      memory_order_relaxed);

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_checkpoint_calls, 1);
  switch (reason) {
    case HZ5_LOWPAGE64_P45_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_reason_p40, 1);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint, current);
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45dr_stage1_current_at_checkpoint_max, current);
      break;
    case HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_reason_acquire_miss, 1);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss, current);
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45dr_stage1_current_at_acquire_miss_max, current);
      break;
    case HZ5_LOWPAGE64_P45_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_reason_snapshot, 1);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45dr_stage1_current_at_snapshot, current);
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45dr_stage1_current_at_snapshot_max, current);
      break;
    case HZ5_LOWPAGE64_P45_REASON_PUBLISH:
      break;
  }

  size_t b0 = 0;
  size_t b1 = 0;
  size_t b2_4 = 0;
  size_t old = 0;
  size_t oldest = 0;
  hz5_lowpage64_p45dr_scan_stage1(&b0, &b1, &b2_4, &old, &oldest);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_bucket0, b0);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_bucket1, b1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_bucket2_4,
                          b2_4);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_stage1_age_old, old);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_stage1_old_current_max,
                          old);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_stage1_oldest_age_max,
                          oldest);

  size_t drain_to_bridge = 0;
  size_t keep_cold = current;
  size_t demote_open = 0;
  size_t block_state = 0;
  size_t block_bridge = 0;
  if (state != HZ5_LOWPAGE64_ADMISSION_OPEN) {
    block_state = current;
  } else if (hot_bridge < HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    size_t bridge_capacity = HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP - hot_bridge;
    drain_to_bridge = current < bridge_capacity ? current : bridge_capacity;
    size_t remaining = current - drain_to_bridge;
    demote_open = old < remaining ? old : remaining;
    keep_cold = remaining - demote_open;
  } else {
    block_bridge = current;
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_would_drain_to_bridge,
                          drain_to_bridge);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_would_keep_cold,
                          keep_cold);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_would_demote_open,
                          demote_open);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p45dr_would_block_drain_closed, block_state);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p45dr_would_block_bridge_excess, block_bridge);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_bridge_current_hot_max,
                          hot_bridge);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_bridge_cold_current_max,
                          current);
  HZ5_LOWPAGE64_COUNT_MAX(
      g_hz5_lowpage64_p45dr_bridge_total_with_cold_max, total_bridge);

  size_t enqueue_nodes = atomic_load_explicit(
      &g_hz5_lowpage64_p45rg_stage1_enqueue_nodes, memory_order_relaxed);
  size_t acquire_nodes = atomic_load_explicit(
      &g_hz5_lowpage64_p45rg_stage1_acquire_nodes, memory_order_relaxed);
  size_t expected = enqueue_nodes > acquire_nodes
                        ? enqueue_nodes - acquire_nodes
                        : 0;
  size_t mismatch = expected > current ? expected - current : current - expected;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_balance_expected,
                          expected);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_balance_actual,
                          current);
  if (mismatch > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45dr_balance_mismatch, 1);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45dr_balance_mismatch_max,
                            mismatch);
  }
}
#else
#define hz5_lowpage64_p45dr_note(reason) ((void)0)
#endif
