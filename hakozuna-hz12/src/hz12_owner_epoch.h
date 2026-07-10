#ifndef HZ12_OWNER_EPOCH_H
#define HZ12_OWNER_EPOCH_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

#ifndef HZ12_OWNER_EPOCH_PARTICIPANT_CAP
#define HZ12_OWNER_EPOCH_PARTICIPANT_CAP 16u
#endif

typedef struct H12OwnerEpochParticipant {
  uint32_t slot;
  uint32_t generation;
} H12OwnerEpochParticipant;

typedef struct H12OwnerEpochStats {
  uint64_t participant_register;
  uint64_t participant_unregister;
  uint64_t retire_begin;
  uint64_t checkpoint;
  uint64_t ready_yes;
  uint64_t ready_no;
  uint64_t stale_checkpoint;
  uint32_t active_participants;
  uint32_t pending_participants;
} H12OwnerEpochStats;

void h12_owner_epoch_reset(void);
int h12_owner_epoch_register(H12OwnerEpochParticipant* out);
int h12_owner_epoch_unregister(H12OwnerEpochParticipant participant);
int h12_owner_epoch_begin_retire(H12OwnerToken owner);
int h12_owner_epoch_checkpoint(H12OwnerEpochParticipant participant);
int h12_owner_epoch_ready_to_dead(H12OwnerToken owner);
void h12_owner_epoch_stats(H12OwnerEpochStats* out);
void h12_owner_epoch_dump(FILE* out);

#endif /* HZ12_OWNER_EPOCH_H */
