$ErrorActionPreference = "Stop"

function Get-Hz5WinBenchIncludeFlags {
    param([Parameter(Mandatory = $true)][string]$Hz5Root)

    @(
        "/I$Hz5Root\include",
        "/I$Hz5Root\contract",
        "/I$Hz5Root\core",
        "/I$Hz5Root\fallback",
        "/I$Hz5Root\lowpage",
        "/I$Hz5Root\policy",
        "/I$Hz5Root\route",
        "/I$Hz5Root\smallfront",
        "/I$Hz5Root\wrapper"
    )
}

function Get-Hz5WinBenchLibSources {
    param([Parameter(Mandatory = $true)][string]$Hz5Root)

    @(
        "$Hz5Root\contract\hz5_contract.c",
        "$Hz5Root\core\hz5_segment.c",
        "$Hz5Root\core\hz5_run.c",
        "$Hz5Root\core\hz5_owner.c",
        "$Hz5Root\core\hz5_remote.c",
        "$Hz5Root\core\hz5_tcache.c",
        "$Hz5Root\core\hz5_stats.c",
        "$Hz5Root\route\hz5_route.c",
        "$Hz5Root\wrapper\hz5_wrapper.c",
        "$Hz5Root\policy\hz5_policy.c",
        "$Hz5Root\lowpage\hz5_lowpage64.c",
        "$Hz5Root\lowpage\hz5_lowpage64_os.c",
        "$Hz5Root\lowpage\hz5_lowpage64_p43_segment.c"
    )
}

function Get-Hz5WinBenchFlags {
    @(
        "/DHZ_BENCH_USE_HZ5_POLICY",
        "/DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN=1"
    )
}

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

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)18432)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kDoubleHashCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kCapacityFlags
    $flags += "/DHZ6_ROUTE_DOUBLE_HASH_L1=1"
    $flags
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kHashXorCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kCapacityFlags
    $flags += "/DHZ6_ROUTE_HASH_XOR_FOLD_L1=1"
    $flags
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kLinearWrapCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kCapacityFlags
    $flags += "/DHZ6_ROUTE_LINEAR_WRAP_L1=1"
    $flags
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute20kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)20480)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc32kSource4kRoute32kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)32768)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)32768)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)4096)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc32kSource3kRoute32kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)32768)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)32768)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)3072)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront8kSourceRunDesc32kSource4kRoute32kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)32768)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)32768)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2048)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)4096)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront12kSourceRunDesc32kSource4kRoute32kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)32768)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)32768)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)3072)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)4096)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)12288)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc32kSource2kRoute32kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)32768)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)32768)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinRedisLowRssSlimRoute4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)2048)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)256)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1"
    )
}

function Get-Hz6WinFront1kDesc4kSource512Route4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)1024)"
    )
}

function Get-Hz6WinAppLikeCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)262144)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)32768)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)65536)"
    )
}

function Get-Hz6WinVisibleFirstAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_VISIBLE_FIRST_FREE_L1=1"
    $flags
}

function Get-Hz6WinNegativeFilterAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_NEGATIVE_FILTER_L1=1"
    $flags
}

function Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1"
    $flags
}

