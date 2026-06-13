// LowPage64 cold control-plane helpers (stats, lists, checkpoint, relbuf).

#if HZ5_LOWPAGE64_STATS
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
static void hz5_lowpage64_count_max(_Atomic size_t* counter, size_t value) {
  size_t cur = atomic_load_explicit(counter, memory_order_relaxed);
  while (value > cur &&
         !atomic_compare_exchange_weak_explicit(
             counter, &cur, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}
#define HZ5_LOWPAGE64_COUNT_MAX(counter, value) \
  hz5_lowpage64_count_max(&(counter), (value))
#else
#define HZ5_LOWPAGE64_COUNT_ADD(counter, value) ((void)0)
#define HZ5_LOWPAGE64_COUNT_MAX(counter, value) ((void)0)
#endif

#if HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P44 BridgeTrimDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P43o AdmissionDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P43o ProjectedDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN && !HZ5_LOWPAGE64_STATS
#error "P43p BridgeColdDryRun requires lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P45_ENABLED && !HZ5_LOWPAGE64_STATS
#error "P45 control-plane lanes require lowpage64 diagnostic stats"
#endif

#if HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN && \
    !HZ5_LOWPAGE64_P45_REFINED_GATE
#error "P45 Stage1DrainDryRun requires P45 RefinedGate"
#endif

#if HZ5_LOWPAGE64_P43P_AGE_STAGE1_DRYRUN && \
    !HZ5_LOWPAGE64_P43P_BRIDGE_COLD_DRYRUN
#error "P43p AgeStage1DryRun requires P43p BridgeColdDryRun"
#endif

static _Atomic(uintptr_t) g_hz5_lowpage64_global_batch_head;
static _Atomic size_t g_hz5_lowpage64_global_batch_count;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_min;
static _Atomic(uintptr_t) g_hz5_lowpage64_raw_max;
#if HZ5_LOWPAGE64_STAGE1_ENABLED
/*
 * Shared BRIDGE_COLD/stage1 queue for P43p/P45 evidence lanes. The storage
 * names retain the P43p prefix for compatibility with existing diagnostics,
 * but new code should treat this as HZ5 bridge-cold state.
 *
 * The canonical speed layer is still release_common() -> relbuf -> global
 * batch -> acquired stash; this queue only holds P40 source-demotion intents
 * that the current admission experiment has decided to keep bridge-side.
 */
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static _Atomic(uintptr_t) g_hz5_lowpage64_p43p_stage1_head;
#endif
static _Atomic size_t g_hz5_lowpage64_p43p_stage1_count;
#endif
#if (HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD) || \
    HZ5_LOWPAGE64_STATS
_Atomic size_t g_hz5_lowpage64_p42_cold_count;
#endif
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static _Atomic uint32_t g_hz5_lowpage64_p40_global_epoch;
#endif
#if HZ5_LOWPAGE64_STATS || HZ5_LOWPAGE64_P43O_ADMISSION_DRYRUN || \
    HZ5_LOWPAGE64_P43O_ADMISSION_GATE || \
    HZ5_LOWPAGE64_P43O_PROJECTED_DRYRUN
static _Atomic size_t g_hz5_lowpage64_p43o_runtime_state;
static _Atomic size_t g_hz5_lowpage64_p43o_runtime_cooldown;
#endif
#if HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_p45_runtime_state;
static _Atomic size_t g_hz5_lowpage64_p45_runtime_cooldown;
#endif
#if HZ5_LOWPAGE64_P42_VA_SOURCE && HZ5_LOWPAGE64_P42_DECOMMIT_COLD
typedef struct Hz5Lowpage64ColdNode {
  void* raw;
  size_t bytes;
  struct Hz5Lowpage64ColdNode* next;
} Hz5Lowpage64ColdNode;

_Atomic(uintptr_t) g_hz5_lowpage64_p42_cold_head;
#endif

#include "hz5_lowpage64_stats_storage.inc"

#if HZ5_LOWPAGE64_SPAN_CACHE
__declspec(thread) static void* g_hz5_lowpage64_span_head;
__declspec(thread) static size_t g_hz5_lowpage64_span_count;
#endif
HZ5_TLS_STATIC void* g_hz5_lowpage64_stash_head;
HZ5_TLS_STATIC size_t g_hz5_lowpage64_stash_count;
#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
__declspec(thread) static void* g_hz5_lowpage64_overflow_head;
__declspec(thread) static size_t g_hz5_lowpage64_overflow_count;
__declspec(thread) static uint32_t g_hz5_lowpage64_epoch;
__declspec(thread) static uint32_t g_hz5_lowpage64_overflow_epoch;
__declspec(thread) static uint8_t g_hz5_lowpage64_high_event_streak;
#endif
#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
__declspec(thread) static void* g_hz5_lowpage64_probation_head;
__declspec(thread) static size_t g_hz5_lowpage64_probation_count;
__declspec(thread) static uint32_t g_hz5_lowpage64_p39_epoch;
__declspec(thread) static uint32_t g_hz5_lowpage64_probation_epoch;
#endif
HZ5_TLS_STATIC void* g_hz5_lowpage64_relbuf_head;
HZ5_TLS_STATIC void* g_hz5_lowpage64_relbuf_tail;
HZ5_TLS_STATIC size_t g_hz5_lowpage64_relbuf_count;

static void hz5_lowpage64_link_next(void* raw, void* next) {
  *(void**)raw = next;
}

static void* hz5_lowpage64_link_next_load(void* raw) {
  return *(void**)raw;
}

#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_node_epoch_store(void* raw, uint32_t epoch) {
  *(uint32_t*)((char*)raw + sizeof(void*)) = epoch;
}

static uint32_t hz5_lowpage64_node_epoch_load(void* raw) {
  return *(uint32_t*)((char*)raw + sizeof(void*));
}

static void hz5_lowpage64_mark_list_epoch(void* head, uint32_t epoch) {
  while (head) {
    hz5_lowpage64_node_epoch_store(head, epoch);
    head = hz5_lowpage64_link_next_load(head);
  }
}
#endif

#if HZ5_LOWPAGE64_P37_OVERFLOW_CAP == 0 && \
    HZ5_LOWPAGE64_P39_PROBATION_CAP == 0
static size_t hz5_lowpage64_list_count(void* head) {
  size_t count = 0;
  while (head) {
    count++;
    head = hz5_lowpage64_link_next_load(head);
  }
  return count;
}
#endif

size_t hz5_lowpage64_round_raw_bytes(size_t size,
                                     size_t alignment,
                                     size_t header_size) {
  size_t raw_request = size + alignment + header_size;
  return (raw_request + HZ5_LOWPAGE64_RAW_PAGE_SIZE - 1u) &
         ~(HZ5_LOWPAGE64_RAW_PAGE_SIZE - 1u);
}

void hz5_lowpage64_note_raw_range(void* raw_ptr, size_t raw_bytes) {
  uintptr_t begin = (uintptr_t)raw_ptr;
  uintptr_t end = begin + raw_bytes;

  uintptr_t min_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_min, memory_order_relaxed);
  while ((min_cur == 0 || begin < min_cur) &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_lowpage64_raw_min, &min_cur, begin, memory_order_relaxed,
             memory_order_relaxed)) {
  }

  uintptr_t max_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_max, memory_order_relaxed);
  while (end > max_cur &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_lowpage64_raw_max, &max_cur, end, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

int hz5_lowpage64_lookup(void* ptr) {
#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
  return hz5_lowpage64_p43_lookup(ptr);
#else
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t min_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_min, memory_order_acquire);
  uintptr_t max_cur =
      atomic_load_explicit(&g_hz5_lowpage64_raw_max, memory_order_acquire);
  return (min_cur != 0 && p >= min_cur && p < max_cur)
             ? HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE
             : HZ5_LOWPAGE64_LOOKUP_MISS;
#endif
}

