#if HZ6_DIAGNOSTIC_PROBES
        {
            const Hz6StatsSnapshot* local_stats = &tds[t].hz6_stats_after;
            const size_t local_descriptor_used =
                hz6_larson_descriptor_used_from_stats(local_stats);
            const size_t local_descriptor_peak =
                (local_stats->descriptor_live_max > local_descriptor_used)
                    ? local_stats->descriptor_live_max
                    : local_descriptor_used;
            const size_t local_route_used = local_stats->route_active_current;
            const size_t local_route_occupied =
                local_stats->route_active_current +
                local_stats->route_tombstone_current;
            const size_t local_route_peak =
                (local_stats->route_active_max > local_route_occupied)
                    ? local_stats->route_active_max
                    : local_route_occupied;
            const size_t local_source_block_used =
                local_stats->memory_active_source_blocks;
            const size_t local_source_block_peak =
                (local_stats->source_block_active_max > local_source_block_used)
                    ? local_stats->source_block_active_max
                    : local_source_block_used;
            const size_t local_frontcache_used =
                local_stats->memory_frontcache_total;
            const size_t local_frontcache_peak =
                (local_stats->frontcache_total_max > local_frontcache_used)
                    ? local_stats->frontcache_total_max
                    : local_frontcache_used;
            const size_t local_transfer_used = local_stats->transfer_current_max;
            if (local_descriptor_peak > hz6_descriptor_used_max_allocator) {
                hz6_descriptor_used_max_allocator = local_descriptor_peak;
            }
            if (local_route_used > hz6_route_used_max_allocator) {
                hz6_route_used_max_allocator = local_route_used;
            }
            if (local_route_peak > hz6_route_occupied_max_allocator) {
                hz6_route_occupied_max_allocator = local_route_peak;
            }
            if (local_source_block_peak > hz6_source_block_used_max_allocator) {
                hz6_source_block_used_max_allocator = local_source_block_peak;
            }
            if (local_frontcache_peak > hz6_frontcache_used_max_allocator) {
                hz6_frontcache_used_max_allocator = local_frontcache_peak;
            }
            if (local_transfer_used > hz6_transfer_used_max_allocator) {
                hz6_transfer_used_max_allocator = local_transfer_used;
            }
        }
        if (tds[t].hz6_stats_after.metadata_descriptor_entry_bytes >
            hz6_stats.metadata_descriptor_entry_bytes) {
            hz6_stats.metadata_descriptor_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_descriptor_thin_hot_entry_bytes >
            hz6_stats.metadata_descriptor_thin_hot_entry_bytes) {
            hz6_stats.metadata_descriptor_thin_hot_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_thin_hot_entry_bytes;
        }
        hz6_stats.metadata_descriptor_thin_hot_table_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_thin_hot_table_bytes;
        hz6_stats.metadata_descriptor_thin_hot_savings_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_thin_hot_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_entry_bytes >
            hz6_stats.metadata_descriptor_ownerless_hot_entry_bytes) {
            hz6_stats.metadata_descriptor_ownerless_hot_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_entry_bytes;
        }
        hz6_stats.metadata_descriptor_ownerless_hot_table_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_table_bytes;
        hz6_stats.metadata_descriptor_ownerless_hot_savings_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_ownerless_hot_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_entry_bytes >
            hz6_stats.metadata_descriptor_owner16_hot_entry_bytes) {
            hz6_stats.metadata_descriptor_owner16_hot_entry_bytes =
                tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_entry_bytes;
        }
        hz6_stats.metadata_descriptor_owner16_hot_table_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_table_bytes;
        hz6_stats.metadata_descriptor_owner16_hot_savings_bytes +=
            tds[t].hz6_stats_after.metadata_descriptor_owner16_hot_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_route_entry_bytes >
            hz6_stats.metadata_route_entry_bytes) {
            hz6_stats.metadata_route_entry_bytes =
                tds[t].hz6_stats_after.metadata_route_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_route_slim_entry_bytes >
            hz6_stats.metadata_route_slim_entry_bytes) {
            hz6_stats.metadata_route_slim_entry_bytes =
                tds[t].hz6_stats_after.metadata_route_slim_entry_bytes;
        }
        hz6_stats.metadata_route_slim_table_bytes +=
            tds[t].hz6_stats_after.metadata_route_slim_table_bytes;
        hz6_stats.metadata_route_slim_savings_bytes +=
            tds[t].hz6_stats_after.metadata_route_slim_savings_bytes;
        hz6_stats.metadata_route_bytes16_table_bytes +=
            tds[t].hz6_stats_after.metadata_route_bytes16_table_bytes;
        hz6_stats.metadata_route_bytes16_savings_bytes +=
            tds[t].hz6_stats_after.metadata_route_bytes16_savings_bytes;
        hz6_stats.metadata_route_bytes16_active_checked +=
            tds[t].hz6_stats_after.metadata_route_bytes16_active_checked;
        hz6_stats.metadata_route_bytes16_overflow +=
            tds[t].hz6_stats_after.metadata_route_bytes16_overflow;
        if (tds[t].hz6_stats_after.metadata_route_bytes16_max >
            hz6_stats.metadata_route_bytes16_max) {
            hz6_stats.metadata_route_bytes16_max =
                tds[t].hz6_stats_after.metadata_route_bytes16_max;
        }
        hz6_stats.metadata_route_bytes16_minus1_overflow +=
            tds[t].hz6_stats_after.metadata_route_bytes16_minus1_overflow;
        hz6_stats.metadata_route_bytes16_minus1_zero +=
            tds[t].hz6_stats_after.metadata_route_bytes16_minus1_zero;
        if (tds[t].hz6_stats_after.metadata_route_bytes16_minus1_max_stored >
            hz6_stats.metadata_route_bytes16_minus1_max_stored) {
            hz6_stats.metadata_route_bytes16_minus1_max_stored =
                tds[t].hz6_stats_after
                    .metadata_route_bytes16_minus1_max_stored;
        }
        if (tds[t].hz6_stats_after.metadata_source_block_entry_bytes >
            hz6_stats.metadata_source_block_entry_bytes) {
            hz6_stats.metadata_source_block_entry_bytes =
                tds[t].hz6_stats_after.metadata_source_block_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_source_block_slim_entry_bytes >
            hz6_stats.metadata_source_block_slim_entry_bytes) {
            hz6_stats.metadata_source_block_slim_entry_bytes =
                tds[t].hz6_stats_after.metadata_source_block_slim_entry_bytes;
        }
        hz6_stats.metadata_source_block_slim_table_bytes +=
            tds[t].hz6_stats_after.metadata_source_block_slim_table_bytes;
        hz6_stats.metadata_source_block_slim_savings_bytes +=
            tds[t].hz6_stats_after.metadata_source_block_slim_savings_bytes;
        if (tds[t].hz6_stats_after.metadata_frontcache_entry_bytes >
            hz6_stats.metadata_frontcache_entry_bytes) {
            hz6_stats.metadata_frontcache_entry_bytes =
                tds[t].hz6_stats_after.metadata_frontcache_entry_bytes;
        }
        if (tds[t].hz6_stats_after.metadata_frontcache_slim_entry_bytes >
            hz6_stats.metadata_frontcache_slim_entry_bytes) {
            hz6_stats.metadata_frontcache_slim_entry_bytes =
                tds[t].hz6_stats_after.metadata_frontcache_slim_entry_bytes;
        }
        hz6_stats.metadata_frontcache_slim_table_bytes +=
            tds[t].hz6_stats_after.metadata_frontcache_slim_table_bytes;
        hz6_stats.metadata_frontcache_slim_savings_bytes +=
            tds[t].hz6_stats_after.metadata_frontcache_slim_savings_bytes;
        hz6_stats.large_span_central_push +=
            tds[t].hz6_stats_after.large_span_central_push;
        hz6_stats.large_span_central_pop +=
            tds[t].hz6_stats_after.large_span_central_pop;
        hz6_stats.large_span_source_alloc +=
            tds[t].hz6_stats_after.large_span_source_alloc;
#endif
