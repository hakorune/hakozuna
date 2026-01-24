param(
    [switch]$MmanStats
)

$ErrorActionPreference = "Stop"

$Hz3Dir = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $Hz3Dir "out_win_link"
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
    "HZ3_SHIM_FORWARD_ONLY=0",
    "HZ3_SMALL_V2_ENABLE=1",
    "HZ3_SEG_SELF_DESC_ENABLE=1",
    "HZ3_SMALL_V2_PTAG_ENABLE=1",
    "HZ3_PTAG_V1_ENABLE=1",
    "HZ3_PTAG_DSTBIN_ENABLE=1",
    "HZ3_PTAG_DSTBIN_FASTLOOKUP=1",
    "HZ3_PTAG32_NOINRANGE=1",
    "HZ3_REALLOC_PTAG32=1",
    "HZ3_USABLE_SIZE_PTAG32=1",
    "HZ3_BIN_COUNT_POLICY=1",
    "HZ3_LOCAL_BINS_SPLIT=1",
    "HZ3_TCACHE_INIT_ON_MISS=1",
    "HZ3_BIN_PAD_LOG2=8"
)

if ($MmanStats) {
    $Defines += "HZ3_WIN_MMAN_STATS=1"
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

$CppFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c++17",
    "/EHsc",
    "/W3",
    "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH"
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

$HookC = Join-Path $Hz3Dir "win\\hz3_hook_link.c"
$HookCObj = Join-Path $ObjDir "hz3_hook_link.obj"
Invoke-Checked $Cc ($CFlags + @("/c", $HookC, "/Fo$HookCObj"))

$HookCpp = Join-Path $Hz3Dir "win\\hz3_hook_link_cpp.cc"
$HookCppObj = Join-Path $ObjDir "hz3_hook_link_cpp.obj"
Invoke-Checked $Cc ($CppFlags + @("/c", $HookCpp, "/Fo$HookCppObj"))

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
$BenchOut = Join-Path $OutDir "bench_minimal_link.exe"

$BenchDefs = @("/DHZ3_BENCH_USE_CRT=1")
Invoke-Checked $Cc ($CFlags + $BenchDefs + @($BenchSrc, $HookCObj, $HookCppObj, $LibOut, "/link", "/out:$BenchOut"))

Write-Host "Built: $BenchOut"