#include "hz5_lowpage64_p43g.inc"

#if HZ5_LOWPAGE64_STATS || HZ5_LOWPAGE64_P44_BRIDGE_TRIM_DRYRUN
static size_t hz5_lowpage64_p44_candidate_value(size_t observed,
                                                size_t soft_cap) {
  if (observed <= soft_cap) {
    return 0;
  }
  size_t candidate = observed - soft_cap;
  if (candidate > HZ5_LOWPAGE64_P44_TRIM_BATCH) {
    candidate = HZ5_LOWPAGE64_P44_TRIM_BATCH;
  }
  return candidate;
}
#endif

#include "hz5_lowpage64_p43o.inc"

#include "hz5_lowpage64_p43p_bridge.inc"

#include "hz5_lowpage64_p45dr.inc"

static size_t hz5_lowpage64_free_list(void* head) {
  size_t freed = 0;
  while (head) {
    void* next = hz5_lowpage64_link_next_load(head);
    hz5_lowpage64_raw_os_release(head);
    freed++;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_global_overflow_frees, 1);
    head = next;
  }
  return freed;
}

static size_t hz5_lowpage64_free_list_global_cap(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_cap_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_global_cap_release_nodes,
                            freed);
  }
  return freed;
}

static size_t hz5_lowpage64_free_list_stash_trim(void* head) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_stash_trim_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_stash_trim_release_nodes,
                            freed);
  }
  return freed;
}