function Get-Hz6WinSharedRouteDirectoryFirstAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1"
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_FIRST_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags
    $flags += "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    $flags += "/DHZ6_DIAGNOSTIC_PROBES=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags
    $flags += "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap1CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)32768)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc192kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kRoute128kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)131072)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kSource2kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kCapacityFlags
    $flags += "/DHZ6_THIN_DESCRIPTOR_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)131072)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute224kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)229376)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRunSlotsCapacityFlags {
    param(
        [Parameter(Mandatory = $true)][int]$RunSlots
    )

    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kCapacityFlags
    $flags += ("/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t){0})" -f $RunSlots)
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun2048CapacityFlags {
    Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRunSlotsCapacityFlags -RunSlots 2048
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun1024CapacityFlags {
    Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRunSlotsCapacityFlags -RunSlots 1024
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRunSlotsCapacityFlags -RunSlots 512
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir192kSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_ROUTE_PACKED_META_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16Source16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16OwnerSourceDryRunSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16Source16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16Source16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1"
    $flags += "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_FRONTCACHE_PACKED_META_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2SourceBlockPackedSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource2kRoute192kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource8kRoute192kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource12kRoute192kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)12288)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute192kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)10240)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticProjectionLocal1kRoute16kSource64Front1kPackedCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)1024)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)64)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)1024)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)1024)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticProjectionLive2kRoute16kSource128Front1kPackedCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)2048)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)1024)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)2048)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticRouteOverflowDesc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)10240)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_ELASTIC_ROUTE_OVERFLOW_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)16384)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)10240)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_ELASTIC_ROUTE_OVERFLOW_L1=1",
        "/DHZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1=1",
        "/DHZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY=((size_t)196608)",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)16384)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)64)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_ELASTIC_ROUTE_OVERFLOW_L1=1",
        "/DHZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1=1",
        "/DHZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1=1",
        "/DHZ6_ELASTIC_DESCRIPTOR_DEPOT_CAPACITY=((size_t)196608)",
        "/DHZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY=((size_t)16384)",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_DESCRIPTOR_NO_BACKPTR_L1=1",
        "/DHZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1=1",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)196608)",
        "/DHZ6_ROUTE_PACKED_META_L1=1",
        "/DHZ6_ROUTE_BYTES16_MINUS1_L2=1",
        "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1",
        "/DHZ6_OWNER_SOURCE_SIDE_META_L2=1",
        "/DHZ6_FRONTCACHE_PACKED_META_L1=1",
        "/DHZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowLocalizeDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $flags += "/DHZ6_ELASTIC_SOURCE_BLOCK_LOCALIZE_DRYRUN_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowRunLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $flags += "/DHZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotRunMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $flags = @($flags | Where-Object { $_ -notmatch '^/DHZ6_SOURCE_RUN_MAX_SLOTS=' })
    $flags += "/DHZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1=1"
    $flags += "/DHZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1=1"
    $flags += "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)4096)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotRunMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags
    $flags += "/DHZ6_ELASTIC_SLOT_OWNER_LOCALITY_DRYRUN_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerSparseMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags
    $flags += "/DHZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotOwnerDirectDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_DEPOT_OWNER_DIRECT_FASTPATH_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2DryRunSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrStorageOwner16NoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_STORAGE_OWNER16_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir128kSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)131072)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir96kSource16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_CAPACITY=((size_t)98304)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSideOwner16Source16kRoute192kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_SIDE_OWNER16_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc144kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)147456)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc152kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)155648)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc156kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)159744)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc158kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)161792)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc148kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)151552)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc128kFront4kThinDescSource16kRoute192kRun512CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)131072)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)196608)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)163840)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kRun512CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_MAX_SLOTS=((size_t)512)"
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute96kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)98304)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute64kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)65536)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)16384)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource12kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)12288)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource14kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)14336)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource32kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)163840)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)32768)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)4096)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1",
        "/DHZ6_THIN_DESCRIPTOR_L1=1",
        "/DHZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY=((size_t)4096)"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap2Desc144kCapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)147456)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap3CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)131072)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastRssCap4CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)131072)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)131072)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)8192)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinDirectLocalFreeOwnerLocalityFastRssCap4CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastRssCap4CapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastWideCap1CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)32768)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)32768)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)32768)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastWideCap2CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)32768)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastWideCap3CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinOwnerLocalityFastWideCap4CapacityFlags {
    $flags = @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)131072)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)8192)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1",
        "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    )
    $flags
}

function Get-Hz6WinDirectLocalFreeOwnerLocalityFastWideCap4CapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinOwnerLocalityFastWideCap4CapacityFlags
    $flags += "/DHZ6_LOCAL_CACHE_DIRECT_FREE_L1=1"
    $flags
}

