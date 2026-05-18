param(
    [switch]$Minimal,
    [switch]$MmanStats,
    [string[]]$ExtraDefines,
    [string]$OutDirName,
    [switch]$TlsDeclspec,
    [switch]$TlsEmulated,
    [switch]$TlsInitLog,
    [switch]$TlsInitFailfast,
    [switch]$SmallSlowStats,
    [switch]$ArenaFailShot,
    [switch]$OomShot,
    [string]$Profile = "",
    [string]$ArenaSize = ""
)

$ErrorActionPreference = "Stop"

$Hz3Dir = Split-Path -Parent $PSScriptRoot
$ResolvedOutDirName = $OutDirName
if (-not $ResolvedOutDirName) {
    $ResolvedOutDirName = if ($Minimal) { "out_win_min" } else { "out_win" }
}
$OutDir = Join-Path $Hz3Dir $ResolvedOutDirName
$ObjDir = Join-Path $OutDir "obj"

New-Item -ItemType Directory -Force $ObjDir | Out-Null

$Cc = "clang-cl"
if (-not (Get-Command $Cc -ErrorAction SilentlyContinue)) {
    throw "clang-cl not found in PATH."
}

function Invoke-Checked {
    param(
        [string]$Exe,
        [string[]]$ArgList
    )
    & $Exe @ArgList | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "$Exe failed with exit code $LASTEXITCODE"
    }
}

$KnownProfiles = @(
    "legacy",
    "speed-default",
    "rss-first",
    "rss-experimental",
    "rss-mru",
    "unsafe-repro-s260",
    "custom"
)

$ResolvedProfile = $Profile
if (-not $ResolvedProfile) {
    $ResolvedProfile = if ($Minimal) { "legacy" } else { "speed-default" }
}
if ($KnownProfiles -notcontains $ResolvedProfile) {
    throw "Unknown HZ3 profile '$ResolvedProfile'. Expected one of: $($KnownProfiles -join ', ')"
}

$ResolvedArenaSize = $ArenaSize
if (-not $ResolvedArenaSize) {
    $ResolvedArenaSize = if ($ResolvedProfile -in @("speed-default", "rss-first", "rss-experimental", "rss-mru", "unsafe-repro-s260")) {
        "0x200000000ULL"
    } else {
        "0x1000000000ULL"
    }
}