static void hz5_lowpage64_count_trim(size_t freed) {
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_trim_freed_nodes, freed);
  }
}

#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_global_push_list(void* head,
                                           void* tail,
                                           size_t count) {
  if (!head || !tail || count == 0) {
    return;
  }

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_global_batch_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_lowpage64_global_batch_count, count,
                            memory_order_relaxed);
}

#if HZ5_LOWPAGE64_STAGE1_ENABLED && HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
static void hz5_lowpage64_p43p_stage1_push_list_impl(void* head,
                                                     void* tail,
                                                     size_t count,
                                                     int count_enqueue) {
  if (!head || !tail || count == 0) {
    return;
  }

#if HZ5_LOWPAGE64_P45_STAGE1_DRAIN_DRYRUN && \
    HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  if (count_enqueue) {
    /*
     * Age for P45dr means "time since this list became BRIDGE_COLD", not the
     * original allocation age. Requeues keep their existing epoch so the
     * diagnostic can distinguish bounded retention from repeated reinjection.
     */
    uint32_t stage1_epoch = atomic_load_explicit(
        &g_hz5_lowpage64_p40_global_epoch, memory_order_relaxed);
    hz5_lowpage64_mark_list_epoch(head, stage1_epoch);
  }
#endif

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_p43p_stage1_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_p43p_stage1_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  size_t previous = atomic_fetch_add_explicit(
      &g_hz5_lowpage64_p43p_stage1_count, count, memory_order_relaxed);
#if HZ5_LOWPAGE64_P43P_BRIDGE_COLD_STAGE1
  if (count_enqueue) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_enqueue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_enqueue_nodes, count);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_requeue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p43p_stage1_requeue_nodes, count);
  }
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p43p_stage1_current_max,
                          previous + count);
#endif
#if HZ5_LOWPAGE64_P45_REFINED_GATE
  if (count_enqueue) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_enqueue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_enqueue_nodes,
                            count);
  } else {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_requeue_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_requeue_nodes,
                            count);
  }
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45rg_stage1_current_max,
                          previous + count);
#endif
  (void)previous;
}

static void hz5_lowpage64_p43p_stage1_push_list(void* head,
                                                void* tail,
                                                size_t count) {
  hz5_lowpage64_p43p_stage1_push_list_impl(head, tail, count, 1);
}

static void hz5_lowpage64_bridge_cold_push_list(void* head,
                                                void* tail,
                                                size_t count) {
  hz5_lowpage64_p43p_stage1_push_list(head, tail, count);
}

static void* hz5_lowpage64_p43p_stage1_pop_batch(size_t limit,
                                                 size_t* out_count) {
  /*
   * Bounded BRIDGE_COLD acquire. P43p pop-all showed that wholesale reinjection
   * can pollute the hot topology, so P45 keeps this intentionally small.
   */
  void* head =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_p43p_stage1_head, 0,
                                      memory_order_acq_rel);
  size_t count = atomic_exchange_explicit(
      &g_hz5_lowpage64_p43p_stage1_count, 0, memory_order_acq_rel);
  if (!head) {
    count = 0;
  }
  if (out_count) {
    *out_count = count;
  }
  if (!head || limit == 0 || count <= limit) {
    return head;
  }

  void* batch_tail = head;
  size_t batch_count = 1;
  while (batch_count < limit && hz5_lowpage64_link_next_load(batch_tail)) {
    batch_tail = hz5_lowpage64_link_next_load(batch_tail);
    batch_count++;
  }

  void* rest_head = hz5_lowpage64_link_next_load(batch_tail);
  hz5_lowpage64_link_next(batch_tail, NULL);
  size_t rest_count = count - batch_count;
  if (rest_head) {
    void* rest_tail = rest_head;
    while (hz5_lowpage64_link_next_load(rest_tail)) {
      rest_tail = hz5_lowpage64_link_next_load(rest_tail);
    }
    hz5_lowpage64_p43p_stage1_push_list_impl(rest_head, rest_tail,
                                             rest_count, 0);
  }
  if (out_count) {
    *out_count = batch_count;
  }
  return head;
}

