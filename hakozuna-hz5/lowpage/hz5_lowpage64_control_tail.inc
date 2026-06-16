// LowPage64 cold control-plane tail helpers.

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
static size_t hz5_lowpage64_free_list_p37_overflow(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_p37_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_p37_release_nodes, freed);
  }
  return freed;
}

static void hz5_lowpage64_p37_release_overflow(int high_event) {
  void* head = g_hz5_lowpage64_overflow_head;
  size_t count = g_hz5_lowpage64_overflow_count;
  g_hz5_lowpage64_overflow_head = NULL;
  g_hz5_lowpage64_overflow_count = 0;
  g_hz5_lowpage64_high_event_streak = 0;
  if (!head) {
    return;
  }

  size_t freed = hz5_lowpage64_free_list_p37_overflow(head);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_release_calls, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_overflow_release_nodes,
                          freed);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_calls, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_freed_nodes, freed);
  if (high_event) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_high_event_releases, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p37_lifetime_releases, 1);
  }
  (void)freed;
  (void)count;
}

static void hz5_lowpage64_p37_checkpoint(void) {
  g_hz5_lowpage64_epoch++;
  if (g_hz5_lowpage64_overflow_count == 0) {
    g_hz5_lowpage64_high_event_streak = 0;
    return;
  }

#if HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME > 0
  if (g_hz5_lowpage64_epoch >
      g_hz5_lowpage64_overflow_epoch +
          (uint32_t)HZ5_LOWPAGE64_P37_OVERFLOW_LIFETIME) {
    hz5_lowpage64_p37_release_overflow(0);
    return;
  }
#endif

#if HZ5_LOWPAGE64_P37_HIGH_STREAK > 0
  if (g_hz5_lowpage64_stash_count >= HZ5_LOWPAGE64_P37_HOT_CAP &&
      g_hz5_lowpage64_overflow_count >= HZ5_LOWPAGE64_P37_HIGH_WATER) {
    if (g_hz5_lowpage64_high_event_streak < UINT8_MAX) {
      g_hz5_lowpage64_high_event_streak++;
    }
    if (g_hz5_lowpage64_high_event_streak >=
        HZ5_LOWPAGE64_P37_HIGH_STREAK) {
      hz5_lowpage64_p37_release_overflow(1);
      return;
    }
  } else {
    g_hz5_lowpage64_high_event_streak = 0;
  }
#endif
}
#else
#define hz5_lowpage64_p37_checkpoint() ((void)0)
#endif

#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
static size_t hz5_lowpage64_free_list_p39_probation(void* head,
                                                    int lifetime_release) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_release_nodes,
                            freed);
    if (lifetime_release) {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p39_probation_lifetime_releases, 1);
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p39_probation_replace_releases, 1);
    }
  }
  return freed;
}

static void hz5_lowpage64_p39_release_probation(int lifetime_release) {
  void* head = g_hz5_lowpage64_probation_head;
  g_hz5_lowpage64_probation_head = NULL;
  g_hz5_lowpage64_probation_count = 0;
  hz5_lowpage64_count_trim(
      hz5_lowpage64_free_list_p39_probation(head, lifetime_release));
}

static void hz5_lowpage64_p39_checkpoint(void) {
  g_hz5_lowpage64_p39_epoch++;
  if (!g_hz5_lowpage64_probation_head) {
    return;
  }

#if HZ5_LOWPAGE64_P39_PROBATION_LIFETIME > 0
  if (g_hz5_lowpage64_p39_epoch >
      g_hz5_lowpage64_probation_epoch +
          (uint32_t)HZ5_LOWPAGE64_P39_PROBATION_LIFETIME) {
    hz5_lowpage64_p39_release_probation(1);
  }
#endif
}

#if HZ5_LOWPAGE64_P39_USE_ON_MISS
static void* hz5_lowpage64_p39_pop_probation(void) {
  if (!g_hz5_lowpage64_probation_head) {
    return NULL;
  }
  void* raw = g_hz5_lowpage64_probation_head;
  g_hz5_lowpage64_probation_head = hz5_lowpage64_link_next_load(raw);
  g_hz5_lowpage64_probation_count--;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_hits, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p39_probation_hits, 1);
  return raw;
}
#endif
#else
#define hz5_lowpage64_p39_checkpoint() ((void)0)
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP == 0 && \
    HZ5_LOWPAGE64_P39_PROBATION_CAP == 0
static size_t hz5_lowpage64_stash_cap_for_batch(size_t acquired_count) {
#if HZ5_LOWPAGE64_STASH_CAP > 0
  (void)acquired_count;
  return HZ5_LOWPAGE64_STASH_CAP;
#elif HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP > 0
  if (acquired_count > HZ5_LOWPAGE64_ADAPTIVE_STASH_TRIGGER) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_adaptive_trigger_calls, 1);
    return HZ5_LOWPAGE64_ADAPTIVE_STASH_CAP;
  }
  return 0;
#else
  (void)acquired_count;
  return 0;
#endif
}
#endif

#if HZ5_LOWPAGE64_STATS
static void hz5_lowpage64_count_acquired_bucket(size_t count) {
  if (count <= 64u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le64, 1);
  } else if (count <= 128u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le128, 1);
  } else if (count <= 192u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_le192, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_acquired_gt192, 1);
  }
}

static void hz5_lowpage64_count_stash_bucket(size_t count) {
  if (count <= 64u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le64, 1);
  } else if (count <= 128u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le128, 1);
  } else if (count <= 192u) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_le192, 1);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_nodes_gt192, 1);
  }
}
#else
#define hz5_lowpage64_count_acquired_bucket(count) ((void)0)
#define hz5_lowpage64_count_stash_bucket(count) ((void)0)
#endif
