#include "hz6_profiles.h"

static size_t hz6_profile_transfer_owner_shard(
    const Hz6ProfileConfig* config,
    uint32_t owner_slot,
    uint16_t class_id) {
  if (!config || config->transfer_shards <= 1) {
    return 0;
  }
  switch (config->transfer_shard_policy) {
    case HZ6_TRANSFER_SHARD_CLASS_ID:
      return ((size_t)class_id) % (size_t)config->transfer_shards;
    case HZ6_TRANSFER_SHARD_OWNER_SLOT:
    default:
#if HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID > 0
      if (class_id <= (uint16_t)HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID) {
        return ((size_t)class_id) % (size_t)config->transfer_shards;
      }
#endif
      return ((size_t)owner_slot) % (size_t)config->transfer_shards;
  }
}

size_t hz6_profile_transfer_producer_shard(const Hz6ProfileConfig* config,
                                           uint32_t owner_slot,
                                           uint16_t class_id) {
  return hz6_profile_transfer_owner_shard(config, owner_slot, class_id);
}

size_t hz6_profile_transfer_consumer_shard(const Hz6ProfileConfig* config,
                                           uint32_t owner_slot,
                                           uint16_t class_id) {
  return hz6_profile_transfer_owner_shard(config, owner_slot, class_id);
}

size_t hz6_profile_source_refill_batch(const Hz6ProfileConfig* config,
                                       uint16_t front_id,
                                       uint16_t class_id) {
  (void)front_id;
  (void)class_id;
  if (!config || config->source_batch == 0) {
    return 1;
  }
  return (size_t)config->source_batch;
}

size_t hz6_profile_scavenge_orphan_budget(const Hz6ProfileConfig* config) {
  return config ? config->scavenge_orphan_bytes : 0;
}

size_t hz6_profile_scavenge_local_free_budget(
    const Hz6ProfileConfig* config) {
  return config ? config->scavenge_local_free_bytes : 0;
}
