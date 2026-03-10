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
    [switch]$OomShot
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
        "HZ3_ARENA_SIZE=0x1000000000ULL",
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