static void* hz5_lowpage64_bridge_cold_pop_batch(size_t limit,
                                                 size_t* out_count) {
  return hz5_lowpage64_p43p_stage1_pop_batch(limit, out_count);
}
#endif

static size_t hz5_lowpage64_free_list_p40_global(void* head,
                                                 size_t age_nodes,
                                                 size_t hard_nodes) {
  size_t freed = hz5_lowpage64_free_list(head);
  if (freed > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_nodes, freed);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_age_nodes,
                            age_nodes);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_release_hard_nodes,
                            hard_nodes);
  }
  return freed;
}

typedef struct Hz5Lowpage64P40Split {
  void* keep_head;
  void* keep_tail;
  size_t keep_count;
  void* demote_head;
  void* demote_tail;
  size_t demote_count;
  size_t age_nodes;
  size_t hard_nodes;
} Hz5Lowpage64P40Split;

static void hz5_lowpage64_append_node(void** head,
                                      void** tail,
                                      size_t* count,
                                      void* node) {
  if (!*head) {
    *head = node;
  } else {
    hz5_lowpage64_link_next(*tail, node);
  }
  *tail = node;
  (*count)++;
}

static void hz5_lowpage64_p40_split_global_list(void* head,
                                                uint32_t epoch,
                                                size_t observed,
                                                Hz5Lowpage64P40Split* out) {
  /*
   * Historical behavior split the global batch into keep vs release lists.
   * The control-plane reading is keep vs source-demotion intent.
   */
  void* node = head;
  while (node) {
    void* next = hz5_lowpage64_link_next_load(node);
    uint32_t node_epoch = hz5_lowpage64_node_epoch_load(node);
    uint32_t age = epoch - node_epoch;
    int over_hard =
#if HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP > 0
        observed > HZ5_LOWPAGE64_P40_GLOBAL_HARD_CAP;
#else
        0;
#endif
    int old_enough = age >= HZ5_LOWPAGE64_P40_TTL_EPOCHS;
    int should_demote =
        out->demote_count < HZ5_LOWPAGE64_P40_RELEASE_BATCH &&
        observed - out->demote_count > HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP &&
        (old_enough || over_hard);

    hz5_lowpage64_link_next(node, NULL);
    if (should_demote) {
      hz5_lowpage64_append_node(&out->demote_head, &out->demote_tail,
                                &out->demote_count, node);
      if (over_hard && !old_enough) {
        out->hard_nodes++;
      } else {
        out->age_nodes++;
      }
    } else {
      hz5_lowpage64_append_node(&out->keep_head, &out->keep_tail,
                                &out->keep_count, node);
    }

    node = next;
  }
}

#if HZ5_LOWPAGE64_STAGE1_ENABLED && HZ5_LOWPAGE64_P45_REFINED_GATE
static int hz5_lowpage64_p45_should_stage1_demote_intent(
    size_t observed,
    size_t demote_count) {
  /*
   * P45 refined gate is the first control-plane behavior probe:
   * - OPEN + bridge residual within soft cap: preserve the existing P40 path.
   * - DRAIN/CLOSED or bridge residual excess: stage only this P40 demotion
   *   intent as BRIDGE_COLD.
   *
   * This deliberately avoids P43p's blunt "all candidates become stage1"
   * shape and does not trim/decommit/release memory.
   */
  size_t p45_state = atomic_load_explicit(
      &g_hz5_lowpage64_p45_runtime_state, memory_order_relaxed);
  Hz5Lowpage64P45Decision p45_decision =
      hz5_lowpage64_p45_decide(observed, p45_state);
  int p45_stage1 = hz5_lowpage64_p45_decision_stage1(&p45_decision);
  if (demote_count > 0) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_demote_intent,
                            demote_count);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p45rg_bridge_residual_max,
                            p45_decision.bridge_residual);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_bridge_residual_total,
                            p45_decision.bridge_residual);
    if (p45_stage1) {
      HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_any,
                              demote_count);
      if (p45_decision.state_blocks) {
        HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_state,
                                demote_count);
      }
      if (p45_decision.bridge_excess_blocks) {
        HZ5_LOWPAGE64_COUNT_ADD(
            g_hz5_lowpage64_p45rg_stage1_bridge_excess, demote_count);
      }
      if (p45_decision.state_blocks && p45_decision.bridge_excess_blocks) {
        HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p45rg_stage1_overlap,
                                demote_count);
      }
    } else {
      HZ5_LOWPAGE64_COUNT_ADD(
          g_hz5_lowpage64_p45rg_allowed_open_bridge_soft, demote_count);
    }
  }
  return p45_stage1;
}
#endif

