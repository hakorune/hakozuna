#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_ROUTE
#define HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE ((uintptr_t)UINTPTR_MAX)

enum {
  HZ5_WIN_LOCAL2P_SIDECAR_MISS = 0,
  HZ5_WIN_LOCAL2P_SIDECAR_VALID = 1,
  HZ5_WIN_LOCAL2P_SIDECAR_INVALID = 2
};

static size_t hz5_policy_win_local2p_sidecar_hash(uintptr_t aligned) {
  uint64_t x = (uint64_t)(aligned >> 13);
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  return (size_t)(x % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE);
}

static int hz5_policy_win_local2p_sidecar_header_valid(
    const Hz5WrapperHdr* header,
    uintptr_t raw,
    uint32_t generation) {
  if (!header || raw != header->raw ||
      header->magic != HZ5_WRAPPER_HDR_MAGIC ||
      header->layout_version != HZ5_WRAPPER_LAYOUT_VERSION ||
      header->layout_size != sizeof(Hz5WrapperHdr) ||
      header->source != HZ5_WRAPPER_SOURCE_WIN_LOCAL2P ||
      header->requested != HZ5_POLICY_LOCAL2P_EXACT_SIZE ||
      header->raw_bytes != BENCHLAB_HZ5_WIN_LOCAL2P_RAW_BYTES ||
      header->local2p_generation != generation) {
    return 0;
  }
  uint32_t state =
      atomic_load_explicit(&header->local2p_state, memory_order_acquire);
  return state == HZ5_LOCAL2P_STATE_ACTIVE;
}

#if BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_DIRECT_TABLE
static void hz5_policy_win_local2p_sidecar_insert(
    uintptr_t aligned,
    Hz5WrapperHdr* header) {
  if (!aligned || !header) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_FAIL);
    return;
  }

  size_t index = hz5_policy_win_local2p_sidecar_hash(aligned);
  Hz5PolicyWinLocal2PSidecarEntry* entry =
      &g_hz5_policy_win_local2p_sidecar[index];
  uintptr_t current =
      atomic_load_explicit(&entry->aligned, memory_order_acquire);
  if (current != 0u && current != aligned) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
  }

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

static void hz5_policy_win_local2p_sidecar_remove_raw(uintptr_t raw) {
  if (!raw) {
    return;
  }
  for (size_t i = 0; i < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE; ++i) {
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[i];
    uintptr_t entry_raw =
        atomic_load_explicit(&entry->raw, memory_order_acquire);
    if (entry_raw == raw) {
      atomic_store_explicit(&entry->aligned, 0u, memory_order_release);
      atomic_store_explicit(&entry->raw, 0u, memory_order_relaxed);
      atomic_store_explicit(&entry->header, 0u, memory_order_relaxed);
      atomic_store_explicit(&entry->generation, 0u, memory_order_relaxed);
      atomic_store_explicit(&entry->state_snapshot, 0u, memory_order_relaxed);
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE);
      return;
    }
  }
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

  size_t index = hz5_policy_win_local2p_sidecar_hash(aligned);
  Hz5PolicyWinLocal2PSidecarEntry* entry =
      &g_hz5_policy_win_local2p_sidecar[index];
  uintptr_t entry_aligned =
      atomic_load_explicit(&entry->aligned, memory_order_acquire);
  if (entry_aligned != aligned) {
    if (entry_aligned != 0u) {
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_MISS);
    return HZ5_WIN_LOCAL2P_SIDECAR_MISS;
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
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_VALID);
    return HZ5_WIN_LOCAL2P_SIDECAR_VALID;
  }

  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_INVALID);
  return HZ5_WIN_LOCAL2P_SIDECAR_INVALID;
}
#else
static void hz5_policy_win_local2p_sidecar_insert(
    uintptr_t aligned,
    Hz5WrapperHdr* header) {
  if (!aligned || !header) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_FAIL);
    return;
  }

  size_t start = hz5_policy_win_local2p_sidecar_hash(aligned);
  size_t reusable = BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
  for (size_t probe = 0; probe < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
       ++probe) {
    size_t index =
        (start + probe) % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[index];
    if (entry->aligned == HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE) {
      if (reusable == BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE) {
        reusable = index;
      }
      continue;
    }
    if (entry->aligned == 0u) {
      if (reusable == BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE) {
        reusable = index;
      }
      break;
    }
    if (entry->aligned == aligned) {
      reusable = index;
      break;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
  }
  if (reusable != BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE) {
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[reusable];
    entry->aligned = aligned;
    entry->raw = header->raw;
    entry->header = (uintptr_t)header;
    entry->generation = header->local2p_generation;
    entry->state_snapshot =
        atomic_load_explicit(&header->local2p_state, memory_order_relaxed);
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT);
    ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
    return;
  }
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_INSERT_FAIL);
}

static void hz5_policy_win_local2p_sidecar_remove_raw(uintptr_t raw) {
  if (!raw) {
    return;
  }
  AcquireSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
  for (size_t i = 0; i < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE; ++i) {
    Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[i];
    if (entry->aligned != 0u &&
        entry->aligned != HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE &&
        entry->raw == raw) {
      entry->aligned = HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE;
      entry->raw = 0u;
      entry->header = 0u;
      entry->generation = 0u;
      entry->state_snapshot = 0u;
      hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_REMOVE);
      ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
      return;
    }
  }
  ReleaseSRWLockExclusive(&g_hz5_policy_win_local2p_sidecar_lock);
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

  int found = 0;
  int valid = 0;
  const Hz5WrapperHdr* found_header = NULL;
  size_t start = hz5_policy_win_local2p_sidecar_hash(aligned);
  AcquireSRWLockShared(&g_hz5_policy_win_local2p_sidecar_lock);
  for (size_t probe = 0; probe < BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
       ++probe) {
    size_t index =
        (start + probe) % BENCHLAB_HZ5_WIN_LOCAL2P_SIDECAR_TABLE_SIZE;
    const Hz5PolicyWinLocal2PSidecarEntry* entry =
        &g_hz5_policy_win_local2p_sidecar[index];
    if (entry->aligned == 0u) {
      break;
    }
    if (entry->aligned == HZ5_WIN_LOCAL2P_SIDECAR_TOMBSTONE) {
      continue;
    }
    if (entry->aligned == aligned) {
      const Hz5WrapperHdr* header = (const Hz5WrapperHdr*)entry->header;
      found = 1;
      found_header = header;
      valid = hz5_policy_win_local2p_sidecar_header_valid(
          header, entry->raw, entry->generation);
      break;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_COLLISION);
  }
  ReleaseSRWLockShared(&g_hz5_policy_win_local2p_sidecar_lock);

  if (!found) {
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_MISS);
    return HZ5_WIN_LOCAL2P_SIDECAR_MISS;
  }
  if (valid) {
    if (header_out) {
      *header_out = (Hz5WrapperHdr*)found_header;
    }
    hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_VALID);
    return HZ5_WIN_LOCAL2P_SIDECAR_VALID;
  }
  hz5_trace_inc(HZ5_TRACE_LOCAL2P_SIDECAR_HIT_INVALID);
  return HZ5_WIN_LOCAL2P_SIDECAR_INVALID;
}
#endif
#endif
