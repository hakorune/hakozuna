#ifndef HZ12_OWNER_REGISTRY_H
#define HZ12_OWNER_REGISTRY_H

#include <stdint.h>
#include <stdio.h>

#ifndef HZ12_OWNER_REGISTRY_CAP
#define HZ12_OWNER_REGISTRY_CAP 64u
#endif

typedef struct H12OwnerToken {
  uint32_t slot;
  uint32_t generation;
} H12OwnerToken;

typedef enum H12OwnerState {
  HZ12_OWNER_FREE = 0,
  HZ12_OWNER_ACTIVE = 1,
  HZ12_OWNER_RETIRING = 2,
  HZ12_OWNER_DEAD = 3
} H12OwnerState;

typedef struct H12OwnerRegistryStats {
  uint64_t register_success;
  uint64_t register_reuse;
  uint64_t register_full;
  uint64_t retire_success;
  uint64_t dead_success;
  uint64_t publish_accept;
  uint64_t publish_stale_reject;
  uint64_t publish_dead_reject;
  uint64_t publish_invalid_reject;
  uint64_t invalid_transition;
} H12OwnerRegistryStats;

void h12_owner_registry_reset(void);
int h12_owner_register(H12OwnerToken* out);
int h12_owner_begin_retire(H12OwnerToken token);
int h12_owner_mark_dead(H12OwnerToken token);
int h12_owner_publishable(H12OwnerToken token);
H12OwnerState h12_owner_state(H12OwnerToken token);
void h12_owner_registry_stats(H12OwnerRegistryStats* out);
void h12_owner_registry_dump(FILE* out);

#endif /* HZ12_OWNER_REGISTRY_H */
