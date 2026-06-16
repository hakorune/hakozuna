function Get-Hz6WinBroadCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)1024)"
    )
}

function Get-Hz6WinControlCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)512)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)512)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinRoute4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)512)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1"
    $flags
}

function Get-Hz6WinLocalExactFreeNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_EXACT_FIRST_FREE_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalFreeNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags
}

function Get-Hz6WinDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalFreeDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalAllocDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalReuseDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_REUSE_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalFreeAllocDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalFreeReuseDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_REUSE_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinSameOwnerFastDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_SAME_OWNER_FAST_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_AVAIL_COUNT_L1=1"
    $flags
}

function Get-Hz6WinSameOwnerTrustedFreeDescAvailNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSameOwnerFastDescAvailNoBoostRoute4kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_TRUSTED_OWNER_L1=1"
    $flags += "/DHZ6_SAME_OWNER_TRUSTED_LOCAL_FREE_L1=1"
    $flags
}

function Get-Hz6WinSameOwnerFastLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SAME_OWNER_FAST_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalSmall8kSameOwnerLargeLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_REUSE_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4"
    $flags += "/DHZ6_SAME_OWNER_FAST_L1=1"
    $flags += "/DHZ6_SAME_OWNER_FAST_MIN_CLASS=5"
    $flags
}

function Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith {
    param([string[]]$ExtraFlags = @())

    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_REUSE_L1=1"
    foreach ($flag in $ExtraFlags) {
        $flags += $flag
    }
    $flags
}

function Get-Hz6WinDirectLocalFreeLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalAllocLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalFreeAllocLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_ALLOC_L1=1"
    $flags
}

function Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith
}

function Get-Hz6WinToySmallHotPathDiagDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1"
    )
}

function Get-Hz6WinToySmallActiveMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1=1",
        "/DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1"
    )
}

function Get-Hz6WinToySmallActiveMapSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1=1"
    $flags += "/DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1"
    $flags
}

function Get-Hz6WinToySmallHotPathDiagSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1"
    $flags
}

function Get-Hz6WinSourceBlockRouteBehaviorDynMapNoToyDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1=0"
    $flags
}

function Get-Hz6WinSmallRunRouteDryRunDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SMALL_RUN_ROUTE_DRYRUN_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1=1"
    $flags
}

function Get-Hz6WinSmallRunRouteBehaviorDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SMALL_RUN_ROUTE_BEHAVIOR_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1=1"
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1=1"
    $flags
}

function Get-Hz6WinSmallRunRouteBehaviorMin512DirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSmallRunRouteBehaviorDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES=512"
    $flags
}

function Get-Hz6WinSmallRunRouteBehaviorRange64kDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSmallRunRouteBehaviorDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY=65536"
    $flags
}

function Get-Hz6WinSmallRunRouteBehaviorRange64kToyOnlyDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSmallRunRouteBehaviorRange64kDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1=1"
    $flags += "/DHZ6_SMALL_RUN_ROUTE_TOY_RANGE_ONLY_L1=1"
    $flags
}

function Get-Hz6WinSmallRunRouteBehaviorRange64kToyArmedDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSmallRunRouteBehaviorRange64kToyOnlyDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SMALL_RUN_ROUTE_ARMED_L1=1"
    $flags
}

function Get-Hz6WinSmallRunRouteBehaviorRange64kToyArmedSlotMax1kDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSmallRunRouteBehaviorRange64kToyArmedDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES=1024"
    $flags
}

function Get-Hz6WinDirectLocalTrustedLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_LOCAL_CACHE_TRUSTED_OWNER_L1=1"
    )
}

function Get-Hz6WinDirectLocalPackedLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1"
    )
}

function Get-Hz6WinDirectLocalExactLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_LOCAL_EXACT_FIRST_FREE_L1=1"
    )
}

function Get-Hz6WinSourceBlockRouteDryRunDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1=1"
    )
}

function Get-Hz6WinSourceBlockRouteSlotMapDryRunDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1=1"
    )
}

function Get-Hz6WinSourceBlockRouteRangeIndexSlotMapDryRunDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1=1"
    )
}

function Get-Hz6WinSourceBlockRouteBehaviorDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1=1"
    )
}

function Get-Hz6WinSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1=1"
    )
}

function Get-Hz6WinSourceBlockRouteBehaviorDynMapSmall8kDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinSourceBlockRouteBehaviorDynMapDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags + @(
        "/DHZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1=1",
        "/DHZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS=4"
    )
}

function Get-Hz6WinDirectLocalFreeReuseSmall8kLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlagsWith @(
        "/DHZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4"
    )
}

