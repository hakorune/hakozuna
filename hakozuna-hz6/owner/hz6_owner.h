#ifndef HZ6_OWNER_H
#define HZ6_OWNER_H

#include "../include/hz6_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6OwnerState {
  HZ6_OWNER_EMPTY = 0,
  HZ6_OWNER_ALIVE = 1,
  HZ6_OWNER_DYING = 2,
  HZ6_OWNER_DEAD = 3
} Hz6OwnerState;

typedef struct Hz6OwnerRecord {
  Hz6OwnerToken token;
  Hz6OwnerState state;
} Hz6OwnerRecord;

int hz6_owner_equal(Hz6OwnerToken a, Hz6OwnerToken b);
int hz6_owner_is_alive(const Hz6OwnerRecord* record, Hz6OwnerToken token);

#ifdef __cplusplus
}
#endif

#endif