function Get-Hz3ProfileDefines {
    param([string]$Name)

    $speedDefault = @(
        "HZ3_PAGE_MEDIUM_ALIGNED_DEFAULT=1",
        "HZ3_S65_CENTRAL_COLD_ENABLE=1",
        "HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE=1",
        "HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE=1",
        "HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS=64",
        "HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL=1",
        "HZ3_S65_DTOR_RECLAIM_AFTER_INBOX_DRAIN=1",
        "HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS=2",
        "HZ3_S65_DTOR_FINAL_RECLAIM_AFTER_DRAIN=1",
        "HZ3_S65_EXITING_INBOX_TO_CENTRAL=1",
        "HZ3_S65_EXITING_MARK_AT_DTOR_START=1",
        "HZ3_S240_LARGE_OWNER_FRONT=1",
        "HZ3_S240_LARGE_OWNER_FRONT_STATS=0",
        "HZ3_S240_LARGE_FRONT_CACHE=1",
        "HZ3_S240_LARGE_OWNER_INBOX=1",
        "HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES=1048576",
        "HZ3_S242_LARGE_DIRECT_SIDE_MAP=1",
        "HZ3_S242_DIRECT_MAPLESS=1",
        "HZ3_S242_DIRECT_MAPLESS_MAX_CLASS_BYTES=1048576",
        "HZ3_S242_DIRECT_STATS=0",
        "HZ3_S242_DIRECT_ENTRY_COUNT=0",
        "HZ3_S246_LARGE_INBOX_TAKE_FIRST=1"
    )

    $mediumCounters = @(
        "HZ3_S203_COUNTERS=1",
        "HZ3_S203_ALLOC_SC=1",
        "HZ3_S203_S65_COLD=1",
        "HZ3_S203_S65_COLD_SC=1"
    )

    $speedDefaultQuiet = @($speedDefault + @(
        "HZ3_S80_MEDIUM_RECLAIM_LOG=0"
    ))

    $targetedReclaim = @($mediumCounters + @(
        "HZ3_S65_MEDIUM_RECLAIM_MODE=2",
        "HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS=4096",
        "HZ3_S65_DELAY_EPOCHS_BUSY=0",
        "HZ3_S65_DELAY_EPOCHS_IDLE=0",
        "HZ3_S65_PURGE_BUDGET_PAGES=65536",
        "HZ3_S65_PURGE_MAX_CALLS=1024",
        "HZ3_S65_STATS=1",
        "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM=1",
        "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_SCALE=1",
        "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_RUNS=4096",
        "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_ALLOW_LIVE_EXITING=0"
    ))

    switch ($Name) {
        "legacy" { return @() }
        "custom" { return @() }
        "speed-default" { return $speedDefaultQuiet }
        "unsafe-repro-s260" { return @($speedDefault + $targetedReclaim) }
        "rss-first" {
            return @($speedDefault + $targetedReclaim + @(
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES=1",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES=2",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_PAGE_MASK=0x00000006u"
            ))
        }
        "rss-experimental" {
            return @($speedDefault + $targetedReclaim + @(
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES=1",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES=2",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_PAGE_MASK=0x00000006u",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_STRIDE=8"
            ))
        }
        "rss-mru" {
            return @($speedDefault + $targetedReclaim + @(
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MIN_PAGES=1",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_MAX_PAGES=2",
                "HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_PAGE_MASK=0x00000006u",
                "HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE=1"
            ))
        }
    }
}

$Defines = @(
    "HZ3_ENABLE=1",
    "HZ3_SHIM_FORWARD_ONLY=0"
)

if ($Minimal) {
    $Defines += @(
        "HZ3_SMALL_V2_ENABLE=0",
        "HZ3_SEG_SELF_DESC_ENABLE=0",
        "HZ3_SMALL_V2_PTAG_ENABLE=0",
        "HZ3_PTAG_V1_ENABLE=0",
        "HZ3_PTAG_DSTBIN_ENABLE=0",
        "HZ3_PTAG_DSTBIN_FASTLOOKUP=0",
        "HZ3_PTAG32_NOINRANGE=0",
        "HZ3_REALLOC_PTAG32=0",
        "HZ3_USABLE_SIZE_PTAG32=0",
        "HZ3_TCACHE_SOA=0",
        "HZ3_TCACHE_SOA_LOCAL=0",
        "HZ3_TCACHE_SOA_BANK=0",
        "HZ3_BIN_PAD_LOG2=0"
    )
} else {
    $Defines += @(
        "HZ3_SMALL_V2_ENABLE=1",
        "HZ3_SEG_SELF_DESC_ENABLE=1",
        "HZ3_SMALL_V2_PTAG_ENABLE=1",
        "HZ3_PTAG_V1_ENABLE=1",
        "HZ3_TCACHE_SOA=1",
        "HZ3_PTAG_DSTBIN_ENABLE=1",
        "HZ3_PTAG_DSTBIN_FASTLOOKUP=1",
        "HZ3_PTAG32_NOINRANGE=1",
        "HZ3_REALLOC_PTAG32=1",
        "HZ3_USABLE_SIZE_PTAG32=1",
        "HZ3_BIN_COUNT_POLICY=1",
        "HZ3_LOCAL_BINS_SPLIT=1",
        "HZ3_TCACHE_INIT_ON_MISS=1",
        "HZ3_BIN_PAD_LOG2=8",
        "HZ3_NUM_SHARDS=63",
        "HZ3_REMOTE_STASH_SPARSE=1",
        "HZ3_SHARD_COLLISION_FAILFAST=1",
        "HZ3_SHARD_COLLISION_SHOT=1",
        "HZ3_ARENA_SIZE=$ResolvedArenaSize",
        "HZ3_S42_SMALL_XFER=1",
        "HZ3_S44_OWNER_STASH=1",
        "HZ3_S44_OWNER_STASH_FASTPOP=1",
        "HZ3_S44_OWNER_STASH_COUNT=0",
        "HZ3_S51_LARGE_MADVISE=0",
        "HZ3_S52_BESTFIT_RANGE=4",
        "HZ3_ARENA_PRESSURE_BOX=0",
        "HZ3_S65_RELEASE_BOUNDARY=1",
        "HZ3_S65_RELEASE_LEDGER=1",
        "HZ3_S65_MEDIUM_RECLAIM=1",
        "HZ3_S74_LANE_BATCH=1",
        "HZ3_S74_REFILL_BURST=8",
        "HZ3_S74_FLUSH_BATCH=64",
        "HZ3_S74_STATS=0",
        "HZ3_OWNER_EXCL_ENABLE=0",
        "HZ3_S142_CENTRAL_LOCKFREE=1",
        "HZ3_S142_XFER_LOCKFREE=1"
    )
}