function Get-Hz6WinSpillRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_FRONTCACHE_SPILL_ON_DESCRIPTOR_EXHAUSTION=1"
    $flags
}

function Get-Hz6WinBorrowRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_FRONTCACHE_BORROW_LARGER_ON_CLASS_MISS=1"
    $flags
}

function Get-Hz6WinCapRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_FRONTCACHE_CAP_ON_FREE=1"
    $flags
}

function Get-Hz6WinSourceRunRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    $flags
}

function Get-Hz6WinSourceRunReclaimRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceRunRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1=1"
    $flags
}

function Get-Hz6WinSourceRunSameClassReclaimRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceRunRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1=1"
    $flags
}

function Get-Hz6WinDescriptorlessRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceRunRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTORLESS_FRONTCACHE_L1=1"
    $flags
}

function Get-Hz6WinDescriptorReserveRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1=1"
    $flags
}

function Get-Hz6WinDescriptorColdRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1=1"
    $flags
}

function Get-Hz6WinDescriptorColdGovRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_COLD_GOV_L1=1"
    $flags
}

function Get-Hz6WinDescriptorColdGovWideWsRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_COLD_GOV_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_COLD_GOV_DETACH_BUDGET=((size_t)512)"
    $flags
}

function Get-Hz6WinDesc4kRoute4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinSource512Route4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)512)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinDesc4kSource512Route4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinRedisLowRssRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDesc4kSource512Route4kCapacityFlags
    $flags += "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinLargerLowRssSourceRunDesc8kRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kTombCompactCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_ROUTE_TOMBSTONE_COMPACT_L1=1"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kTombCompactAggressive1024CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kTombCompactCapacityFlags
    $flags += "/DHZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1=1"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_COMPACT_MIN=((size_t)1024)"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kTombCompactAggressive2048CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kTombCompactCapacityFlags
    $flags += "/DHZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1=1"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_COMPACT_MIN=((size_t)2048)"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kConditionalTombDryRunCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1=1"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_CONDITIONAL_ABS_MIN=((size_t)1024)"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_CONDITIONAL_COOLDOWN=((size_t)1024)"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kConditionalTombCompactCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_ROUTE_TOMBSTONE_COMPACT_L1=1"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1=1"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_CONDITIONAL_ABS_MIN=((size_t)1024)"
    $flags += "/DHZ6_ROUTE_TOMBSTONE_CONDITIONAL_COOLDOWN=((size_t)1024)"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kRetainedOverflowCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_ROUTE_RETAINED_OVERFLOW_L1=1"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kSlotLookupCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_SLOT_LOOKUP_L1=1"
    $flags
}

function Get-Hz6WinLargerLowRssSourceRunDesc8kRoute64kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)65536)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinLargerLowRssSourceRunDesc8kSource2kRoute64kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)65536)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinLargeDirectRetainLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LARGE_DIRECT_RETAIN_L1=1"
    $flags
}

function Get-Hz6WinLargeDirectRetain32mLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargeDirectRetainLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LARGE_DIRECT_RETAIN_BYTES_CAP=((size_t)32u * 1024u * 1024u)"
    $flags
}

function Get-Hz6WinLargeDirectRetain16mLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinLargeDirectRetainLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $flags += "/DHZ6_LARGE_DIRECT_RETAIN_BYTES_CAP=((size_t)16u * 1024u * 1024u)"
    $flags
}

function Get-Hz6WinLargerLowRssFront6kSourceRunDesc8kRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)6144)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinLargerLowRssFront4kSourceRunDesc8kRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront8kSourceRunDesc16kSource2kRoute16kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)16384)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)1024)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc16kSource2kRoute16kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)16384)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2048)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc16kTransfer2304Source2kRoute16kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)16384)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2304)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc16kTransfer2560Source2kRoute16kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)16384)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2560)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc24kSource2kRoute24kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)24576)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)24576)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)3072)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc22kSource2kRoute22kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)22528)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)22528)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2816)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc20kSource2kRoute20kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)20480)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)20480)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2560)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc19kSource2kRoute19kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)19456)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)19456)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2432)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc18kSource2kRoute18kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)18432)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)18432)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2304)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)17408)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kCapacityFlags
    $flags += "/DHZ6_ROUTE_LINEAR_WRAP_L1=1"
    $flags
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapLoopCarryCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapCapacityFlags
    $flags += "/DHZ6_ROUTE_LOOP_CARRY_L1=1"
    $flags
}
. (Join-Path $PSScriptRoot "bench_app_like_allocator_build_hz6_capacity_flags_core_tail.ps1")
