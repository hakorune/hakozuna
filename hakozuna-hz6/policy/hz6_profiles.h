#ifndef HZ6_PROFILES_H
#define HZ6_PROFILES_H

#include "../include/hz6_config.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6ProfileId {
  HZ6_PROFILE_STRICT = 0,
  HZ6_PROFILE_SPEED = 1,
  HZ6_PROFILE_RSS = 2,
  HZ6_PROFILE_REMOTE = 3
} Hz6ProfileId;

typedef enum Hz6TransferShardPolicy {
  HZ6_TRANSFER_SHARD_OWNER_SLOT = 1,
  HZ6_TRANSFER_SHARD_CLASS_ID = 2
} Hz6TransferShardPolicy;

typedef struct Hz6ProfileConfig {
  Hz6ProfileId id;
  uint32_t transfer_first;
  uint32_t strict_owner_remote;
  uint32_t transfer_capacity;
  uint32_t transfer_shards;
  Hz6TransferShardPolicy transfer_shard_policy;
  size_t route_page_granularity;
  uint32_t source_batch;
  size_t scavenge_local_free_bytes;
  size_t scavenge_orphan_bytes;
} Hz6ProfileConfig;

Hz6ProfileConfig hz6_profile_config(Hz6ProfileId id);

size_t hz6_profile_transfer_producer_shard(const Hz6ProfileConfig* config,
                                           uint32_t owner_slot,
                                           uint16_t class_id);

size_t hz6_profile_transfer_consumer_shard(const Hz6ProfileConfig* config,
                                           uint32_t owner_slot,
                                           uint16_t class_id);

#ifdef __cplusplus
}
#endif

#endif
