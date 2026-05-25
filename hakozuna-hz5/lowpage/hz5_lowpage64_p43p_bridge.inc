#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN
typedef enum Hz5Lowpage64P43pReason {
  HZ5_LOWPAGE64_P43P_REASON_P40 = 1,
  HZ5_LOWPAGE64_P43P_REASON_SNAPSHOT = 2,
} Hz5Lowpage64P43pReason;

#if HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN
static void hz5_lowpage64_p43p_age_note(size_t admission_state,
                                        size_t demote_intent) {
  size_t b0 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket0, 0, memory_order_relaxed);
  size_t b1 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket1, 0, memory_order_relaxed);
  size_t b2 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket2, 0, memory_order_relaxed);
  size_t b3 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket3, 0, memory_order_relaxed);
  size_t b4 = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_age_bucket4, 0, memory_order_relaxed);
  size_t old = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_age_bucket_old, memory_order_relaxed);

  old += b4;
  b4 = b3;
  b3 = b2;
  b2 = b1;
  b1 = b0;
  b0 = demote_intent;

  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_stage1_items_total,
      demote_intent);

  size_t demote_old = 0;
  if (admission_state == HZ5_LOWPAGE64_ADMISSION_OPEN) {
    demote_old = old < HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
                     ? old
                     : HZ5_LOWPAGE64_P43P_DEMOTE_BATCH;
    old -= demote_old;
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43p_age_would_demote_old_open,
        demote_old);
  } else if (admission_state == HZ5_LOWPAGE64_ADMISSION_DRAIN) {
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43p_age_would_block_old_drain,
        old);
  } else if (admission_state == HZ5_LOWPAGE64_ADMISSION_CLOSED) {
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43p_age_would_block_old_closed,
        old);
  }

  size_t young = b0 + b1;
  size_t warm = b2 + b3 + b4;
  size_t hot_pollution = warm + old;
  size_t young_reuse = young < HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
                           ? young
                           : HZ5_LOWPAGE64_P43P_DEMOTE_BATCH;

  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_would_reuse_young,
      young_reuse);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_would_keep_old,
      old);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_hot_reinject_avoided,
      hot_pollution);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_age_pop_all_pollution_projection,
      hot_pollution);

  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket0, b0,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket1, b1,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket2, b2,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket3, b3,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket4, b4,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_bucket_old, old,
                        memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_young_current,
                        young, memory_order_relaxed);
  atomic_store_explicit(&g_hz5_lowpage64_p43p_age_old_current,
                        old, memory_order_relaxed);
}
#else
#define hz5_lowpage64_p43p_age_note(admission_state, demote_intent) \
  ((void)0)
#endif

