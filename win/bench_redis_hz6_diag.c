#include "bench_redis_hz6_diag.h"

#include <stdio.h>
#include <string.h>

#if defined(HZ_BENCH_USE_HZ6) && HZ6_DIAGNOSTIC_PROBES
void print_hz6_redis_stats(const char* pattern,
                           const void* rows,
                           size_t row_stride,
                           size_t hz6_stats_offset,
                           uint32_t threads) {
    Hz6StatsSnapshot total;
    const unsigned char* base;
    uint32_t i;

    memset(&total, 0, sizeof(total));
    base = (const unsigned char*)rows;

    for (i = 0; i < threads; ++i) {
        const Hz6StatsSnapshot* s =
            (const Hz6StatsSnapshot*)(base + ((size_t)i * row_stride) +
                                      hz6_stats_offset);
        total.source_alloc += s->source_alloc;
        total.alloc_fail += s->alloc_fail;
        total.control_plane_normal += s->control_plane_normal;
        total.control_plane_burst_supply_would_open +=
            s->control_plane_burst_supply_would_open;
        total.control_plane_close_would_start +=
            s->control_plane_close_would_start;
        total.descriptor_exhausted += s->descriptor_exhausted;
        total.route_register_fail += s->route_register_fail;
        total.source_block_exhausted += s->source_block_exhausted;
        total.source_prefill_attempt += s->source_prefill_attempt;
        total.source_prefill_filled += s->source_prefill_filled;
        total.source_prefill_fallback += s->source_prefill_fallback;
        total.source_refill_starvation += s->source_refill_starvation;
        total.source_refill_saturation += s->source_refill_saturation;
        total.source_refill_boost += s->source_refill_boost;
        total.source_refill_clamp += s->source_refill_clamp;
        total.source_admission_open += s->source_admission_open;
        total.source_admission_boosted += s->source_admission_boosted;
        total.source_admission_clamped += s->source_admission_clamped;
        total.route_lookup_probe_total += s->route_lookup_probe_total;
        if (s->route_lookup_probe_max > total.route_lookup_probe_max) {
            total.route_lookup_probe_max = s->route_lookup_probe_max;
        }
        total.route_register_probe_total += s->route_register_probe_total;
        if (s->route_register_probe_max > total.route_register_probe_max) {
            total.route_register_probe_max = s->route_register_probe_max;
        }
        total.route_register_reason_source_run_slot +=
            s->route_register_reason_source_run_slot;
        total.route_register_reason_direct_source +=
            s->route_register_reason_direct_source;
        total.route_register_reason_materialize +=
            s->route_register_reason_materialize;
        total.route_register_reason_rehome += s->route_register_reason_rehome;
        total.route_register_reason_unknown += s->route_register_reason_unknown;
        total.route_unregister_reason_frontcache_overflow +=
            s->route_unregister_reason_frontcache_overflow;
        total.route_unregister_reason_cap_release +=
            s->route_unregister_reason_cap_release;
        total.route_unregister_reason_descriptorless_detach +=
            s->route_unregister_reason_descriptorless_detach;
        total.route_unregister_reason_source_slot_release +=
            s->route_unregister_reason_source_slot_release;
        total.route_unregister_reason_rehome += s->route_unregister_reason_rehome;
        total.route_unregister_reason_unknown += s->route_unregister_reason_unknown;
        total.route_register_used_tombstone += s->route_register_used_tombstone;
        total.route_register_full_probe_with_tombstone +=
            s->route_register_full_probe_with_tombstone;
        total.route_tombstone_current += s->route_tombstone_current;
        if (s->route_tombstone_max > total.route_tombstone_max) {
            total.route_tombstone_max = s->route_tombstone_max;
        }
        total.route_tombstone_compact_attempt += s->route_tombstone_compact_attempt;
        total.route_tombstone_compact_success += s->route_tombstone_compact_success;
        total.route_tombstone_compact_fail_alloc +=
            s->route_tombstone_compact_fail_alloc;
        total.route_tombstone_compact_moved += s->route_tombstone_compact_moved;
        total.route_tombstone_cond_probe += s->route_tombstone_cond_probe;
        total.route_tombstone_cond_would_compact +=
            s->route_tombstone_cond_would_compact;
        total.route_tombstone_cond_ratio25 += s->route_tombstone_cond_ratio25;
        total.route_tombstone_cond_occupancy75 +=
            s->route_tombstone_cond_occupancy75;
        total.route_tombstone_cond_cooldown_blocked +=
            s->route_tombstone_cond_cooldown_blocked;
        if (s->route_tombstone_cond_highwater >
            total.route_tombstone_cond_highwater) {
            total.route_tombstone_cond_highwater =
                s->route_tombstone_cond_highwater;
        }
        total.route_retained_overflow_attempt += s->route_retained_overflow_attempt;
        total.route_retained_overflow_success += s->route_retained_overflow_success;
        total.route_retained_overflow_fail += s->route_retained_overflow_fail;
        total.source_block_probe_total += s->source_block_probe_total;
        if (s->source_block_probe_max > total.source_block_probe_max) {
            total.source_block_probe_max = s->source_block_probe_max;
        }
        total.descriptor_probe_total += s->descriptor_probe_total;
        if (s->descriptor_probe_max > total.descriptor_probe_max) {
            total.descriptor_probe_max = s->descriptor_probe_max;
        }
    }

    printf("[HZ6_REDIS_STATS] pattern=%s source_alloc=%zu alloc_fail=%zu "
           "control_plane_normal=%zu control_plane_burst_supply_would_open=%zu "
           "control_plane_close_would_start=%zu descriptor_exhausted=%zu "
           "route_register_fail=%zu source_block_exhausted=%zu "
           "source_prefill_attempt=%zu source_prefill_filled=%zu "
           "source_prefill_fallback=%zu source_refill_starvation=%zu "
           "source_refill_saturation=%zu source_refill_boost=%zu "
           "source_refill_clamp=%zu source_admission_open=%zu "
           "source_admission_boosted=%zu source_admission_clamped=%zu "
           "route_lookup_probe_total=%zu route_lookup_probe_max=%zu "
           "route_register_probe_total=%zu route_register_probe_max=%zu "
           "route_register_reason_source_run_slot=%zu "
           "route_register_reason_direct_source=%zu "
           "route_register_reason_materialize=%zu "
           "route_register_reason_rehome=%zu "
           "route_register_reason_unknown=%zu "
           "route_unregister_reason_frontcache_overflow=%zu "
           "route_unregister_reason_cap_release=%zu "
           "route_unregister_reason_descriptorless_detach=%zu "
           "route_unregister_reason_source_slot_release=%zu "
           "route_unregister_reason_rehome=%zu "
           "route_unregister_reason_unknown=%zu "
           "route_register_used_tombstone=%zu "
           "route_register_full_probe_with_tombstone=%zu "
           "route_tombstone_current=%zu route_tombstone_max=%zu "
           "route_tombstone_compact_attempt=%zu "
           "route_tombstone_compact_success=%zu "
           "route_tombstone_compact_fail_alloc=%zu "
           "route_tombstone_compact_moved=%zu "
           "route_tombstone_cond_probe=%zu "
           "route_tombstone_cond_would_compact=%zu "
           "route_tombstone_cond_ratio25=%zu "
           "route_tombstone_cond_occupancy75=%zu "
           "route_tombstone_cond_cooldown_blocked=%zu "
           "route_tombstone_cond_highwater=%zu "
           "route_retained_overflow_attempt=%zu "
           "route_retained_overflow_success=%zu "
           "route_retained_overflow_fail=%zu "
           "source_block_probe_total=%zu source_block_probe_max=%zu "
           "descriptor_probe_total=%zu descriptor_probe_max=%zu\n",
           pattern,
           total.source_alloc,
           total.alloc_fail,
           total.control_plane_normal,
           total.control_plane_burst_supply_would_open,
           total.control_plane_close_would_start,
           total.descriptor_exhausted,
           total.route_register_fail,
           total.source_block_exhausted,
           total.source_prefill_attempt,
           total.source_prefill_filled,
           total.source_prefill_fallback,
           total.source_refill_starvation,
           total.source_refill_saturation,
           total.source_refill_boost,
           total.source_refill_clamp,
           total.source_admission_open,
           total.source_admission_boosted,
           total.source_admission_clamped,
           total.route_lookup_probe_total,
           total.route_lookup_probe_max,
           total.route_register_probe_total,
           total.route_register_probe_max,
           total.route_register_reason_source_run_slot,
           total.route_register_reason_direct_source,
           total.route_register_reason_materialize,
           total.route_register_reason_rehome,
           total.route_register_reason_unknown,
           total.route_unregister_reason_frontcache_overflow,
           total.route_unregister_reason_cap_release,
           total.route_unregister_reason_descriptorless_detach,
           total.route_unregister_reason_source_slot_release,
           total.route_unregister_reason_rehome,
           total.route_unregister_reason_unknown,
           total.route_register_used_tombstone,
           total.route_register_full_probe_with_tombstone,
           total.route_tombstone_current,
           total.route_tombstone_max,
           total.route_tombstone_compact_attempt,
           total.route_tombstone_compact_success,
           total.route_tombstone_compact_fail_alloc,
           total.route_tombstone_compact_moved,
           total.route_tombstone_cond_probe,
           total.route_tombstone_cond_would_compact,
           total.route_tombstone_cond_ratio25,
           total.route_tombstone_cond_occupancy75,
           total.route_tombstone_cond_cooldown_blocked,
           total.route_tombstone_cond_highwater,
           total.route_retained_overflow_attempt,
           total.route_retained_overflow_success,
           total.route_retained_overflow_fail,
           total.source_block_probe_total,
           total.source_block_probe_max,
           total.descriptor_probe_total,
           total.descriptor_probe_max);
}
#endif
