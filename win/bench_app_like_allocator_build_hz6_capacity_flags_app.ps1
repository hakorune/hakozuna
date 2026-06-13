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

