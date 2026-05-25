#if HZ5_LOWPAGE64_P43O_ANY
typedef enum Hz5Lowpage64P43oReason {
  HZ5_LOWPAGE64_P43O_REASON_ACQUIRE_MISS = 1,
  HZ5_LOWPAGE64_P43O_REASON_P40 = 2,
  HZ5_LOWPAGE64_P43O_REASON_SNAPSHOT = 3,
} Hz5Lowpage64P43oReason;

typedef enum Hz5Lowpage64P43oState {
  HZ5_LOWPAGE64_P43O_STATE_OPEN = 0,
  HZ5_LOWPAGE64_P43O_STATE_DRAIN = 1,
  HZ5_LOWPAGE64_P43O_STATE_CLOSED = 2,
} Hz5Lowpage64P43oState;

typedef enum Hz5Lowpage64AdmissionState {
  HZ5_LOWPAGE64_ADMISSION_OPEN = HZ5_LOWPAGE64_P43O_STATE_OPEN,
  HZ5_LOWPAGE64_ADMISSION_DRAIN = HZ5_LOWPAGE64_P43O_STATE_DRAIN,
  HZ5_LOWPAGE64_ADMISSION_CLOSED = HZ5_LOWPAGE64_P43O_STATE_CLOSED,
} Hz5Lowpage64AdmissionState;

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
static void hz5_lowpage64_p43o_bucket_free(size_t free_current,
                                           size_t* hot,
                                           size_t* warm,
                                           size_t* cold) {
  size_t low = HZ5_LOWPAGE64_P43O_LOW_WATER;
  size_t high = HZ5_LOWPAGE64_P43O_HIGH_WATER;
  *hot = free_current < low ? free_current : low;
  if (free_current <= low) {
    *warm = 0;
  } else {
    size_t warm_limit = high > low ? high - low : 0;
    size_t warm_current = free_current - low;
    *warm = warm_current < warm_limit ? warm_current : warm_limit;
  }
  *cold = free_current > high ? free_current - high : 0;
}
#else
#define hz5_lowpage64_p43o_bucket_free(free_current, hot, warm, cold) \
  do {                                                               \
    *(hot) = 0;                                                      \
    *(warm) = 0;                                                     \
    *(cold) = 0;                                                     \
  } while (0)
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_ADMISSION_GATE || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN || \
    HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN || \
    HZ5_LOWPAGE64_P45_ENABLED
static size_t hz5_lowpage64_admission_choose_state(
    size_t old_state,
    size_t source_free_current,
    _Atomic size_t* cooldown_ptr) {
  if (source_free_current >= HZ5_LOWPAGE64_P43O_HARD_WATER) {
    atomic_store_explicit(cooldown_ptr, HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS,
                          memory_order_relaxed);
    return HZ5_LOWPAGE64_ADMISSION_CLOSED;
  }
  if (source_free_current >= HZ5_LOWPAGE64_P43O_HIGH_WATER) {
    atomic_store_explicit(cooldown_ptr, HZ5_LOWPAGE64_P43O_COOLDOWN_EPOCHS,
                          memory_order_relaxed);
    return HZ5_LOWPAGE64_ADMISSION_DRAIN;
  }
  if (source_free_current > HZ5_LOWPAGE64_P43O_LOW_WATER) {
    return old_state;
  }
  if (old_state == HZ5_LOWPAGE64_ADMISSION_OPEN) {
    return HZ5_LOWPAGE64_ADMISSION_OPEN;
  }
  size_t cooldown =
      atomic_load_explicit(cooldown_ptr, memory_order_relaxed);
  if (cooldown > 0) {
    atomic_store_explicit(cooldown_ptr, cooldown - 1,
                          memory_order_relaxed);
    return old_state;
  }
  return HZ5_LOWPAGE64_ADMISSION_OPEN;
}

typedef struct Hz5Lowpage64AdmissionStep {
  Hz5Lowpage64AdmissionState old_state;
  Hz5Lowpage64AdmissionState new_state;
  int flipped;
} Hz5Lowpage64AdmissionStep;

static Hz5Lowpage64AdmissionStep hz5_lowpage64_admission_advance(
    _Atomic size_t* state_ptr,
    _Atomic size_t* cooldown_ptr,
    size_t source_free_current) {
  Hz5Lowpage64AdmissionStep step;
  step.old_state = (Hz5Lowpage64AdmissionState)atomic_load_explicit(
      state_ptr, memory_order_relaxed);
  step.new_state =
      (Hz5Lowpage64AdmissionState)hz5_lowpage64_admission_choose_state(
          step.old_state, source_free_current, cooldown_ptr);
  step.flipped = step.new_state != step.old_state;
  atomic_store_explicit(state_ptr, step.new_state, memory_order_relaxed);
  return step;
}
#endif

static size_t hz5_lowpage64_p40_demote_intent_count(
    size_t observed_global) {
  if (observed_global <= HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    return 0;
  }
  return observed_global - HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP;
}

#if HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_GATE
static size_t hz5_lowpage64_p43o_projected_release_batch(
    size_t demote_intent) {
  return demote_intent < HZ5_LOWPAGE64_P40_RELEASE_BATCH
             ? demote_intent
             : HZ5_LOWPAGE64_P40_RELEASE_BATCH;
}

