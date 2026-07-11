#include "hz12_owner_epoch.h"

#include "hz12_port.h"

#include <stdatomic.h>
#include <string.h>

typedef struct H12OwnerEpochParticipantEntry {
  uint32_t generation;
  int active;
  atomic_uint acknowledged_epoch;
} H12OwnerEpochParticipantEntry;

typedef struct H12OwnerEpochRetire {
  H12OwnerToken owner;
  uint32_t epoch;
  uint32_t participant_count;
  uint32_t participant_generation[HZ12_OWNER_EPOCH_PARTICIPANT_CAP];
  int active;
} H12OwnerEpochRetire;

static H12OwnerEpochParticipantEntry
    h12_owner_epoch_participants[HZ12_OWNER_EPOCH_PARTICIPANT_CAP];
static H12OwnerEpochRetire h12_owner_epoch_retire;
static H12OwnerEpochStats h12_owner_epoch_counters;
static HZ12_MUTEX h12_owner_epoch_lock;
static int h12_owner_epoch_initialized;

static int h12_owner_epoch_participant_valid(H12OwnerEpochParticipant token,
                                             H12OwnerEpochParticipantEntry** out) {
  H12OwnerEpochParticipantEntry* entry;
  if (token.slot >= HZ12_OWNER_EPOCH_PARTICIPANT_CAP ||
      token.generation == 0u) {
    return 0;
  }
  entry = &h12_owner_epoch_participants[token.slot];
  if (!entry->active || entry->generation != token.generation) return 0;
  if (out) *out = entry;
  return 1;
}

void h12_owner_epoch_reset(void) {
  if (!h12_owner_epoch_initialized) {
    hz12_mutex_init(&h12_owner_epoch_lock);
    h12_owner_epoch_initialized = 1;
  }
  hz12_mutex_lock(&h12_owner_epoch_lock);
  for (uint32_t i = 0u; i < HZ12_OWNER_EPOCH_PARTICIPANT_CAP; ++i) {
    h12_owner_epoch_participants[i].generation = 0u;
    h12_owner_epoch_participants[i].active = 0;
    atomic_store_explicit(&h12_owner_epoch_participants[i].acknowledged_epoch,
                          0u, memory_order_relaxed);
  }
  memset(&h12_owner_epoch_retire, 0, sizeof(h12_owner_epoch_retire));
  memset(&h12_owner_epoch_counters, 0, sizeof(h12_owner_epoch_counters));
  hz12_mutex_unlock(&h12_owner_epoch_lock);
}

int h12_owner_epoch_register(H12OwnerEpochParticipant* out) {
  uint32_t i;
  if (!out || !h12_owner_epoch_initialized) return 0;
  hz12_mutex_lock(&h12_owner_epoch_lock);
  for (i = 0u; i < HZ12_OWNER_EPOCH_PARTICIPANT_CAP; ++i) {
    H12OwnerEpochParticipantEntry* entry = &h12_owner_epoch_participants[i];
    if (!entry->active) {
      entry->generation += 1u;
      if (entry->generation == 0u) entry->generation = 1u;
      entry->active = 1;
      atomic_store_explicit(&entry->acknowledged_epoch, 0u,
                            memory_order_relaxed);
      out->slot = i;
      out->generation = entry->generation;
      h12_owner_epoch_counters.participant_register += 1u;
      h12_owner_epoch_counters.active_participants += 1u;
      hz12_mutex_unlock(&h12_owner_epoch_lock);
      return 1;
    }
  }
  hz12_mutex_unlock(&h12_owner_epoch_lock);
  return 0;
}

int h12_owner_epoch_unregister(H12OwnerEpochParticipant participant) {
  H12OwnerEpochParticipantEntry* entry;
  int result = 0;
  if (!h12_owner_epoch_initialized) return 0;
  hz12_mutex_lock(&h12_owner_epoch_lock);
  if (h12_owner_epoch_participant_valid(participant, &entry)) {
    entry->active = 0;
    h12_owner_epoch_counters.participant_unregister += 1u;
    h12_owner_epoch_counters.active_participants -= 1u;
    result = 1;
  }
  hz12_mutex_unlock(&h12_owner_epoch_lock);
  return result;
}

int h12_owner_epoch_begin_retire(H12OwnerToken owner) {
  uint32_t i;
  uint32_t count = 0u;
  if (!h12_owner_epoch_initialized || owner.generation == 0u) return 0;
  hz12_mutex_lock(&h12_owner_epoch_lock);
  if (h12_owner_epoch_retire.active) {
    hz12_mutex_unlock(&h12_owner_epoch_lock);
    return 0;
  }
  for (i = 0u; i < HZ12_OWNER_EPOCH_PARTICIPANT_CAP; ++i) {
    if (h12_owner_epoch_participants[i].active) {
      h12_owner_epoch_retire.participant_generation[i] =
          h12_owner_epoch_participants[i].generation;
      count += 1u;
    } else {
      h12_owner_epoch_retire.participant_generation[i] = 0u;
    }
  }
  h12_owner_epoch_retire.owner = owner;
  h12_owner_epoch_retire.epoch += 1u;
  if (h12_owner_epoch_retire.epoch == 0u) h12_owner_epoch_retire.epoch = 1u;
  h12_owner_epoch_retire.participant_count = count;
  h12_owner_epoch_retire.active = 1;
  h12_owner_epoch_counters.retire_begin += 1u;
  h12_owner_epoch_counters.pending_participants = count;
  hz12_mutex_unlock(&h12_owner_epoch_lock);
  return 1;
}

