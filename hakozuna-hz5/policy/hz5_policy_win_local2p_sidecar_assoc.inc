#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_DIRECT_TABLE
#error "BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_TABLE requires direct-table sidecar mode to be off"
#endif
#ifndef HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE
#define HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE ((uintptr_t)UINTPTR_MAX)
#endif
#ifndef HZ5_WIN_LOCAL2P_SIDECAR_MISS
enum {
  HZ5_WIN_LOCAL2P_SIDECAR_MISS = 0,
  HZ5_WIN_LOCAL2P_SIDECAR_VALID = 1,
  HZ5_WIN_LOCAL2P_SIDECAR_INVALID = 2
};
#endif
#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_TABLE
#if (BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS == 0u) || \
    ((BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS & \
      (BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS - 1u)) != 0u)
#error "BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS must be a non-zero power of two"
#endif
#if (BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE % \
     BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS) != 0u
#error "BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE must be divisible by BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS"
#endif

static size_t hz5_policy_win_local2p_sidecar_assoc_set_count(void) {
  return BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE /
         BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS;
}

static size_t hz5_policy_win_local2p_sidecar_assoc_way_count(void) {
  return BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ASSOC_WAYS;
}

static size_t hz5_policy_win_local2p_sidecar_assoc_set_index(uintptr_t aligned) {
  size_t set_count = hz5_policy_win_local2p_sidecar_assoc_set_count();
  if (set_count == 0u) {
    return 0u;
  }
  return hz5_policy_win_local2p_sidecar_hash(aligned) % set_count;
}

static size_t hz5_policy_win_local2p_sidecar_assoc_entry_index(uintptr_t aligned,
                                                               size_t way) {
  size_t set_base =
      hz5_policy_win_local2p_sidecar_assoc_set_index(aligned) *
      hz5_policy_win_local2p_sidecar_assoc_way_count();
  return set_base + way;
}

static size_t hz5_policy_win_local2p_sidecar_assoc_victim_way(
    uintptr_t aligned) {
  size_t way_count = hz5_policy_win_local2p_sidecar_assoc_way_count();
  return (size_t)(((aligned >> 13) ^ (aligned >> 19)) & (way_count - 1u));
}

static void hz5_policy_win_local2p_sidecar_insert(
    uintptr_t aligned,
    Hz5WrapperHdr* header) {
  if (!aligned || !header) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_FAIL);
    return;
  }

  size_t way_count = hz5_policy_win_local2p_sidecar_assoc_way_count();
  size_t base = hz5_policy_win_local2p_sidecar_assoc_entry_index(aligned, 0u);
  size_t empty_index = SIZE_MAX;
  size_t victim_index = SIZE_MAX;

  for (size_t way = 0; way < way_count; ++way) {
    size_t index = base + way;
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[index];
    uintptr_t current =
        atomic_load_explicit(&entry->aligned, memory_order_acquire);
    if (current == aligned) {
      victim_index = index;
      break;
    }
    if (current == 0u) {
      if (empty_index == SIZE_MAX) {
        empty_index = index;
      }
      continue;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_COLLISION);
  }

  if (victim_index == SIZE_MAX) {
    if (empty_index != SIZE_MAX) {
      victim_index = empty_index;
    } else {
      victim_index =
          base + hz5_policy_win_local2p_sidecar_assoc_victim_way(aligned);
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_ASSOC_VICTIM);
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_COLLISION);
    }
  }

  Hz5PolicyWinLocal2PSidecarEntry* entry =
      &g_hz5_policy_win_local2p_sidecar[victim_index];
  atomic_store_explicit(&entry->raw, header->raw, memory_order_relaxed);
  atomic_store_explicit(&entry->header, (uintptr_t)header,
                        memory_order_relaxed);
  atomic_store_explicit(&entry->generation, header->local2p_generation,
                        memory_order_relaxed);
  atomic_store_explicit(
      &entry->state_snapshot,
      atomic_load_explicit(&header->local2p_state, memory_order_relaxed),
      memory_order_relaxed);
  atomic_store_explicit(&entry->aligned, aligned, memory_order_release);
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT);
}

static void hz5_policy_win_local2p_sidecar_remove_raw(uintptr_t raw,
                                                      uintptr_t aligned) {
  if (!raw || !aligned) {
    return;
  }

  size_t way_count = hz5_policy_win_local2p_sidecar_assoc_way_count();
  size_t base = hz5_policy_win_local2p_sidecar_assoc_entry_index(aligned, 0u);
  size_t scanned = 0u;
  for (size_t way = 0; way < way_count; ++way) {
    ++scanned;
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[base + way];
    uintptr_t entry_aligned =
        atomic_load_explicit(&entry->aligned, memory_order_acquire);
    uintptr_t entry_raw =
        atomic_load_explicit(&entry->raw, memory_order_acquire);
    if (entry_aligned == aligned || entry_raw == raw) {
      atomic_store_explicit(&entry->aligned, 0u, memory_order_release);
      atomic_store_explicit(&entry->raw, 0u, memory_order_relaxed);
      atomic_store_explicit(&entry->header, 0u, memory_order_relaxed);
      atomic_store_explicit(&entry->generation, 0u, memory_order_relaxed);
      atomic_store_explicit(&entry->state_snapshot, 0u, memory_order_relaxed);
      hz5_trace_add(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE_SCAN_STEPS,
                    (uint64_t)scanned);
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE);
      return;
    }
  }

  hz5_trace_add(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE_SCAN_STEPS,
                (uint64_t)scanned);
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE_MISS);
}

static int hz5_policy_win_local2p_sidecar_probe(uintptr_t aligned,
                                                Hz5WrapperHdr** header_out) {
  if (header_out) {
    *header_out = NULL;
  }
  if (!aligned) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_MISS);
    return HZ5_WIN_LOCAL2P_SIDECAR_MISS;
  }

  int saw_empty = 0;
  size_t steps = 0u;
  size_t way_count = hz5_policy_win_local2p_sidecar_assoc_way_count();
  size_t base = hz5_policy_win_local2p_sidecar_assoc_entry_index(aligned, 0u);
  for (size_t way = 0; way < way_count; ++way) {
    ++steps;
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[base + way];
    uintptr_t entry_aligned =
        atomic_load_explicit(&entry->aligned, memory_order_acquire);
    if (entry_aligned != aligned) {
      if (entry_aligned != 0u) {
        hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
        hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_PROBE_COLLISION);
      } else {
        saw_empty = 1;
      }
      continue;
    }

    uintptr_t raw = atomic_load_explicit(&entry->raw, memory_order_acquire);
    uintptr_t header_addr =
        atomic_load_explicit(&entry->header, memory_order_acquire);
    uint32_t generation =
        atomic_load_explicit(&entry->generation, memory_order_acquire);
    Hz5WrapperHdr* header = (Hz5WrapperHdr*)header_addr;
    if (hz5_policy_win_local2p_sidecar_header_valid(header, raw, generation)) {
      if (header_out) {
        *header_out = header;
      }
      hz5_policy_win_local2p_sidecar_note_probe_steps(steps);
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_VALID);
      return HZ5_WIN_LOCAL2P_SIDECAR_VALID;
    }

    hz5_policy_win_local2p_sidecar_note_probe_steps(steps);
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_INVALID);
    return HZ5_WIN_LOCAL2P_SIDECAR_INVALID;
  }

  if (saw_empty) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_PROBE_EMPTY);
  }
  hz5_policy_win_local2p_sidecar_note_probe_steps(steps);
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_MISS);
  return HZ5_WIN_LOCAL2P_SIDECAR_MISS;
}
#endif
#endif
