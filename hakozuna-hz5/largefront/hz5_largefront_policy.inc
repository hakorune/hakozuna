#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
// Heavy diagnostic lane: these counters are touched from malloc/free paths.
// Keep this separate from Policy-L0 so speed lanes never inherit hot counters.
static _Atomic uint64_t
    g_hz5_largefront_obs_alloc_calls[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_local_pop_hit[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_remote_take_first[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_remote_to_local[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_global_pop_hit[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_new_span[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_free_local[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_free_remote[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_free_global[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_refill[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch4[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch8[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch16[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_source_batch_other[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_obs_local_free_highwater[HZ5_LARGEFRONT_CLASS_COUNT];
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
// Control-plane observation lane: only source refill, remote flush, and owner
// drain slow paths update these counters. This is the safe base for future
// policy selection; it is not a speed lane.
static _Atomic uint64_t
    g_hz5_largefront_policy_source_empty[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_refill[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch4[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch8[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch16[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_source_batch_other[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_mapped_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_remote_flush[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_remote_flush_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_remote_flush_cap_hit[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_drain[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_drain_spans[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_take_first[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_to_local[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_republish[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_hold[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_orphan[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_owner_state_fail[HZ5_LARGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L8_SHADOW
static _Atomic uint64_t
    g_hz5_largefront_policy_l8_heavy_drain[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_l8_sparse_drain[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_l8_local_like[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_l8_hold_like[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_l8_republish_like[HZ5_LARGEFRONT_CLASS_COUNT];
static _Atomic uint64_t
    g_hz5_largefront_policy_l8_mixed_like[HZ5_LARGEFRONT_CLASS_COUNT];
#endif
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE || \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
static void hz5_largefront_counter_inc(_Atomic uint64_t* counter) {
  atomic_fetch_add_explicit(counter, 1u, memory_order_relaxed);
}

static void hz5_largefront_counter_add(_Atomic uint64_t* counter,
                                       uint64_t value) {
  atomic_fetch_add_explicit(counter, value, memory_order_relaxed);
}

static void hz5_largefront_counter_note_source_batch(
    _Atomic uint64_t* batch4,
    _Atomic uint64_t* batch8,
    _Atomic uint64_t* batch16,
    _Atomic uint64_t* batch_other,
    uint32_t batch_count) {
  if (batch_count == 4u) {
    hz5_largefront_counter_inc(batch4);
  } else if (batch_count == 8u) {
    hz5_largefront_counter_inc(batch8);
  } else if (batch_count == 16u) {
    hz5_largefront_counter_inc(batch16);
  } else {
    hz5_largefront_counter_inc(batch_other);
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OBSERVE
static void hz5_largefront_obs_max(_Atomic uint64_t* counter, uint64_t value) {
  uint64_t old = atomic_load_explicit(counter, memory_order_relaxed);
  while (old < value &&
         !atomic_compare_exchange_weak_explicit(counter,
                                                &old,
                                                value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}

static void hz5_largefront_obs_note_source_refill(uint32_t class_index,
                                                  uint32_t batch_count) {
  if (!hz5_largefront_class_valid(class_index)) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_obs_source_refill[class_index]);
  hz5_largefront_counter_add(&g_hz5_largefront_obs_source_spans[class_index],
                             batch_count);
  hz5_largefront_counter_note_source_batch(
      &g_hz5_largefront_obs_source_batch4[class_index],
      &g_hz5_largefront_obs_source_batch8[class_index],
      &g_hz5_largefront_obs_source_batch16[class_index],
      &g_hz5_largefront_obs_source_batch_other[class_index],
      batch_count);
}

__attribute__((destructor)) static void hz5_largefront_obs_dump(void) {
  if (!getenv("HZ5_LARGEFRONT_OBSERVE")) {
    return;
  }
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    fprintf(stderr,
            "[HZ5_LARGEFRONT_OBSERVE] class=%u bytes=%u"
            " alloc_calls=%llu local_pop_hit=%llu remote_take_first=%llu"
            " remote_to_local=%llu global_pop_hit=%llu new_span=%llu"
            " free_local=%llu free_remote=%llu free_global=%llu"
            " source_refill=%llu source_spans=%llu"
            " source_batch4=%llu source_batch8=%llu"
            " source_batch16=%llu source_batch_other=%llu"
            " local_free_highwater=%llu\n",
            i,
            hz5_largefront_class_bytes(i),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_alloc_calls[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_local_pop_hit[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_remote_take_first[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_remote_to_local[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_global_pop_hit[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_new_span[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_free_local[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_free_remote[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_free_global[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_refill[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_spans[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch4[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch8[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch16[i], memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_source_batch_other[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_obs_local_free_highwater[i],
                memory_order_relaxed));
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L0
static void hz5_largefront_policy_note_source_empty(uint32_t class_index) {
  if (!hz5_largefront_class_valid(class_index)) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_source_empty[class_index]);
}

static void hz5_largefront_policy_note_source_refill(uint32_t class_index,
                                                     uint32_t batch_count) {
  if (!hz5_largefront_class_valid(class_index)) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_source_refill[class_index]);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_source_spans[class_index], batch_count);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_mapped_spans[class_index], batch_count);
  hz5_largefront_counter_note_source_batch(
      &g_hz5_largefront_policy_source_batch4[class_index],
      &g_hz5_largefront_policy_source_batch8[class_index],
      &g_hz5_largefront_policy_source_batch16[class_index],
      &g_hz5_largefront_policy_source_batch_other[class_index],
      batch_count);
}

static void hz5_largefront_policy_note_remote_flush(uint32_t class_index,
                                                    uint32_t count,
                                                    uint32_t cap) {
  if (!hz5_largefront_class_valid(class_index) || count == 0u) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_remote_flush[class_index]);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_remote_flush_spans[class_index], count);
  if (cap == 0u) {
    cap = HZ5_LARGEFRONT_REMOTE_BATCH_CAP;
  }
  if (count >= cap) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_remote_flush_cap_hit[class_index]);
  }
}

static void hz5_largefront_policy_note_owner_drain(uint32_t class_index,
                                                   uint32_t spans,
                                                   uint32_t take_first,
                                                   uint32_t to_local,
                                                   uint32_t republished,
                                                   uint32_t held,
                                                   uint32_t orphaned,
                                                   uint32_t state_fail) {
  if (!hz5_largefront_class_valid(class_index) || spans == 0u) {
    return;
  }
  hz5_largefront_counter_inc(
      &g_hz5_largefront_policy_owner_drain[class_index]);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_drain_spans[class_index], spans);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_take_first[class_index], take_first);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_to_local[class_index], to_local);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_republish[class_index], republished);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_hold[class_index], held);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_orphan[class_index], orphaned);
  hz5_largefront_counter_add(
      &g_hz5_largefront_policy_owner_state_fail[class_index], state_fail);
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L8_SHADOW
  if (spans >= HZ5_LARGEFRONT_POLICY_L8_HEAVY_SPANS) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_l8_heavy_drain[class_index]);
  } else {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_l8_sparse_drain[class_index]);
  }

  // Shadow classification only. It records which sink dominated this drain;
  // it never changes the owner handoff behavior.
  if (to_local >= held && to_local >= republished && to_local != 0u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_l8_local_like[class_index]);
  } else if (held >= to_local && held >= republished && held != 0u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_l8_hold_like[class_index]);
  } else if (republished >= to_local && republished >= held &&
             republished != 0u) {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_l8_republish_like[class_index]);
  } else {
    hz5_largefront_counter_inc(
        &g_hz5_largefront_policy_l8_mixed_like[class_index]);
  }
#endif
}

__attribute__((destructor)) static void hz5_largefront_policy_l0_dump(void) {
  if (!getenv("HZ5_LARGEFRONT_POLICY_L0")) {
    return;
  }
  for (uint32_t i = 0; i < HZ5_LARGEFRONT_CLASS_COUNT; ++i) {
    fprintf(stderr,
            "[HZ5_LARGEFRONT_POLICY_L0] class=%u bytes=%u"
            " source_empty=%llu source_refill=%llu source_spans=%llu"
            " source_batch4=%llu source_batch8=%llu"
            " source_batch16=%llu source_batch_other=%llu"
            " mapped_spans_proxy=%llu"
            " remote_flush=%llu remote_flush_spans=%llu"
            " remote_flush_cap_hit=%llu"
            " owner_drain=%llu owner_drain_spans=%llu"
            " owner_take_first=%llu owner_to_local=%llu"
            " owner_republish=%llu owner_hold=%llu"
            " owner_orphan=%llu owner_state_fail=%llu"
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L8_SHADOW
            " l8_heavy_drain=%llu l8_sparse_drain=%llu"
            " l8_local_like=%llu l8_hold_like=%llu"
            " l8_republish_like=%llu l8_mixed_like=%llu"
#endif
            "\n",
            i,
            hz5_largefront_class_bytes(i),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_empty[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_refill[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch4[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch8[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch16[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_source_batch_other[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_mapped_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_remote_flush[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_remote_flush_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_remote_flush_cap_hit[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_drain[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_drain_spans[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_take_first[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_to_local[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_republish[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_hold[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_orphan[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_owner_state_fail[i],
                memory_order_relaxed)
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L8_SHADOW
                ,
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_l8_heavy_drain[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_l8_sparse_drain[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_l8_local_like[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_l8_hold_like[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_l8_republish_like[i],
                memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(
                &g_hz5_largefront_policy_l8_mixed_like[i],
                memory_order_relaxed)
#endif
            );
  }
}
#endif

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L1B
static uint32_t hz5_largefront_policy_l1b_clamp_cap(uint32_t cap) {
  if (cap < HZ5_LARGEFRONT_POLICY_L1B_MIN_CAP) {
    return HZ5_LARGEFRONT_POLICY_L1B_MIN_CAP;
  }
  if (cap > HZ5_LARGEFRONT_POLICY_L1B_MAX_CAP) {
    return HZ5_LARGEFRONT_POLICY_L1B_MAX_CAP;
  }
  return cap;
}

static void hz5_largefront_policy_l1b_note_remote_flush(Hz5LargeTls* tls,
                                                        uint32_t class_index,
                                                        uint32_t count) {
  if (!tls || !hz5_largefront_class_valid(class_index) ||
      hz5_largefront_class_bytes(class_index) != 131072u || count == 0u) {
    return;
  }
  uint32_t cap = hz5_largefront_policy_l1b_clamp_cap(tls->remote_batch_cap);
  if (count >= cap && cap < HZ5_LARGEFRONT_POLICY_L1B_MAX_CAP) {
    cap <<= 1;
  } else if (count * 4u <= cap && cap > HZ5_LARGEFRONT_POLICY_L1B_MIN_CAP) {
    cap >>= 1;
  }
  tls->remote_batch_cap = hz5_largefront_policy_l1b_clamp_cap(cap);
}
#endif
