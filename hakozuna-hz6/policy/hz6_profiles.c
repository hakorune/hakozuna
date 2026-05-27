#include "hz6_profiles.h"

Hz6ProfileConfig hz6_profile_config(Hz6ProfileId id) {
  Hz6ProfileConfig config;
  config.id = id;
  config.transfer_first = 0;
  config.strict_owner_remote = 1;
  config.transfer_capacity = 16;
  config.transfer_shards = 1;
  config.route_page_granularity = 0;
  config.source_batch = 1;
  config.scavenge_local_free_bytes = 0;
  config.scavenge_orphan_bytes = 0;

  switch (id) {
    case HZ6_PROFILE_SPEED:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 64;
      config.transfer_shards = 4;
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