static void hz5_lowpage64_p43p_note(
    Hz5Lowpage64P43pReason reason,
    size_t observed_global,
    const Hz5Lowpage64P43StatsSnapshot* maybe_stats) {
  Hz5Lowpage64P43StatsSnapshot local_stats = {0};
  const Hz5Lowpage64P43StatsSnapshot* stats = maybe_stats;
  if (!stats) {
    hz5_lowpage64_p43_stats_snapshot(&local_stats);
    stats = &local_stats;
  }

  size_t free_current = stats->slots_committed_free_current;
  atomic_store_explicit(&g_hz5_lowpage64_p43p_p43_committed_free_current,
                        free_current, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43p_p43_committed_free_max,
                          free_current);

  Hz5Lowpage64AdmissionStep step = hz5_lowpage64_admission_advance(
      &g_hz5_lowpage64_p43o_runtime_state,
      &g_hz5_lowpage64_p43o_runtime_cooldown, free_current);
  if (step.flipped) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_admission_flips, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_age_admission_flips, 1);
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_epoch, 1);

  switch (step.new_state) {
    case HZ5_LOWPAGE64_ADMISSION_OPEN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_state_open, 1);
      break;
    case HZ5_LOWPAGE64_ADMISSION_DRAIN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_state_drain, 1);
      break;
    case HZ5_LOWPAGE64_ADMISSION_CLOSED:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_state_closed, 1);
      break;
  }

  if (reason == HZ5_LOWPAGE64_P43P_REASON_SNAPSHOT) {
    hz5_lowpage64_p43p_age_note(step.new_state, 0);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_reason_snapshot, 1);
    return;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_reason_p40, 1);
  size_t demote_intent =
      hz5_lowpage64_p40_demote_intent_count(observed_global);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_p40_release_candidates,
                          demote_intent);
  hz5_lowpage64_p43p_age_note(step.new_state, demote_intent);
  if (demote_intent == 0) {
    return;
  }

  size_t old_bridge_cold = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
      memory_order_relaxed);
  size_t bridge_cold = old_bridge_cold + demote_intent;
  size_t demote = 0;
  if (step.new_state == HZ5_LOWPAGE64_ADMISSION_OPEN) {
    demote = bridge_cold < HZ5_LOWPAGE64_P43P_DEMOTE_BATCH
                 ? bridge_cold
                 : HZ5_LOWPAGE64_P43P_DEMOTE_BATCH;
    bridge_cold -= demote;
  }

  size_t demote_from_new =
      demote > old_bridge_cold ? demote - old_bridge_cold : 0;
  size_t keep_from_new =
      demote_intent > demote_from_new
          ? demote_intent - demote_from_new
          : 0;

  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p43p_would_stage1_bridge_cold,
      demote_intent);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_would_demote_to_source,
                          demote);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_would_keep_bridge,
                          keep_from_new);
  atomic_store_explicit(
      &g_hz5_lowpage64_p43p_bridge_cold_projected_current,
      bridge_cold, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(
      g_hz5_lowpage64_p43p_bridge_cold_projected_max,
      bridge_cold);
}
#else
#define hz5_lowpage64_p43p_note(reason, observed_global, stats) ((void)0)
#endif

#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
typedef enum Hz5Lowpage64P44Reason {
  HZ5_LOWPAGE64_P44_REASON_PUBLISH = 1,
  HZ5_LOWPAGE64_P44_REASON_ACQUIRE_MISS = 2,
  HZ5_LOWPAGE64_P44_REASON_P40 = 3,
  HZ5_LOWPAGE64_P44_REASON_SNAPSHOT = 4,
} Hz5Lowpage64P44Reason;

static void hz5_lowpage64_p44_count_pair(_Atomic size_t* observed_total,
                                         _Atomic size_t* observed_max,
                                         _Atomic size_t* candidate_total,
                                         _Atomic size_t* candidate_max,
                                         size_t observed,
                                         size_t candidate) {
  HZ5_LOWPAGE64_COUNT_ADD(*observed_total, observed);
  HZ5_LOWPAGE64_COUNT_MAX(*observed_max, observed);
  HZ5_LOWPAGE64_COUNT_ADD(*candidate_total, candidate);
  HZ5_LOWPAGE64_COUNT_MAX(*candidate_max, candidate);
}