function Invoke-AppLikeHz5BenchBuild {
    param(
        [Parameter(Mandatory = $true)][string]$Compiler,
        [Parameter(Mandatory = $true)][string[]]$BaseFlags,
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$BenchSrc,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $hz5Root = Join-Path $RepoRoot "hakozuna-hz5"
    if (-not (Test-Path $hz5Root)) {
        Write-Warning "HZ5 root not found; skipping $(Split-Path -Leaf $OutputPath): $hz5Root"
        return
    }

    $args = @()
    $args += $BaseFlags
    $args += (Get-Hz5WinBenchFlags)
    $args += (Get-Hz5WinBenchIncludeFlags -Hz5Root $hz5Root)
    $args += (Get-Hz5WinBenchLibSources -Hz5Root $hz5Root)
    $args += $BenchSrc
    $args += "/Fe:$OutputPath"
    Write-Host "[hz5-win] building $(Split-Path -Leaf $OutputPath)"
    & $Compiler @args
    if ($LASTEXITCODE -ne 0) {
        throw "HZ5 app-like bench build failed for $(Split-Path -Leaf $OutputPath) with exit code $LASTEXITCODE"
    }
}

function Invoke-AppLikeHz6BenchBuilds {
    param(
        [Parameter(Mandatory = $true)][string]$Compiler,
        [Parameter(Mandatory = $true)][string[]]$BaseFlags,
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$BenchSrc,
        [Parameter(Mandatory = $true)][string]$OutDir,
        [Parameter(Mandatory = $true)][string]$OutputPrefix,
        [string[]]$Hz6Profiles,
        [string[]]$CapacityLanes,
        [switch]$DiagnosticHz6Probes,
        [switch]$IncludeControlCapacity
    )

    $hz6Root = Join-Path $RepoRoot "hakozuna-hz6"
    $hz6Common = Join-Path $hz6Root "win\hz6_win_build_common.ps1"
    if (-not (Test-Path $hz6Common)) {
        Write-Warning "HZ6 build helper not found; skipping $OutputPrefix HZ6 rows: $hz6Common"
        return
    }

    . $hz6Common
    $includeFlags = Get-Hz6WinIncludeFlags -Hz6Root $hz6Root -ExtraIncludeRoots @("win")
    $libSources = Get-Hz6WinLibSources -Hz6Root $hz6Root
    $commonFlags = Get-Hz6WinClangCommonFlags
    $profileMap = @{
        "strict" = @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" }
        "speed" = @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" }
        "rss" = @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" }
    }
    $broadFlags = Get-Hz6WinBroadCapacityFlags
    $controlFlags = Get-Hz6WinControlCapacityFlags
    $route4kFlags = Get-Hz6WinRoute4kCapacityFlags
    $noBoostRoute4kFlags = Get-Hz6WinNoBoostRoute4kCapacityFlags
    $localExactFreeNoBoostRoute4kFlags = Get-Hz6WinLocalExactFreeNoBoostRoute4kCapacityFlags
    $directLocalFreeNoBoostRoute4kFlags = Get-Hz6WinDirectLocalFreeNoBoostRoute4kCapacityFlags
    $descAvailNoBoostRoute4kFlags = Get-Hz6WinDescAvailNoBoostRoute4kCapacityFlags
    $directLocalFreeDescAvailNoBoostRoute4kFlags = Get-Hz6WinDirectLocalFreeDescAvailNoBoostRoute4kCapacityFlags
    $directLocalAllocDescAvailNoBoostRoute4kFlags = Get-Hz6WinDirectLocalAllocDescAvailNoBoostRoute4kCapacityFlags
    $directLocalReuseDescAvailNoBoostRoute4kFlags = Get-Hz6WinDirectLocalReuseDescAvailNoBoostRoute4kCapacityFlags
    $directLocalFreeAllocDescAvailNoBoostRoute4kFlags = Get-Hz6WinDirectLocalFreeAllocDescAvailNoBoostRoute4kCapacityFlags
    $directLocalFreeReuseDescAvailNoBoostRoute4kFlags = Get-Hz6WinDirectLocalFreeReuseDescAvailNoBoostRoute4kCapacityFlags
    $sameOwnerFastDescAvailNoBoostRoute4kFlags = Get-Hz6WinSameOwnerFastDescAvailNoBoostRoute4kCapacityFlags
    $spillRoute4kFlags = Get-Hz6WinSpillRoute4kCapacityFlags
    $borrowRoute4kFlags = Get-Hz6WinBorrowRoute4kCapacityFlags
    $capRoute4kFlags = Get-Hz6WinCapRoute4kCapacityFlags
    $sourceRunRoute4kFlags = Get-Hz6WinSourceRunRoute4kCapacityFlags
    $sourceRunReclaimRoute4kFlags = Get-Hz6WinSourceRunReclaimRoute4kCapacityFlags
    $sourceRunSameClassReclaimRoute4kFlags = Get-Hz6WinSourceRunSameClassReclaimRoute4kCapacityFlags
    $descriptorlessRoute4kFlags = Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $descriptorReserveRoute4kFlags = Get-Hz6WinDescriptorReserveRoute4kCapacityFlags
    $descriptorColdRoute4kFlags = Get-Hz6WinDescriptorColdRoute4kCapacityFlags
    $descriptorColdGovRoute4kFlags = Get-Hz6WinDescriptorColdGovRoute4kCapacityFlags
    $descriptorColdGovWideWsRoute4kFlags = Get-Hz6WinDescriptorColdGovWideWsRoute4kCapacityFlags
    $desc4kRoute4kFlags = Get-Hz6WinDesc4kRoute4kCapacityFlags
    $source512Route4kFlags = Get-Hz6WinSource512Route4kCapacityFlags
    $desc4kSource512Route4kFlags = Get-Hz6WinDesc4kSource512Route4kCapacityFlags
    $redisLowRssRoute4kFlags = Get-Hz6WinRedisLowRssRoute4kCapacityFlags
    $redisLowRssSourceRunRoute4kFlags = Get-Hz6WinRedisLowRssSourceRunRoute4kCapacityFlags
    $redisLowRssSourceRunRoute8kFlags = Get-Hz6WinRedisLowRssSourceRunRoute8kCapacityFlags
    $redisLowRssSourceRunDesc8kRoute8kFlags = Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $largerLowRssSourceRunDesc8kRoute8kFlags = Get-Hz6WinLargerLowRssSourceRunDesc8kRoute8kCapacityFlags
    $redisLowRssSourceRunDesc8kRoute8kTombCompactFlags = Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kTombCompactCapacityFlags
    $redisLowRssSourceRunDesc8kRoute8kRetainedOverflowFlags = Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kRetainedOverflowCapacityFlags
    $redisLowRssSourceRunDesc8kRoute8kSlotLookupFlags = Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kSlotLookupCapacityFlags
    $largerLowRssSourceRunDesc8kRoute64kFlags = Get-Hz6WinLargerLowRssSourceRunDesc8kRoute64kCapacityFlags
    $largerLowRssSourceRunDesc8kSource2kRoute64kFlags = Get-Hz6WinLargerLowRssSourceRunDesc8kSource2kRoute64kCapacityFlags
    $largerLowRssFront8kSourceRunDesc8kRoute8kFlags = Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $largerLowRssFront6kSourceRunDesc8kRoute8kFlags = Get-Hz6WinLargerLowRssFront6kSourceRunDesc8kRoute8kCapacityFlags
    $largerLowRssFront4kSourceRunDesc8kRoute8kFlags = Get-Hz6WinLargerLowRssFront4kSourceRunDesc8kRoute8kCapacityFlags
    $mixedCleanFront8kSourceRunDesc16kSource2kRoute16kFlags = Get-Hz6WinMixedCleanFront8kSourceRunDesc16kSource2kRoute16kCapacityFlags
    $mixedCleanFront16kSourceRunDesc16kSource2kRoute16kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc16kSource2kRoute16kCapacityFlags
    $mixedCleanFront16kSourceRunDesc16kTransfer2304Source2kRoute16kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc16kTransfer2304Source2kRoute16kCapacityFlags
    $mixedCleanFront16kSourceRunDesc16kTransfer2560Source2kRoute16kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc16kTransfer2560Source2kRoute16kCapacityFlags
    $mixedCleanFront16kSourceRunDesc24kSource2kRoute24kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc24kSource2kRoute24kCapacityFlags
    $mixedCleanFront16kSourceRunDesc22kSource2kRoute22kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc22kSource2kRoute22kCapacityFlags
    $mixedCleanFront16kSourceRunDesc20kSource2kRoute20kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc20kSource2kRoute20kCapacityFlags
    $mixedCleanFront16kSourceRunDesc19kSource2kRoute19kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc19kSource2kRoute19kCapacityFlags
    $mixedCleanFront16kSourceRunDesc18kSource2kRoute18kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc18kSource2kRoute18kCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute17kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapLoopCarryFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapLoopCarryCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kDoubleHashFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kDoubleHashCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kHashXorFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kHashXorCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kLinearWrapFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute18kLinearWrapCapacityFlags
    $mixedCleanFront16kSourceRunDesc17kSource2kRoute20kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute20kCapacityFlags
    $mixedCleanFront16kSourceRunDesc32kSource4kRoute32kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc32kSource4kRoute32kCapacityFlags
    $mixedCleanFront16kSourceRunDesc32kSource3kRoute32kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc32kSource3kRoute32kCapacityFlags
    $mixedCleanFront8kSourceRunDesc32kSource4kRoute32kFlags = Get-Hz6WinMixedCleanFront8kSourceRunDesc32kSource4kRoute32kCapacityFlags
    $mixedCleanFront12kSourceRunDesc32kSource4kRoute32kFlags = Get-Hz6WinMixedCleanFront12kSourceRunDesc32kSource4kRoute32kCapacityFlags
    $mixedCleanFront16kSourceRunDesc32kSource2kRoute32kFlags = Get-Hz6WinMixedCleanFront16kSourceRunDesc32kSource2kRoute32kCapacityFlags
    $redisLowRssSlimRoute4kFlags = Get-Hz6WinRedisLowRssSlimRoute4kCapacityFlags
    $front1kDesc4kSource512Route4kFlags = Get-Hz6WinFront1kDesc4kSource512Route4kCapacityFlags
    $appLikeFlags = Get-Hz6WinAppLikeCapacityFlags
    $visibleFirstAppLikeFlags = Get-Hz6WinVisibleFirstAppLikeCapacityFlags
    $negativeFilterAppLikeFlags = Get-Hz6WinNegativeFilterAppLikeCapacityFlags
    $sharedRouteDirectoryAppLikeFlags = Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags
    $sharedRouteDirectoryFirstAppLikeFlags = Get-Hz6WinSharedRouteDirectoryFirstAppLikeCapacityFlags
    $ownerLocalityAppLikeFlags = Get-Hz6WinOwnerLocalityAppLikeCapacityFlags
    $ownerLocalityFastAppLikeFlags = Get-Hz6WinOwnerLocalityFastAppLikeCapacityFlags
    $ownerLocalityFastRssCap1Flags = Get-Hz6WinOwnerLocalityFastRssCap1CapacityFlags
    $ownerLocalityFastRssCap2Flags = Get-Hz6WinOwnerLocalityFastRssCap2CapacityFlags
    $ownerLocalityFastRssCap2Desc192kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc192kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kRoute128kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kRoute128kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kSource2kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kSource2kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource12kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource12kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource14kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource14kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute224kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute224kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun2048Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun2048CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun1024Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun1024CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir192kSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir192kSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16Source16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16Source16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16OwnerSourceDryRunSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16OwnerSourceDryRunSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2SourceBlockPackedSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2SourceBlockPackedSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource2kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource2kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource8kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource8kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource12kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource12kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2ElasticProjectionLocal1kRoute16kSource64Front1kPackedFlags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticProjectionLocal1kRoute16kSource64Front1kPackedCapacityFlags
    $ownerLocalityFastRssCap2ElasticProjectionLive2kRoute16kSource128Front1kPackedFlags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticProjectionLive2kRoute16kSource128Front1kPackedCapacityFlags
    $ownerLocalityFastRssCap2ElasticRouteOverflowDesc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticRouteOverflowDesc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowLocalizeDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowLocalizeDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowRunLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowRunLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotRunMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotRunMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerSparseMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerSparseMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096CapacityFlags
    $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotOwnerDirectDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotOwnerDirectDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2DryRunSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2DryRunSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrStorageOwner16NoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrStorageOwner16NoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir128kSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir128kSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir96kSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir96kSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSideOwner16Source16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSideOwner16Source16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc158kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc158kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc156kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc156kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc152kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc152kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc148kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc148kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc144kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc144kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc128kFront4kThinDescSource16kRoute192kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc128kFront4kThinDescSource16kRoute192kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kRun512Flags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kRun512CapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute96kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute96kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute64kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute64kCapacityFlags
    $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource32kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc160kFront4kThinDescSource32kCapacityFlags
    $ownerLocalityFastRssCap2Desc144kFlags = Get-Hz6WinOwnerLocalityFastRssCap2Desc144kCapacityFlags
    $ownerLocalityFastRssCap3Flags = Get-Hz6WinOwnerLocalityFastRssCap3CapacityFlags
    $ownerLocalityFastRssCap4Flags = Get-Hz6WinOwnerLocalityFastRssCap4CapacityFlags
    $directLocalFreeOwnerLocalityFastRssCap4Flags = Get-Hz6WinDirectLocalFreeOwnerLocalityFastRssCap4CapacityFlags
    $ownerLocalityFastWideCap1Flags = Get-Hz6WinOwnerLocalityFastWideCap1CapacityFlags
    $ownerLocalityFastWideCap2Flags = Get-Hz6WinOwnerLocalityFastWideCap2CapacityFlags
    $ownerLocalityFastWideCap3Flags = Get-Hz6WinOwnerLocalityFastWideCap3CapacityFlags
    $ownerLocalityFastWideCap4Flags = Get-Hz6WinOwnerLocalityFastWideCap4CapacityFlags
    $directLocalFreeOwnerLocalityFastWideCap4Flags = Get-Hz6WinDirectLocalFreeOwnerLocalityFastWideCap4CapacityFlags
    $laneMap = @{
        "default" = @{ Suffix = ""; ExtraFlags = @() }
        "broad" = @{ Suffix = "_broad"; ExtraFlags = $broadFlags }
        "control" = @{ Suffix = "_control"; ExtraFlags = $controlFlags }
        "route4k" = @{ Suffix = "_route4k"; ExtraFlags = $route4kFlags }
        "noboost-route4k" = @{ Suffix = "_noboost_route4k"; ExtraFlags = $noBoostRoute4kFlags }
        "localexactfree-noboost-route4k" = @{ Suffix = "_localexactfree_noboost_route4k"; ExtraFlags = $localExactFreeNoBoostRoute4kFlags }
        "directlocalfree-noboost-route4k" = @{ Suffix = "_directlocalfree_noboost_route4k"; ExtraFlags = $directLocalFreeNoBoostRoute4kFlags }
        "descavail-noboost-route4k" = @{ Suffix = "_descavail_noboost_route4k"; ExtraFlags = $descAvailNoBoostRoute4kFlags }
        "directlocalfree-descavail-noboost-route4k" = @{ Suffix = "_directlocalfree_descavail_noboost_route4k"; ExtraFlags = $directLocalFreeDescAvailNoBoostRoute4kFlags }
        "directlocalalloc-descavail-noboost-route4k" = @{ Suffix = "_directlocalalloc_descavail_noboost_route4k"; ExtraFlags = $directLocalAllocDescAvailNoBoostRoute4kFlags }
        "directlocalreuse-descavail-noboost-route4k" = @{ Suffix = "_directlocalreuse_descavail_noboost_route4k"; ExtraFlags = $directLocalReuseDescAvailNoBoostRoute4kFlags }
        "directlocalfreealloc-descavail-noboost-route4k" = @{ Suffix = "_directlocalfreealloc_descavail_noboost_route4k"; ExtraFlags = $directLocalFreeAllocDescAvailNoBoostRoute4kFlags }
        "directlocalfreereuse-descavail-noboost-route4k" = @{ Suffix = "_directlocalfreereuse_descavail_noboost_route4k"; ExtraFlags = $directLocalFreeReuseDescAvailNoBoostRoute4kFlags }
        "sameownerfast-descavail-noboost-route4k" = @{ Suffix = "_sameownerfast_descavail_noboost_route4k"; ExtraFlags = $sameOwnerFastDescAvailNoBoostRoute4kFlags }
        "spill-route4k" = @{ Suffix = "_spill_route4k"; ExtraFlags = $spillRoute4kFlags }
        "borrow-route4k" = @{ Suffix = "_borrow_route4k"; ExtraFlags = $borrowRoute4kFlags }
        "cap-route4k" = @{ Suffix = "_cap_route4k"; ExtraFlags = $capRoute4kFlags }
        "sourcerun-route4k" = @{ Suffix = "_sourcerun_route4k"; ExtraFlags = $sourceRunRoute4kFlags }
        "sourcerun-reclaim-route4k" = @{ Suffix = "_sourcerun_reclaim_route4k"; ExtraFlags = $sourceRunReclaimRoute4kFlags }
        "sourcerun-sameclass-route4k" = @{ Suffix = "_sourcerun_sameclass_route4k"; ExtraFlags = $sourceRunSameClassReclaimRoute4kFlags }
        "descriptorless-route4k" = @{ Suffix = "_descriptorless_route4k"; ExtraFlags = $descriptorlessRoute4kFlags }
        "descriptorreserve-route4k" = @{ Suffix = "_descriptorreserve_route4k"; ExtraFlags = $descriptorReserveRoute4kFlags }
        "descriptorcold-route4k" = @{ Suffix = "_descriptorcold_route4k"; ExtraFlags = $descriptorColdRoute4kFlags }
        "descriptorcoldgov-route4k" = @{ Suffix = "_descriptorcoldgov_route4k"; ExtraFlags = $descriptorColdGovRoute4kFlags }
        "descriptorcoldgov-widews-route4k" = @{ Suffix = "_descriptorcoldgov_widews_route4k"; ExtraFlags = $descriptorColdGovWideWsRoute4kFlags }
        "desc4k-route4k" = @{ Suffix = "_desc4k_route4k"; ExtraFlags = $desc4kRoute4kFlags }
        "source512-route4k" = @{ Suffix = "_source512_route4k"; ExtraFlags = $source512Route4kFlags }
        "desc4k-source512-route4k" = @{ Suffix = "_desc4k_source512_route4k"; ExtraFlags = $desc4kSource512Route4kFlags }
        "redislowrss-route4k" = @{ Suffix = "_redislowrss_route4k"; ExtraFlags = $redisLowRssRoute4kFlags }
        "redislowrss-sourcerun-route4k" = @{ Suffix = "_redislowrss_sourcerun_route4k"; ExtraFlags = $redisLowRssSourceRunRoute4kFlags }
        "redislowrss-sourcerun-route8k" = @{ Suffix = "_redislowrss_sourcerun_route8k"; ExtraFlags = $redisLowRssSourceRunRoute8kFlags }
        "redislowrss-sourcerun-desc8k-route8k" = @{ Suffix = "_redislowrss_sourcerun_desc8k_route8k"; ExtraFlags = $redisLowRssSourceRunDesc8kRoute8kFlags }
        "largerlowrss-sourcerun-desc8k-route8k" = @{ Suffix = "_largerlowrss_sourcerun_desc8k_route8k"; ExtraFlags = $largerLowRssSourceRunDesc8kRoute8kFlags }
        "redislowrss-sourcerun-desc8k-route8k-tombcompact" = @{ Suffix = "_redislowrss_sourcerun_desc8k_route8k_tombcompact"; ExtraFlags = $redisLowRssSourceRunDesc8kRoute8kTombCompactFlags }
        "redislowrss-sourcerun-desc8k-route8k-retainedoverflow" = @{ Suffix = "_redislowrss_sourcerun_desc8k_route8k_retainedoverflow"; ExtraFlags = $redisLowRssSourceRunDesc8kRoute8kRetainedOverflowFlags }
        "redislowrss-sourcerun-desc8k-route8k-slotlookup" = @{ Suffix = "_redislowrss_sourcerun_desc8k_route8k_slotlookup"; ExtraFlags = $redisLowRssSourceRunDesc8kRoute8kSlotLookupFlags }
        "largerlowrss-sourcerun-desc8k-route64k" = @{ Suffix = "_largerlowrss_sourcerun_desc8k_route64k"; ExtraFlags = $largerLowRssSourceRunDesc8kRoute64kFlags }
        "largerlowrss-sourcerun-desc8k-source2k-route64k" = @{ Suffix = "_largerlowrss_sourcerun_desc8k_source2k_route64k"; ExtraFlags = $largerLowRssSourceRunDesc8kSource2kRoute64kFlags }
        "largerlowrss-front8k-sourcerun-desc8k-route8k" = @{ Suffix = "_largerlowrss_front8k_sourcerun_desc8k_route8k"; ExtraFlags = $largerLowRssFront8kSourceRunDesc8kRoute8kFlags }
        "largerlowrss-front6k-sourcerun-desc8k-route8k" = @{ Suffix = "_largerlowrss_front6k_sourcerun_desc8k_route8k"; ExtraFlags = $largerLowRssFront6kSourceRunDesc8kRoute8kFlags }
        "largerlowrss-front4k-sourcerun-desc8k-route8k" = @{ Suffix = "_largerlowrss_front4k_sourcerun_desc8k_route8k"; ExtraFlags = $largerLowRssFront4kSourceRunDesc8kRoute8kFlags }
        "mixedclean-front8k-sourcerun-desc16k-source2k-route16k" = @{ Suffix = "_mixedclean_front8k_sourcerun_desc16k_source2k_route16k"; ExtraFlags = $mixedCleanFront8kSourceRunDesc16kSource2kRoute16kFlags }
        "mixedclean-front16k-sourcerun-desc16k-source2k-route16k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc16k_source2k_route16k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc16kSource2kRoute16kFlags }
        "mixedclean-front16k-sourcerun-desc16k-transfer2304-source2k-route16k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc16k_transfer2304_source2k_route16k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc16kTransfer2304Source2kRoute16kFlags }
        "mixedclean-front16k-sourcerun-desc16k-transfer2560-source2k-route16k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc16k_transfer2560_source2k_route16k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc16kTransfer2560Source2kRoute16kFlags }
        "mixedclean-front16k-sourcerun-desc24k-source2k-route24k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc24k_source2k_route24k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc24kSource2kRoute24kFlags }
        "mixedclean-front16k-sourcerun-desc22k-source2k-route22k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc22k_source2k_route22k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc22kSource2kRoute22kFlags }
        "mixedclean-front16k-sourcerun-desc20k-source2k-route20k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc20k_source2k_route20k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc20kSource2kRoute20kFlags }
        "mixedclean-front16k-sourcerun-desc19k-source2k-route19k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc19k_source2k_route19k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc19kSource2kRoute19kFlags }
        "mixedclean-front16k-sourcerun-desc18k-source2k-route18k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc18k_source2k_route18k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc18kSource2kRoute18kFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route17k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute17kFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k_linearwrap"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k_linearwrap_loopcarry"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute17kLinearWrapLoopCarryFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route18k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-doublehash" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_doublehash"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kDoubleHashFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-hashxor" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_hashxor"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kHashXorFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-linearwrap" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_linearwrap"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute18kLinearWrapFlags }
        "mixedclean-front16k-sourcerun-desc17k-source2k-route20k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc17k_source2k_route20k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc17kSource2kRoute20kFlags }
        "mixedclean-front16k-sourcerun-desc32k-source4k-route32k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc32k_source4k_route32k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc32kSource4kRoute32kFlags }
        "mixedclean-front16k-sourcerun-desc32k-source3k-route32k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc32k_source3k_route32k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc32kSource3kRoute32kFlags }
        "mixedclean-front8k-sourcerun-desc32k-source4k-route32k" = @{ Suffix = "_mixedclean_front8k_sourcerun_desc32k_source4k_route32k"; ExtraFlags = $mixedCleanFront8kSourceRunDesc32kSource4kRoute32kFlags }
        "mixedclean-front12k-sourcerun-desc32k-source4k-route32k" = @{ Suffix = "_mixedclean_front12k_sourcerun_desc32k_source4k_route32k"; ExtraFlags = $mixedCleanFront12kSourceRunDesc32kSource4kRoute32kFlags }
        "mixedclean-front16k-sourcerun-desc32k-source2k-route32k" = @{ Suffix = "_mixedclean_front16k_sourcerun_desc32k_source2k_route32k"; ExtraFlags = $mixedCleanFront16kSourceRunDesc32kSource2kRoute32kFlags }
        "redislowrss-slim-route4k" = @{ Suffix = "_redislowrss_slim_route4k"; ExtraFlags = $redisLowRssSlimRoute4kFlags }
        "front1k-desc4k-source512-route4k" = @{ Suffix = "_front1k_desc4k_source512_route4k"; ExtraFlags = $front1kDesc4kSource512Route4kFlags }
        "appcap" = @{ Suffix = "_appcap"; ExtraFlags = $appLikeFlags }
        "visiblefirst-appcap" = @{ Suffix = "_visiblefirst_appcap"; ExtraFlags = $visibleFirstAppLikeFlags }
        "negativefilter-appcap" = @{ Suffix = "_negativefilter_appcap"; ExtraFlags = $negativeFilterAppLikeFlags }
        "sharedir-appcap" = @{ Suffix = "_sharedir_appcap"; ExtraFlags = $sharedRouteDirectoryAppLikeFlags }
        "sharedirfirst-appcap" = @{ Suffix = "_sharedirfirst_appcap"; ExtraFlags = $sharedRouteDirectoryFirstAppLikeFlags }
        "ownerlocality-appcap" = @{ Suffix = "_ownerlocality_appcap"; ExtraFlags = $ownerLocalityAppLikeFlags }
        "ownerlocalityfast-appcap" = @{ Suffix = "_ownerlocalityfast_appcap"; ExtraFlags = $ownerLocalityFastAppLikeFlags }
        "ownerlocalityfast-rsscap-1" = @{ Suffix = "_ownerlocalityfast_rsscap_1"; ExtraFlags = $ownerLocalityFastRssCap1Flags }
        "ownerlocalityfast-rsscap-2" = @{ Suffix = "_ownerlocalityfast_rsscap_2"; ExtraFlags = $ownerLocalityFastRssCap2Flags }
        "ownerlocalityfast-rsscap-2-desc192k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc192k"; ExtraFlags = $ownerLocalityFastRssCap2Desc192kFlags }
        "ownerlocalityfast-rsscap-2-desc160k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-route128k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_route128k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kRoute128kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-source2k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_source2k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kSource2kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source12k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource12kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source14k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource14kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route128k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route224k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute224kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run2048"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun2048Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run1024"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun1024Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir192k_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir192kSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_dir192k_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_dir192k_routepacked_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16Source16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_osdry_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16OwnerSourceDryRunSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2Source16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_sbp_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2SourceBlockPackedSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source2k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s2_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource2kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source8k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s8_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource8kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source12k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s12_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource12kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-elasticproj-local1k-route16k-source64-front1k-packed" = @{ Suffix = "_ownerloc_rss2_elproj_l1k_r16k_s64_f1k_packed"; ExtraFlags = $ownerLocalityFastRssCap2ElasticProjectionLocal1kRoute16kSource64Front1kPackedFlags }
        "ownerlocalityfast-rsscap-2-elasticproj-live2k-route16k-source128-front1k-packed" = @{ Suffix = "_ownerloc_rss2_elproj_l2k_r16k_s128_f1k_packed"; ExtraFlags = $ownerLocalityFastRssCap2ElasticProjectionLive2kRoute16kSource128Front1kPackedFlags }
        "ownerlocalityfast-rsscap-2-elasticroute-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512" = @{ Suffix = "_ownerloc_rss2_elroute_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r16_run512"; ExtraFlags = $ownerLocalityFastRssCap2ElasticRouteOverflowDesc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512Flags }
        "ownerlocalityfast-rsscap-2-elasticdescroute-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512" = @{ Suffix = "_ownerloc_rss2_eldescroute_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r16_run512"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource10kRoute16kRun512Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-localizedryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_locdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowLocalizeDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-runlocalitydryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_runlocdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowRunLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_depotrunmeta_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotRunMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_slotownerdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerLocalityDryRunDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownersparse-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_slotownersparse_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowSlotOwnerSparseMetaDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun4096Flags }
        "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = @{ Suffix = "_ownerloc_rss2_eldescsrcroute_depotownerdirect_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"; ExtraFlags = $ownerLocalityFastRssCap2ElasticDescriptorSourceRouteOverflowDepotOwnerDirectDesc16kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2FrontcachePackedSourceBlockPackedSource64Route16kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2dryrun-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2dry_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrNoRouteBackptrDir192kRoutePackedBytes16StorageOwner16OwnerSourceL2DryRunSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512" = @{ Suffix = "_ownerloc_rss2_d160_f4_thin_nb_so16_nrb_d192_rp_s16_r192_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrStorageOwner16NoRouteBackptrDir192kRoutePackedSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir128k-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir128k_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir128kSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir96k-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir96k_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrDir96kSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_sideowner16_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescNoBackptrSideOwner16Source16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc158k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc158kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc156k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc156kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc152k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc152kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc148k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc148kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc144k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc144kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc128k_front4k_thindesc_source16k_route192k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc128kFront4kThinDescSource16kRoute192kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route160k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route160k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute160kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route128k_run512"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute128kRun512Flags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route96k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route96k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute96kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route64k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route64k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource16kRoute64kFlags }
        "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source32k"; ExtraFlags = $ownerLocalityFastRssCap2Desc160kFront4kThinDescSource32kFlags }
        "ownerlocalityfast-rsscap-2-desc144k" = @{ Suffix = "_ownerlocalityfast_rsscap_2_desc144k"; ExtraFlags = $ownerLocalityFastRssCap2Desc144kFlags }
        "ownerlocalityfast-rsscap-3" = @{ Suffix = "_ownerlocalityfast_rsscap_3"; ExtraFlags = $ownerLocalityFastRssCap3Flags }
        "ownerlocalityfast-rsscap-4" = @{ Suffix = "_ownerlocalityfast_rsscap_4"; ExtraFlags = $ownerLocalityFastRssCap4Flags }
        "directlocalfree-ownerlocalityfast-rsscap-4" = @{ Suffix = "_directlocalfree_ownerlocalityfast_rsscap_4"; ExtraFlags = $directLocalFreeOwnerLocalityFastRssCap4Flags }
        "ownerlocalityfast-widecap-1" = @{ Suffix = "_ownerlocalityfast_widecap_1"; ExtraFlags = $ownerLocalityFastWideCap1Flags }
        "ownerlocalityfast-widecap-2" = @{ Suffix = "_ownerlocalityfast_widecap_2"; ExtraFlags = $ownerLocalityFastWideCap2Flags }
        "ownerlocalityfast-widecap-3" = @{ Suffix = "_ownerlocalityfast_widecap_3"; ExtraFlags = $ownerLocalityFastWideCap3Flags }
        "ownerlocalityfast-widecap-4" = @{ Suffix = "_ownerlocalityfast_widecap_4"; ExtraFlags = $ownerLocalityFastWideCap4Flags }
        "directlocalfree-ownerlocalityfast-widecap-4" = @{ Suffix = "_directlocalfree_ownerlocalityfast_widecap_4"; ExtraFlags = $directLocalFreeOwnerLocalityFastWideCap4Flags }
    }

    function Split-Hz6BuildList {
        param([string[]]$Values)
        $items = @()
        foreach ($value in @($Values)) {
            foreach ($part in @($value -split ',' | ForEach-Object { $_.Trim().ToLowerInvariant() } | Where-Object { $_ -ne "" })) {
                $items += $part
            }
        }
        $items
    }

    $selectedProfileNames = Split-Hz6BuildList -Values $Hz6Profiles
    if (-not $selectedProfileNames -or $selectedProfileNames.Count -eq 0) {
        $selectedProfileNames = @("strict", "speed", "rss")
    }

    $selectedLaneNames = Split-Hz6BuildList -Values $CapacityLanes
    if (-not $selectedLaneNames -or $selectedLaneNames.Count -eq 0) {
        if ($IncludeControlCapacity) {
            $selectedLaneNames = @("default", "broad", "control", "route4k", "appcap", "ownerlocality-appcap", "ownerlocalityfast-appcap")
        } else {
            $selectedLaneNames = @("default", "broad", "appcap")
        }
    }

    $profiles = @()
    foreach ($profileName in $selectedProfileNames) {
        if (-not $profileMap.ContainsKey($profileName)) {
            throw "Unknown HZ6 profile: $profileName"
        }
        $profiles += $profileMap[$profileName]
    }

    $selectedLanes = @()
    foreach ($laneName in $selectedLaneNames) {
        if (-not $laneMap.ContainsKey($laneName)) {
            throw "Unknown HZ6 capacity lane: $laneName"
        }
        $selectedLanes += $laneMap[$laneName]
    }

    foreach ($profile in $profiles) {
        foreach ($variant in $selectedLanes) {
            $output = Join-Path $OutDir ("{0}_hz6_{1}{2}.exe" -f $OutputPrefix, $profile.Name, $variant.Suffix)
            $args = @()
            $args += $commonFlags
            $args += "/DHZ_BENCH_USE_HZ6"
            $args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
            if ($DiagnosticHz6Probes) {
                $args += "/DHZ6_DIAGNOSTIC_PROBES=1"
            }
            $args += $variant.ExtraFlags
            $args += $includeFlags
            $args += $libSources
            $args += $BenchSrc
            $args += "/Fe:$output"
            Write-Host "[hz6-win] building $(Split-Path -Leaf $output)"
            & $Compiler @args
            if ($LASTEXITCODE -ne 0) {
                throw "HZ6 app-like bench build failed for $(Split-Path -Leaf $output) with exit code $LASTEXITCODE"
            }
        }
    }
}
