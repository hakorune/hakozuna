#include "hz12_owner_retire_gate.h"

#include "hz12_owner_epoch.h"
#include "hz12_port.h"
#include "hz12_token_inbox.h"

#include <string.h>

static H12OwnerRetireGateStats h12_owner_retire_gate_counters;
static HZ12_MUTEX h12_owner_retire_gate_lock;
static int h12_owner_retire_gate_initialized;

void h12_owner_retire_gate_reset(void) {
  if (!h12_owner_retire_gate_initialized) {
    hz12_mutex_init(&h12_owner_retire_gate_lock);
    h12_owner_retire_gate_initialized = 1;
  }
  hz12_mutex_lock(&h12_owner_retire_gate_lock);
  memset(&h12_owner_retire_gate_counters, 0,
         sizeof(h12_owner_retire_gate_counters));
  hz12_mutex_unlock(&h12_owner_retire_gate_lock);
}

int h12_owner_retire_gate_ready(H12OwnerToken owner) {
  int epoch_ready;
  uint32_t pending;
  int ready;
  if (!h12_owner_retire_gate_initialized) return 0;
  epoch_ready = h12_owner_epoch_ready_to_dead(owner);
  pending = h12_token_inbox_pending(owner);
  ready = epoch_ready && pending == 0u;
  hz12_mutex_lock(&h12_owner_retire_gate_lock);
  h12_owner_retire_gate_counters.query += 1u;
  h12_owner_retire_gate_counters.last_pending = pending;
  if (!epoch_ready) {
    h12_owner_retire_gate_counters.blocked_epoch += 1u;
  } else if (pending != 0u) {
    h12_owner_retire_gate_counters.blocked_inbox += 1u;
  } else {
    h12_owner_retire_gate_counters.open += 1u;
  }
  hz12_mutex_unlock(&h12_owner_retire_gate_lock);
  return ready;
}

void h12_owner_retire_gate_stats(H12OwnerRetireGateStats* out) {
  if (!out || !h12_owner_retire_gate_initialized) return;
  hz12_mutex_lock(&h12_owner_retire_gate_lock);
  *out = h12_owner_retire_gate_counters;
  hz12_mutex_unlock(&h12_owner_retire_gate_lock);
}

void h12_owner_retire_gate_dump(FILE* out) {
  H12OwnerRetireGateStats stats;
  if (!out) return;
  memset(&stats, 0, sizeof(stats));
  h12_owner_retire_gate_stats(&stats);
  fprintf(out,
          "[HZ12_OWNER_RETIRE_GATE] query=%llu open=%llu blocked_epoch=%llu "
          "blocked_inbox=%llu last_pending=%u\n",
          (unsigned long long)stats.query, (unsigned long long)stats.open,
          (unsigned long long)stats.blocked_epoch,
          (unsigned long long)stats.blocked_inbox, stats.last_pending);
}