static void hz5_lowpage64_p44_note(Hz5Lowpage64P44Reason reason) {
  Hz5Lowpage64P43StatsSnapshot p43_stats = {0};
  hz5_lowpage64_p43_stats_snapshot(&p43_stats);

  size_t global = atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                                       memory_order_acquire);
  size_t stash = g_hz5_lowpage64_stash_count;
  size_t relbuf = g_hz5_lowpage64_relbuf_count;
  size_t p43_free = p43_stats.slots_committed_free_current;

  size_t global_candidate = hz5_lowpage64_p44_candidate_value(
      global, HZ5_LOWPAGE64_P44_GLOBAL_SOFT_CAP);
  size_t stash_candidate = hz5_lowpage64_p44_candidate_value(
      stash, HZ5_LOWPAGE64_P44_STASH_SOFT_CAP);
  size_t relbuf_candidate = hz5_lowpage64_p44_candidate_value(
      relbuf, HZ5_LOWPAGE64_P44_RELBUF_SOFT_CAP);
  size_t p43_candidate = hz5_lowpage64_p44_candidate_value(
      p43_free, HZ5_LOWPAGE64_P44_P43_COMMITTED_FREE_SOFT_CAP);
  size_t total_candidate = global_candidate + stash_candidate +
                           relbuf_candidate + p43_candidate;

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_checkpoint_calls, 1);
  switch (reason) {
    case HZ5_LOWPAGE64_P44_REASON_PUBLISH:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_publish, 1);
      break;
    case HZ5_LOWPAGE64_P44_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_acquire_miss, 1);
      break;
    case HZ5_LOWPAGE64_P44_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_p40, 1);
      break;
    case HZ5_LOWPAGE64_P44_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_reason_snapshot, 1);
      break;
  }

  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_global_observed_total,
      &g_hz5_lowpage64_p44_global_observed_max,
      &g_hz5_lowpage64_p44_global_candidate_total,
      &g_hz5_lowpage64_p44_global_candidate_max, global, global_candidate);
  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_stash_observed_total,
      &g_hz5_lowpage64_p44_stash_observed_max,
      &g_hz5_lowpage64_p44_stash_candidate_total,
      &g_hz5_lowpage64_p44_stash_candidate_max, stash, stash_candidate);
  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_relbuf_observed_total,
      &g_hz5_lowpage64_p44_relbuf_observed_max,
      &g_hz5_lowpage64_p44_relbuf_candidate_total,
      &g_hz5_lowpage64_p44_relbuf_candidate_max, relbuf, relbuf_candidate);
  hz5_lowpage64_p44_count_pair(
      &g_hz5_lowpage64_p44_p43_free_observed_total,
      &g_hz5_lowpage64_p44_p43_free_observed_max,
      &g_hz5_lowpage64_p44_p43_free_candidate_total,
      &g_hz5_lowpage64_p44_p43_free_candidate_max, p43_free,
      p43_candidate);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p44_candidate_total,
                          total_candidate);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p44_candidate_max,
                          total_candidate);
}
#else
#define hz5_lowpage64_p44_note(reason) ((void)0)
#endif

#if HZ5_LOWPAGE64_P45_ENABLED
typedef enum Hz5Lowpage64P45Reason {
  HZ5_LOWPAGE64_P45_REASON_PUBLISH = 1,
  HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS = 2,
  HZ5_LOWPAGE64_P45_REASON_P40 = 3,
  HZ5_LOWPAGE64_P45_REASON_SNAPSHOT = 4,
} Hz5Lowpage64P45Reason;

typedef struct Hz5Lowpage64P45Decision {
  size_t demote_intent;
  size_t bridge_hot_current;
  size_t bridge_residual;
  int state_blocks;
  int bridge_blocks;
  int bridge_excess_blocks;
  size_t state;
} Hz5Lowpage64P45Decision;

static size_t hz5_lowpage64_p45_bridge_hot_current(size_t observed_global) {
  /*
   * Hot bridge pressure excludes BRIDGE_COLD/stage1 backlog on purpose. The
   * refined gate decides whether the current P40 demotion intent should stay
   * in the hot bridge path or become bridge-cold; letting existing bridge-cold
   * count against itself would create a self-amplifying stage1 gate.
   */
  return observed_global + g_hz5_lowpage64_stash_count +
         g_hz5_lowpage64_relbuf_count;
}

static Hz5Lowpage64P45Decision hz5_lowpage64_p45_decide(
    size_t observed_global,
    size_t state) {
  Hz5Lowpage64P45Decision decision;
  decision.demote_intent =
      hz5_lowpage64_p40_demote_intent_count(observed_global);
  decision.bridge_hot_current =
      hz5_lowpage64_p45_bridge_hot_current(observed_global);
  decision.bridge_residual =
      decision.bridge_hot_current > decision.demote_intent
          ? decision.bridge_hot_current - decision.demote_intent
          : 0;
  decision.state_blocks = state != HZ5_LOWPAGE64_ADMISSION_OPEN;
  decision.bridge_blocks = decision.bridge_hot_current > 0;
  decision.bridge_excess_blocks =
      decision.bridge_residual > HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP;
  decision.state = state;
  return decision;
}

