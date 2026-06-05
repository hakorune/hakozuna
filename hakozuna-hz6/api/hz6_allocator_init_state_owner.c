#include "hz6_allocator_init_internal.h"

static uint32_t g_hz6_allocator_owner_slot_seed = 1;

void hz6_allocator_init_state_owner(Hz6Allocator* allocator,
                                    Hz6ProfileId profile_id) {
  allocator->profile = hz6_profile_config(profile_id);
  allocator->owner.token.slot = g_hz6_allocator_owner_slot_seed++;
  allocator->owner.token.generation = 1;
  allocator->owner.state = HZ6_OWNER_ALIVE;
  allocator->stats = (Hz6StatsSnapshot){0};
#if HZ6_DIAGNOSTIC_PROBES
  allocator->diagnostic_descriptor_live_current = 0;
  allocator->diagnostic_source_block_active_current = 0;
  allocator->diagnostic_frontcache_total_current = 0;
#endif
  hz6_source_registry_init(&allocator->source_registry);
}
