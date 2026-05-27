#ifndef HZ6_PROFILES_H
#define HZ6_PROFILES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6ProfileId {
  HZ6_PROFILE_STRICT = 0,
  HZ6_PROFILE_SPEED = 1,
  HZ6_PROFILE_RSS = 2,
  HZ6_PROFILE_REMOTE = 3
} Hz6ProfileId;

typedef struct Hz6ProfileConfig {
  Hz6ProfileId id;
  uint32_t transfer_first;
  uint32_t strict_owner_remote;
  uint32_t transfer_capacity;
  uint32_t source_batch;
} Hz6ProfileConfig;

Hz6ProfileConfig hz6_profile_config(Hz6ProfileId id);

#ifdef __cplusplus
}
#endif

#endif

