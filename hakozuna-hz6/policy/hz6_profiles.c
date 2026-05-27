#include "hz6_profiles.h"

Hz6ProfileConfig hz6_profile_config(Hz6ProfileId id) {
  Hz6ProfileConfig config;
  config.id = id;
  config.transfer_first = 0;
  config.strict_owner_remote = 1;
  config.transfer_capacity = 16;
  config.transfer_shards = 1;
  config.transfer_shard_policy = HZ6_TRANSFER_SHARD_OWNER_SLOT;
  config.route_backend_policy = HZ6_ROUTE_POLICY_EXACT_TABLE;
  config.route_page_granularity = 0;
  config.source_kind = HZ6_SOURCE_OS_PAGED;
  config.source_batch = 1;
  config.scavenge_local_free_bytes = 0;
  config.scavenge_orphan_bytes = 0;

  switch (id) {
    case HZ6_PROFILE_SPEED:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 64;
      config.transfer_shards = 4;
      config.route_backend_policy = HZ6_ROUTE_POLICY_PAGE_TABLE;
      config.route_page_granularity = HZ6_ROUTE_PAGE_GRANULARITY;
      config.source_batch = 16;
      config.scavenge_local_free_bytes = 4096;
      config.scavenge_orphan_bytes = 4096;
      break;
    case HZ6_PROFILE_RSS:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 16;
      config.transfer_shards = 1;
      config.source_batch = 4;
      config.scavenge_local_free_bytes = 65536;
      config.scavenge_orphan_bytes = 65536;
      break;
    case HZ6_PROFILE_REMOTE:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 128;
      config.transfer_shards = 4;
      config.route_backend_policy = HZ6_ROUTE_POLICY_PAGE_TABLE;
      config.route_page_granularity = HZ6_ROUTE_PAGE_GRANULARITY;
      config.source_batch = 16;
      config.scavenge_local_free_bytes = 8192;
      config.scavenge_orphan_bytes = 8192;
      break;
    case HZ6_PROFILE_STRICT:
    default:
      break;
  }

  return config;
}

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