$Defines += Get-Hz3ProfileDefines -Name $ResolvedProfile

if ($MmanStats) {
    $Defines += "HZ3_WIN_MMAN_STATS=1"
}
if ($TlsDeclspec) {
    $Defines += "HZ3_TLS_DECLSPEC=1"
} elseif ($TlsEmulated) {
    $Defines += "HZ3_TLS_DECLSPEC=0"
}
if ($TlsInitLog) {
    $Defines += "HZ3_TLS_INIT_LOG=1"
}
if ($TlsInitFailfast) {
    $Defines += "HZ3_TLS_INIT_FAILFAST=1"
}
if ($SmallSlowStats) {
    $Defines += "HZ3_S85_SMALL_V2_SLOW_STATS=1"
}
if ($ArenaFailShot) {
    $Defines += "HZ3_ARENA_ALLOC_FAIL_SHOT=1"
}
if ($OomShot) {
    $Defines += "HZ3_OOM_SHOT=1"
}
if ($ExtraDefines) {
    $Defines += $ExtraDefines
}

$DefFlags = $Defines | ForEach-Object { "/D$_" }
$IncFlags = @(
    "/I$Hz3Dir\\include",
    "/I$Hz3Dir\\win\\include"
)
$CFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3"
) + $DefFlags + $IncFlags

$SrcFiles = Get-ChildItem (Join-Path $Hz3Dir "src") -Filter "*.c" | Where-Object {
    $_.Name -ne "hz3_shim.c"
}

$Objs = @()
foreach ($src in $SrcFiles) {
    $obj = Join-Path $ObjDir ($src.BaseName + ".obj")
    Invoke-Checked $Cc ($CFlags + @("/c", $src.FullName, "/Fo$obj"))
    $Objs += $obj
}

$LibTool = $null
$LibCmd = Get-Command "llvm-lib" -ErrorAction SilentlyContinue
if ($LibCmd) {
    $LibTool = $LibCmd.Path
} else {
    $LibCmd = Get-Command "lib" -ErrorAction SilentlyContinue
    if ($LibCmd) {
        $LibTool = $LibCmd.Path
    }
}
if (-not $LibTool) {
    throw "llvm-lib or lib.exe not found in PATH."
}

$LibOut = Join-Path $OutDir "hz3_win.lib"
Invoke-Checked $LibTool (@("/nologo", "/out:$LibOut") + $Objs)

$BenchSrc = Join-Path $Hz3Dir "bench\\bench_minimal.c"
$BenchOut = Join-Path $OutDir "bench_minimal.exe"

Invoke-Checked $Cc ($CFlags + @($BenchSrc, $LibOut, "/link", "/out:$BenchOut"))

Write-Host "Built: $BenchOut"
Write-Host "Built: $LibOut"
Write-Host "Profile: $ResolvedProfile"
Write-Host "ArenaSize: $ResolvedArenaSize"