static int hz5_lowpage64_p45_decision_stage1(
    const Hz5Lowpage64P45Decision* decision) {
  return decision->state_blocks || decision->bridge_excess_blocks;
}

static void hz5_lowpage64_p45_note(
    Hz5Lowpage64P45Reason reason,
    size_t observed_global,
    const Hz5Lowpage64P43StatsSnapshot* maybe_stats) {
  Hz5Lowpage64P43StatsSnapshot local_stats = {0};
  const Hz5Lowpage64P43StatsSnapshot* stats = maybe_stats;
  if (!stats) {
    hz5_lowpage64_p43_stats_snapshot(&local_stats);
    stats = &local_stats;
  }

  size_t source_free = stats->slots_committed_free_current;
  size_t source_committed = stats->slots_committed_current;
  Hz5Lowpage64AdmissionStep step = hz5_lowpage64_admission_advance(
      &g_hz5_lowpage64_p45_runtime_state,
      &g_hz5_lowpage64_p45_runtime_cooldown, source_free);
  if (step.flipped) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_admission_flips, 1);
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_epoch, 1);
  switch (step.new_state) {
    case HZ5_LOWPAGE64_ADMISSION_OPEN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_state_open, 1);
      break;
    case HZ5_LOWPAGE64_ADMISSION_DRAIN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_state_drain, 1);
      break;
    case HZ5_LOWPAGE64_ADMISSION_CLOSED:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_state_closed, 1);
      break;
  }

  switch (reason) {
    case HZ5_LOWPAGE64_P45_REASON_PUBLISH:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_publish, 1);
      break;
    case HZ5_LOWPAGE64_P45_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_acquire_miss, 1);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_source_miss, 1);
      break;
    case HZ5_LOWPAGE64_P45_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_p40, 1);
      break;
    case HZ5_LOWPAGE64_P45_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_reason_snapshot, 1);
      break;
  }

  Hz5Lowpage64P45Decision decision =
      hz5_lowpage64_p45_decide(observed_global, step.new_state);

  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45_bridge_current_max,
                          decision.bridge_hot_current);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_bridge_pressure_total,
                          decision.bridge_hot_current);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45_source_free_current_max,
                          source_free);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_source_free_pressure_total,
                          source_free);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45_source_committed_current_max,
                          source_committed);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p45_source_committed_pressure_total,
      source_committed);

  if (reason == HZ5_LOWPAGE64_P45_REASON_P40) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_p40_demote_intent,
                            decision.demote_intent);
    if (decision.demote_intent > 0) {
      HZ5_LOWPAGE64_COUNT_MAX(
          g_hz5_lowpage64_p45_bridge_residual_after_demote_max,
          decision.bridge_residual);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_bridge_residual_after_demote_total,
          decision.bridge_residual);
    }
    if (decision.demote_intent > 0 && !decision.state_blocks &&
        !decision.bridge_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_demote_clean_open,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 && !decision.state_blocks &&
        !decision.bridge_excess_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_demote_open_bridge_soft,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 &&
        (decision.state_blocks || decision.bridge_blocks)) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_any,
                              decision.demote_intent);
    }
    if (decision.demote_intent > 0 &&
        hz5_lowpage64_p45_decision_stage1(&decision)) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_refined_any,
                              decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.state_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_p40_demote_while_drain_closed,
          decision.demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_state,
                              decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.bridge_excess_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_stage1_bridge_excess,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.bridge_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_p40_demote_while_bridge_nonempty,
          decision.demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45_would_stage1_bridge_nonempty,
          decision.demote_intent);
    }
    if (decision.demote_intent > 0 && decision.state_blocks &&
        decision.bridge_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45_would_stage1_overlap,
                              decision.demote_intent);
    }
  }
}
#else
#define hz5_lowpage64_p45_note(reason, observed_global, stats) ((void)0)
#endif