static void hz5_lowpage64_p40_apply_demote_intents(
    const Hz5Lowpage64P40Split* split,
    size_t observed) {
#if HZ5_LOWPAGE64_STAGE1_ENABLED
#if HZ5_LOWPAGE64_P45_REFINED_GATE
  if (hz5_lowpage64_p45_should_stage1_demote_intent(
          observed, split->demote_count)) {
    hz5_lowpage64_bridge_cold_push_list(split->demote_head,
                                        split->demote_tail,
                                        split->demote_count);
    return;
  }
#else
  hz5_lowpage64_bridge_cold_push_list(split->demote_head,
                                      split->demote_tail,
                                      split->demote_count);
  (void)observed;
  return;
#endif
#else
  (void)observed;
#endif

  hz5_lowpage64_count_trim(
      hz5_lowpage64_free_list_p40_global(split->demote_head,
                                         split->age_nodes,
                                         split->hard_nodes));
}

static void hz5_lowpage64_p40_checkpoint(void) {
  // P44 observes candidate state at slow/checkpoint boundaries only. It must
  // not turn this P40 checkpoint into an actual trim policy by accident.
  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_P40);

  size_t observed = atomic_load_explicit(&g_hz5_lowpage64_global_batch_count,
                                         memory_order_acquire);
  if (observed <= HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP) {
    return;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_checkpoint_calls, 1);
  Hz5Lowpage64P43StatsSnapshot p43_stats = {0};
  hz5_lowpage64_p43_stats_snapshot(&p43_stats);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_p43_free_before_total,
                          p43_stats.slots_committed_free_current);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p40_p43_free_before_max,
                          p43_stats.slots_committed_free_current);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_p43_committed_before_total,
                          p43_stats.slots_committed_current);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p40_p43_committed_before_max,
                          p43_stats.slots_committed_current);
  hz5_lowpage64_p43o_note(HZ5_LOWPAGE64_P43O_REASON_P40, observed,
                          &p43_stats);
  hz5_lowpage64_p43p_note(HZ5_LOWPAGE64_P43P_REASON_P40, observed,
                          &p43_stats);
  hz5_lowpage64_p45_note(HZ5_LOWPAGE64_P45_REASON_P40, observed,
                         &p43_stats);
#if HZ5_LOWPAGE64_P43O_PROJECTED_GATE
  size_t p40_demote_intent =
      hz5_lowpage64_p40_demote_intent_count(observed);
  if (!hz5_lowpage64_p43o_projected_source_admission_open(
          &p43_stats, p40_demote_intent)) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_nodes,
                            p43_stats.slots_committed_free_current);
    return;
  }
#endif
#if HZ5_LOWPAGE64_P43O_ADMISSION_GATE
  if (!hz5_lowpage64_p43o_source_admission_open(&p43_stats)) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_nodes,
                            p43_stats.slots_committed_free_current);
    return;
  }
#endif
#if HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP > 0
  if (p43_stats.slots_committed_free_current >=
      HZ5_LOWPAGE64_P40_SKIP_WHEN_P43_FREE_CAP) {
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_calls, 1);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_skip_p43_free_nodes,
                            p43_stats.slots_committed_free_current);
    return;
  }
