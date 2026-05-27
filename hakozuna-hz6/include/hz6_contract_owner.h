#ifndef HZ6_CONTRACT_OWNER_H
#define HZ6_CONTRACT_OWNER_H

#include <stdint.h>

typedef enum Hz6ObjectState {
  HZ6_STATE_DEAD = 0,
  HZ6_STATE_ACTIVE = 1,
  HZ6_STATE_LOCAL_FREE = 2,
  HZ6_STATE_TRANSFER_FREE = 3,
  HZ6_STATE_REMOTE_PENDING = 4,
  HZ6_STATE_ORPHAN = 5
} Hz6ObjectState;

typedef struct Hz6OwnerToken {
  uint32_t slot;
  uint32_t generation;
} Hz6OwnerToken;

#endif
