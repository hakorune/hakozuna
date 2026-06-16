function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute4kLinearWrapLoopCarryCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1",
        "/DHZ6_ROUTE_LINEAR_WRAP_L1=1",
        "/DHZ6_ROUTE_LOOP_CARRY_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute8kLinearWrapLoopCarryCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1",
        "/DHZ6_ROUTE_LINEAR_WRAP_L1=1",
        "/DHZ6_ROUTE_LOOP_CARRY_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource4kRoute8kLinearWrapLoopCarryCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)4096)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1",
        "/DHZ6_ROUTE_LINEAR_WRAP_L1=1",
        "/DHZ6_ROUTE_LOOP_CARRY_L1=1"
    )
}

function Get-Hz6WinMixedCleanFront16kSourceRunDesc17kSource2kRoute16kLinearWrapLoopCarryCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)17408)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)16384)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)2176)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)2048)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)16384)",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1",
        "/DHZ6_ROUTE_LINEAR_WRAP_L1=1",
        "/DHZ6_ROUTE_LOOP_CARRY_L1=1"
    )
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