#endif
  uint32_t epoch = atomic_fetch_add_explicit(
                       &g_hz5_lowpage64_p40_global_epoch, 1,
                       memory_order_relaxed) +
                   1u;

  void* head =
      (void*)atomic_exchange_explicit(&g_hz5_lowpage64_global_batch_head, 0,
                                      memory_order_acq_rel);
  size_t expected_count = atomic_exchange_explicit(
      &g_hz5_lowpage64_global_batch_count, 0, memory_order_acq_rel);
  (void)expected_count;
  if (!head) {
    return;
  }

  /*
   * Historical name: release list.
   *
   * Under the P43i/P45 contract these nodes are better read as P40
   * source-demotion intents. If the lane has no bridge-cold control they may
   * still go to the old raw release path, but P45 can instead keep them
   * bridge-side as BRIDGE_COLD/stage1 without decommit/runtime release.
   */
  Hz5Lowpage64P40Split split = {0};
  hz5_lowpage64_p40_split_global_list(head, epoch, observed, &split);

  if (split.keep_head) {
    hz5_lowpage64_global_push_list(split.keep_head, split.keep_tail,
                                   split.keep_count);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p40_keep_nodes,
                            split.keep_count);
  }
  hz5_lowpage64_p40_apply_demote_intents(&split, observed);
  hz5_lowpage64_p45dr_note(HZ5_LOWPAGE64_P45_REASON_P40);
}
#else
#define hz5_lowpage64_p40_checkpoint() ((void)0)
#define hz5_lowpage64_mark_list_epoch(head, epoch) ((void)0)
#endif

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

static void hz5_lowpage64_set_stash_from_list(void* head,
                                              size_t acquired_count) {
#if HZ5_LOWPAGE64_P39_PROBATION_CAP > 0
  (void)acquired_count;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);

  if (g_hz5_lowpage64_probation_head) {
    hz5_lowpage64_p39_release_probation(0);
  }

  void* hot_head = head;
  void* hot_tail = NULL;
  size_t hot_kept = 0;
  void* node = head;
  while (node && hot_kept < HZ5_LOWPAGE64_P39_HOT_CAP) {
    hot_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    hot_kept++;
  }
  if (hot_tail) {
    hz5_lowpage64_link_next(hot_tail, NULL);
  }

  void* probation_head = node;
  void* probation_tail = NULL;
  size_t probation_kept = 0;
  while (node && probation_kept < HZ5_LOWPAGE64_P39_PROBATION_CAP) {
    probation_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    probation_kept++;
  }
  if (probation_tail) {
    hz5_lowpage64_link_next(probation_tail, NULL);
  }

  g_hz5_lowpage64_stash_head = hot_head;
  g_hz5_lowpage64_stash_count = hot_kept;
  g_hz5_lowpage64_probation_head = probation_head;
  g_hz5_lowpage64_probation_count = probation_kept;
  if (probation_kept > 0) {
    g_hz5_lowpage64_probation_epoch = g_hz5_lowpage64_p39_epoch;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, hot_kept);
  hz5_lowpage64_count_stash_bucket(hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p39_probation_set_nodes_total, probation_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p39_probation_set_nodes_max,
                          probation_kept);

  hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
  return;
#elif HZ5_LOWPAGE64_P37_OVERFLOW_CAP > 0
  (void)acquired_count;
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);

  void* hot_head = head;
  void* hot_tail = NULL;
  size_t hot_kept = 0;
  void* node = head;
  while (node && hot_kept < HZ5_LOWPAGE64_P37_HOT_CAP) {
    hot_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    hot_kept++;
  }
  if (hot_tail) {
    hz5_lowpage64_link_next(hot_tail, NULL);
  }

  void* overflow_head = node;
  void* overflow_tail = NULL;
  size_t overflow_kept = 0;
  while (node && overflow_kept < HZ5_LOWPAGE64_P37_OVERFLOW_CAP) {
    overflow_tail = node;
    node = hz5_lowpage64_link_next_load(node);
    overflow_kept++;
  }
  if (overflow_tail) {
    hz5_lowpage64_link_next(overflow_tail, NULL);
  }

  g_hz5_lowpage64_stash_head = hot_head;
  g_hz5_lowpage64_stash_count = hot_kept;
  g_hz5_lowpage64_overflow_head = overflow_head;
  g_hz5_lowpage64_overflow_count = overflow_kept;
  if (overflow_kept > 0) {
    g_hz5_lowpage64_overflow_epoch = g_hz5_lowpage64_epoch;
  }

  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          hot_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, hot_kept);
  hz5_lowpage64_count_stash_bucket(hot_kept);
  HZ5_LOWPAGE64_COUNT_ADD(
      g_hz5_lowpage64_p37_overflow_set_nodes_total, overflow_kept);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p37_overflow_set_nodes_max,
                          overflow_kept);

  hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
  return;
