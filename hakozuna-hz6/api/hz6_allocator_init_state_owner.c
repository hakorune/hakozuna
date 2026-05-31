#include "hz6_allocator_init_internal.h"

static uint32_t g_hz6_allocator_owner_slot_seed = 1;

void hz6_allocator_init_state_owner(Hz6Allocator* allocator,
                                    Hz6ProfileId profile_id) {
  allocator->profile = hz6_profile_config(profile_id);
  allocator->owner.token.slot = g_hz6_allocator_owner_slot_seed++;
  allocator->owner.token.generation = 1;
  allocator->owner.state = HZ6_OWNER_ALIVE;
  allocator->stats.route_valid = 0;
  allocator->stats.route_invalid = 0;
  allocator->stats.route_miss = 0;
  allocator->stats.transfer_push = 0;
  allocator->stats.transfer_pop = 0;
  allocator->stats.source_alloc = 0;
  allocator->stats.source_refill_starvation = 0;
  allocator->stats.source_refill_saturation = 0;
  allocator->stats.source_refill_boost = 0;
  allocator->stats.source_refill_clamp = 0;
  allocator->stats.source_prefill_attempt = 0;
  allocator->stats.source_prefill_filled = 0;
  allocator->stats.source_prefill_fallback = 0;
  allocator->stats.front_source_ops_alloc = 0;
  allocator->stats.front_source_slot_alloc = 0;
  allocator->stats.front_source_prefill_alloc = 0;
  allocator->stats.toy_source_prefill_call = 0;
  allocator->stats.local2p_source_alloc = 0;
  allocator->stats.midpage_source_alloc = 0;
  allocator->stats.large_source_alloc = 0;
  allocator->stats.toy_source_alloc = 0;
  allocator->stats.alloc_fail = 0;
  allocator->stats.descriptor_exhausted = 0;
  allocator->stats.route_register_fail = 0;
  allocator->stats.source_block_exhausted = 0;
  allocator->stats.route_active_current = 0;
  allocator->stats.route_active_max = 0;
  allocator->stats.descriptor_probe_total = 0;
  allocator->stats.descriptor_probe_max = 0;
  allocator->stats.route_register_probe_total = 0;
  allocator->stats.route_register_probe_max = 0;
  allocator->stats.route_unregister_probe_total = 0;
  allocator->stats.route_unregister_probe_max = 0;
  allocator->stats.source_block_probe_total = 0;
  allocator->stats.source_block_probe_max = 0;
  allocator->stats.large_span_central_push = 0;
  allocator->stats.large_span_central_pop = 0;
  allocator->stats.large_span_source_alloc = 0;
  hz6_source_registry_init(&allocator->source_registry);
}
