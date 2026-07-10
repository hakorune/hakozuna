#include "hz12_owner_registry.h"

#include "hz12_port.h"

#include <string.h>

typedef struct H12OwnerEntry {
  uint32_t generation;
  H12OwnerState state;
} H12OwnerEntry;

static H12OwnerEntry h12_owner_entries[HZ12_OWNER_REGISTRY_CAP];
static H12OwnerRegistryStats h12_owner_stats;
static HZ12_MUTEX h12_owner_lock;
static int h12_owner_initialized;

static int h12_owner_token_matches(H12OwnerToken token,
                                   H12OwnerEntry** out) {
  H12OwnerEntry* entry;
  if (token.slot >= HZ12_OWNER_REGISTRY_CAP || token.generation == 0u) {
    return 0;
  }
  entry = &h12_owner_entries[token.slot];
  if (entry->generation != token.generation) return 0;
  if (out) *out = entry;
  return 1;
}

void h12_owner_registry_reset(void) {
  if (!h12_owner_initialized) {
    hz12_mutex_init(&h12_owner_lock);
    h12_owner_initialized = 1;
  }
  hz12_mutex_lock(&h12_owner_lock);
  memset(h12_owner_entries, 0, sizeof(h12_owner_entries));
  memset(&h12_owner_stats, 0, sizeof(h12_owner_stats));
  hz12_mutex_unlock(&h12_owner_lock);
}

int h12_owner_register(H12OwnerToken* out) {
  uint32_t slot = HZ12_OWNER_REGISTRY_CAP;
  uint32_t i;
  int reused = 0;
  if (!out || !h12_owner_initialized) return 0;
  hz12_mutex_lock(&h12_owner_lock);
  for (i = 0u; i < HZ12_OWNER_REGISTRY_CAP; ++i) {
    if (h12_owner_entries[i].state == HZ12_OWNER_DEAD) {
      slot = i;
      reused = 1;
      break;
    }
  }
  if (slot == HZ12_OWNER_REGISTRY_CAP) {
    for (i = 0u; i < HZ12_OWNER_REGISTRY_CAP; ++i) {
      if (h12_owner_entries[i].state == HZ12_OWNER_FREE) {
        slot = i;
        break;
      }
    }
  }
  if (slot == HZ12_OWNER_REGISTRY_CAP) {
    h12_owner_stats.register_full += 1u;
    hz12_mutex_unlock(&h12_owner_lock);
    return 0;
  }
  h12_owner_entries[slot].generation += 1u;
  if (h12_owner_entries[slot].generation == 0u) {
    h12_owner_entries[slot].generation = 1u;
  }
  h12_owner_entries[slot].state = HZ12_OWNER_ACTIVE;
  out->slot = slot;
  out->generation = h12_owner_entries[slot].generation;
  h12_owner_stats.register_success += 1u;
  if (reused) h12_owner_stats.register_reuse += 1u;
  hz12_mutex_unlock(&h12_owner_lock);
  return 1;
}

int h12_owner_begin_retire(H12OwnerToken token) {
  H12OwnerEntry* entry;
  int success = 0;
  if (!h12_owner_initialized) return 0;
  hz12_mutex_lock(&h12_owner_lock);
  if (h12_owner_token_matches(token, &entry) &&
      entry->state == HZ12_OWNER_ACTIVE) {
    entry->state = HZ12_OWNER_RETIRING;
    h12_owner_stats.retire_success += 1u;
    success = 1;
  } else {
    h12_owner_stats.invalid_transition += 1u;
  }
  hz12_mutex_unlock(&h12_owner_lock);
  return success;
}

int h12_owner_mark_dead(H12OwnerToken token) {
  H12OwnerEntry* entry;
  int success = 0;
  if (!h12_owner_initialized) return 0;
  hz12_mutex_lock(&h12_owner_lock);
  if (h12_owner_token_matches(token, &entry) &&
      entry->state == HZ12_OWNER_RETIRING) {
    entry->state = HZ12_OWNER_DEAD;
    h12_owner_stats.dead_success += 1u;
    success = 1;
  } else {
    h12_owner_stats.invalid_transition += 1u;
  }
  hz12_mutex_unlock(&h12_owner_lock);
  return success;
}

int h12_owner_publishable(H12OwnerToken token) {
  H12OwnerEntry* entry;
  int publishable = 0;
  if (!h12_owner_initialized) return 0;
  hz12_mutex_lock(&h12_owner_lock);
  if (token.slot >= HZ12_OWNER_REGISTRY_CAP || token.generation == 0u) {
    h12_owner_stats.publish_invalid_reject += 1u;
  } else if (h12_owner_entries[token.slot].generation != token.generation) {
    h12_owner_stats.publish_stale_reject += 1u;
  } else {
    entry = &h12_owner_entries[token.slot];
    if (entry->state == HZ12_OWNER_ACTIVE ||
        entry->state == HZ12_OWNER_RETIRING) {
      h12_owner_stats.publish_accept += 1u;
      publishable = 1;
    } else {
      h12_owner_stats.publish_dead_reject += 1u;
    }
  }
  hz12_mutex_unlock(&h12_owner_lock);
  return publishable;
}

H12OwnerState h12_owner_state(H12OwnerToken token) {
  H12OwnerState state = HZ12_OWNER_FREE;
  H12OwnerEntry* entry;
  if (!h12_owner_initialized) return state;
  hz12_mutex_lock(&h12_owner_lock);
  if (h12_owner_token_matches(token, &entry)) state = entry->state;
  hz12_mutex_unlock(&h12_owner_lock);
  return state;
}

void h12_owner_registry_stats(H12OwnerRegistryStats* out) {
  if (!out || !h12_owner_initialized) return;
  hz12_mutex_lock(&h12_owner_lock);
  *out = h12_owner_stats;
  hz12_mutex_unlock(&h12_owner_lock);
}

void h12_owner_registry_dump(FILE* out) {
  H12OwnerRegistryStats stats;
  if (!out) return;
  memset(&stats, 0, sizeof(stats));
  h12_owner_registry_stats(&stats);
  fprintf(out,
          "[HZ12_OWNER_REGISTRY] register=%llu reuse=%llu full=%llu "
          "retire=%llu dead=%llu publish_accept=%llu stale_reject=%llu "
          "dead_reject=%llu invalid_reject=%llu invalid_transition=%llu\n",
          (unsigned long long)stats.register_success,
          (unsigned long long)stats.register_reuse,
          (unsigned long long)stats.register_full,
          (unsigned long long)stats.retire_success,
          (unsigned long long)stats.dead_success,
          (unsigned long long)stats.publish_accept,
          (unsigned long long)stats.publish_stale_reject,
          (unsigned long long)stats.publish_dead_reject,
          (unsigned long long)stats.publish_invalid_reject,
          (unsigned long long)stats.invalid_transition);
}