#else
  size_t cap = hz5_lowpage64_stash_cap_for_batch(acquired_count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_calls, 1);
  if (cap > 0) {
    void* keep_head = head;
    void* keep_tail = NULL;
    size_t kept = 0;
    void* node = head;
    while (node && kept < cap) {
      keep_tail = node;
      node = hz5_lowpage64_link_next_load(node);
      kept++;
    }
    if (keep_tail) {
      hz5_lowpage64_link_next(keep_tail, NULL);
    }
    g_hz5_lowpage64_stash_head = keep_head;
    g_hz5_lowpage64_stash_count = kept;
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total, kept);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max, kept);
    HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total, kept);
    HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max, kept);
    hz5_lowpage64_count_stash_bucket(kept);
    hz5_lowpage64_count_trim(hz5_lowpage64_free_list_stash_trim(node));
    return;
  }

  g_hz5_lowpage64_stash_head = head;
  g_hz5_lowpage64_stash_count = hz5_lowpage64_list_count(head);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_stash_set_nodes_total,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_stash_set_nodes_max,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_p38_set_hot_nodes_total,
                          g_hz5_lowpage64_stash_count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_p38_set_hot_nodes_max,
                          g_hz5_lowpage64_stash_count);
  hz5_lowpage64_count_stash_bucket(g_hz5_lowpage64_stash_count);
#endif
}

static void hz5_lowpage64_publish_relbuf(void) {
  if (!g_hz5_lowpage64_relbuf_head || !g_hz5_lowpage64_relbuf_tail ||
      g_hz5_lowpage64_relbuf_count == 0) {
    return;
  }

  void* head = g_hz5_lowpage64_relbuf_head;
  void* tail = g_hz5_lowpage64_relbuf_tail;
  size_t count = g_hz5_lowpage64_relbuf_count;
  g_hz5_lowpage64_relbuf_head = NULL;
  g_hz5_lowpage64_relbuf_tail = NULL;
  g_hz5_lowpage64_relbuf_count = 0;

  size_t global_count = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_count, memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_relbuf_count_max, count);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_global_count_before_max,
                          global_count);
  if (global_count + count > HZ5_LOWPAGE64_GLOBAL_CAP) {
    (void)hz5_lowpage64_free_list_global_cap(head);
    return;
  }

  uint32_t publish_epoch = 0;
#if HZ5_LOWPAGE64_P40_GLOBAL_SOFT_CAP > 0
  publish_epoch = atomic_load_explicit(&g_hz5_lowpage64_p40_global_epoch,
                                       memory_order_relaxed);
  hz5_lowpage64_mark_list_epoch(head, publish_epoch);
#endif

  uintptr_t old_head = atomic_load_explicit(
      &g_hz5_lowpage64_global_batch_head, memory_order_acquire);
  do {
    hz5_lowpage64_link_next(tail, (void*)old_head);
  } while (!atomic_compare_exchange_weak_explicit(
      &g_hz5_lowpage64_global_batch_head, &old_head, (uintptr_t)head,
      memory_order_acq_rel, memory_order_acquire));

  atomic_fetch_add_explicit(&g_hz5_lowpage64_global_batch_count, count,
                            memory_order_relaxed);
  HZ5_LOWPAGE64_COUNT_MAX(g_hz5_lowpage64_global_count_after_max,
                          global_count + count);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publishes, 1);
  HZ5_LOWPAGE64_COUNT_ADD(g_hz5_lowpage64_batch_publish_nodes, count);
  (void)publish_epoch;
  hz5_lowpage64_p44_note(HZ5_LOWPAGE64_P44_REASON_PUBLISH);
  hz5_lowpage64_p45_note(HZ5_LOWPAGE64_P45_REASON_PUBLISH,
                         global_count + count, NULL);
  hz5_lowpage64_p40_checkpoint();
}