static int hz5_lowpage64_p43o_projected_blocks(size_t free_current,
                                               size_t demote_intent) {
  size_t projected_batch =
      hz5_lowpage64_p43o_projected_release_batch(demote_intent);
  if (projected_batch == 0) {
    return 0;
  }
  return free_current + projected_batch >= HZ5_LOWPAGE64_P43O_HIGH_WATER;
}
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_GATE
static int hz5_lowpage64_p43o_source_admission_open(
    const Hz5Lowpage64P43StatsSnapshot* stats) {
  Hz5Lowpage64AdmissionStep step = hz5_lowpage64_admission_advance(
      &g_hz5_lowpage64_p43o_runtime_state,
      &g_hz5_lowpage64_p43o_runtime_cooldown,
      stats ? stats->slots_committed_free_current : 0);
  return step.new_state == HZ5_LOWPAGE64_ADMISSION_OPEN;
}
#else
#define hz5_lowpage64_p43o_source_admission_open(stats) (1)
#endif

#if HZ5_LOWPAGE64_P43O_PROJECTED_GATE
static int hz5_lowpage64_p43o_projected_source_admission_open(
    const Hz5Lowpage64P43StatsSnapshot* stats,
    size_t demote_intent) {
  size_t free_current = stats ? stats->slots_committed_free_current : 0;
  return !hz5_lowpage64_p43o_projected_blocks(free_current,
                                              demote_intent);
}
#else
#define hz5_lowpage64_p43o_projected_source_admission_open(stats, candidates) \
  (1)
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
static void hz5_lowpage64_p43o_note(
    Hz5Lowpage64P43oReason reason,
    size_t observed_global,
    const Hz5Lowpage64P43StatsSnapshot* maybe_stats) {
  Hz5Lowpage64P43StatsSnapshot local_stats = {0};
  const Hz5Lowpage64P43StatsSnapshot* stats = maybe_stats;
  if (!stats) {
    hz5_lowpage64_p43_stats_snapshot(&local_stats);
    stats = &local_stats;
  }

  size_t free_current = stats->slots_committed_free_current;
  Hz5Lowpage64AdmissionStep step = hz5_lowpage64_admission_advance(
      &g_hz5_lowpage64_p43o_runtime_state,
      &g_hz5_lowpage64_p43o_runtime_cooldown, free_current);
  if (step.flipped) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_admission_flips, 1);
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_epoch, 1);
  switch (step.new_state) {
    case HZ5_LOWPAGE64_ADMISSION_OPEN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_state_open, 1);
      break;
    case HZ5_LOWPAGE64_ADMISSION_DRAIN:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_state_drain, 1);
      break;
    case HZ5_LOWPAGE64_ADMISSION_CLOSED:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_state_closed, 1);
      break;
  }
  switch (reason) {
    case HZ5_LOWPAGE64_P43O_REASON_ACQUIRE_MISS:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_reason_acquire_miss, 1);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_source_miss, 1);
      break;
    case HZ5_LOWPAGE64_P43O_REASON_P40:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_reason_p40, 1);
      break;
    case HZ5_LOWPAGE64_P43O_REASON_SNAPSHOT:
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_reason_snapshot, 1);
      break;
  }

  size_t hot = 0;
  size_t warm = 0;
  size_t cold = 0;
  hz5_lowpage64_p43o_bucket_free(free_current, &hot, &warm, &cold);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43o_p43_free_max,
                          free_current);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p43_free_hot_total, hot);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p43_free_warm_total, warm);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p43_free_cold_total, cold);

  size_t demote_intent = 0;
  if (reason == HZ5_LOWPAGE64_P43O_REASON_P40) {
    demote_intent = hz5_lowpage64_p40_demote_intent_count(observed_global);
  }
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p40_release_candidates,
                          demote_intent);
  if (demote_intent > 0) {
    if (step.new_state == HZ5_LOWPAGE64_ADMISSION_OPEN) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_p40_would_demote_to_source,
          demote_intent);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p40_would_stage1,
                              demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_p40_would_keep_bridge,
                              demote_intent);
    }
  }

#if HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
  if (demote_intent > 0) {
    size_t projected_batch =
        hz5_lowpage64_p43o_projected_release_batch(demote_intent);
    size_t projected_free = free_current + projected_batch;
    int projected_blocks = hz5_lowpage64_p43o_projected_blocks(
        free_current, demote_intent);
    HZ5_LOWPAGE64_COUNT_ADD(
        g_hz5_lowpage64_p43o_projected_p40_candidates,
        demote_intent);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43o_projected_free_max,
                            projected_free);
    if (projected_blocks) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_projected_would_stage1,
          demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_projected_would_keep_bridge,
          demote_intent);
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43o_projected_blocks, 1);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p43o_projected_would_demote_to_source,
          demote_intent);
    }
  }
#endif
}
#else
#define hz5_lowpage64_p43o_note(reason, observed_global, stats) ((void)0)
#endif
#else
#define hz5_lowpage64_p43o_bucket_free(free_current, hot, warm, cold) \
  do {                                                               \
    *(hot) = 0;                                                      \
    *(warm) = 0;                                                     \
    *(cold) = 0;                                                     \
  } while (0)
#define hz5_lowpage64_p43o_source_admission_open(stats) (1)
#define hz5_lowpage64_p43o_projected_source_admission_open(stats, candidates) \
  (1)
#define hz5_lowpage64_p43o_note(reason, observed_global, stats) ((void)0)
#endif

