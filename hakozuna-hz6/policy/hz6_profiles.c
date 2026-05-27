#include "hz6_profiles.h"

Hz6ProfileConfig hz6_profile_config(Hz6ProfileId id) {
  Hz6ProfileConfig config;
  config.id = id;
  config.transfer_first = 0;
  config.strict_owner_remote = 1;
  config.transfer_capacity = 16;
  config.transfer_shards = 1;
  config.source_batch = 1;

  switch (id) {
    case HZ6_PROFILE_SPEED:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 64;
      config.transfer_shards = 4;
      config.source_batch = 16;
      break;
    case HZ6_PROFILE_RSS:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 16;
      config.transfer_shards = 1;
      config.source_batch = 4;
      break;
    case HZ6_PROFILE_REMOTE:
      config.transfer_first = 1;
      config.strict_owner_remote = 0;
      config.transfer_capacity = 128;
      config.transfer_shards = 4;
      config.source_batch = 16;
      break;
    case HZ6_PROFILE_STRICT:
    default:
      break;
  }

  return config;
}