int h12_owner_epoch_checkpoint(H12OwnerEpochParticipant participant) {
  H12OwnerEpochParticipantEntry* entry;
  uint32_t epoch;
  if (!h12_owner_epoch_initialized) return 0;
  hz12_mutex_lock(&h12_owner_epoch_lock);
  if (!h12_owner_epoch_participant_valid(participant, &entry) ||
      !h12_owner_epoch_retire.active) {
    h12_owner_epoch_counters.stale_checkpoint += 1u;
    hz12_mutex_unlock(&h12_owner_epoch_lock);
    return 0;
  }
  epoch = h12_owner_epoch_retire.epoch;
  atomic_store_explicit(&entry->acknowledged_epoch, epoch, memory_order_release);
  h12_owner_epoch_counters.checkpoint += 1u;
  hz12_mutex_unlock(&h12_owner_epoch_lock);
  return 1;
}

int h12_owner_epoch_ready_to_dead(H12OwnerToken owner) {
  uint32_t i;
  uint32_t pending = 0u;
  int ready = 0;
  if (!h12_owner_epoch_initialized) return 0;
  hz12_mutex_lock(&h12_owner_epoch_lock);
  if (h12_owner_epoch_retire.active &&
      h12_owner_epoch_retire.owner.slot == owner.slot &&
      h12_owner_epoch_retire.owner.generation == owner.generation) {
    for (i = 0u; i < HZ12_OWNER_EPOCH_PARTICIPANT_CAP; ++i) {
      H12OwnerEpochParticipantEntry* entry = &h12_owner_epoch_participants[i];
      if (h12_owner_epoch_retire.participant_generation[i] != 0u &&
          (entry->generation !=
               h12_owner_epoch_retire.participant_generation[i] ||
           atomic_load_explicit(&entry->acknowledged_epoch,
                                memory_order_acquire) <
               h12_owner_epoch_retire.epoch)) {
        pending += 1u;
      }
    }
    h12_owner_epoch_counters.pending_participants = pending;
    ready = pending == 0u;
  }
  if (ready) {
    h12_owner_epoch_counters.ready_yes += 1u;
  } else {
    h12_owner_epoch_counters.ready_no += 1u;
  }
  hz12_mutex_unlock(&h12_owner_epoch_lock);
  return ready;
}

int h12_owner_epoch_finish_retire(H12OwnerToken owner) {
  int finished = 0;
  if (!h12_owner_epoch_initialized || owner.generation == 0u ||
      !h12_owner_epoch_ready_to_dead(owner)) {
    return 0;
  }
  hz12_mutex_lock(&h12_owner_epoch_lock);
  if (h12_owner_epoch_retire.active &&
      h12_owner_epoch_retire.owner.slot == owner.slot &&
      h12_owner_epoch_retire.owner.generation == owner.generation) {
    h12_owner_epoch_retire.active = 0;
    h12_owner_epoch_retire.participant_count = 0u;
    memset(h12_owner_epoch_retire.participant_generation, 0,
           sizeof(h12_owner_epoch_retire.participant_generation));
    finished = 1;
  }
  hz12_mutex_unlock(&h12_owner_epoch_lock);
  return finished;
}

void h12_owner_epoch_stats(H12OwnerEpochStats* out) {
  if (!out || !h12_owner_epoch_initialized) return;
  hz12_mutex_lock(&h12_owner_epoch_lock);
  *out = h12_owner_epoch_counters;
  hz12_mutex_unlock(&h12_owner_epoch_lock);
}

void h12_owner_epoch_dump(FILE* out) {
  H12OwnerEpochStats stats;
  if (!out) return;
  memset(&stats, 0, sizeof(stats));
  h12_owner_epoch_stats(&stats);
  fprintf(out,
          "[HZ12_OWNER_EPOCH] register=%llu unregister=%llu retire=%llu "
          "checkpoint=%llu ready_yes=%llu ready_no=%llu stale=%llu "
          "active=%u pending=%u\n",
          (unsigned long long)stats.participant_register,
          (unsigned long long)stats.participant_unregister,
          (unsigned long long)stats.retire_begin,
          (unsigned long long)stats.checkpoint,
          (unsigned long long)stats.ready_yes,
          (unsigned long long)stats.ready_no,
          (unsigned long long)stats.stale_checkpoint,
          stats.active_participants, stats.pending_participants);
}
