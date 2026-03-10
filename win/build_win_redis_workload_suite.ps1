param(
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_redis"
$ObjDir = Join-Path $OutDir "obj"
$SuiteBuild = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"

New-Item -ItemType Directory -Force $OutDir | Out-Null
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

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}

& $SuiteBuild
if ($LASTEXITCODE -ne 0) {
    throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
}

$VcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"

$BenchSrc = Join-Path $RepoRoot "win\bench_redis_workload_compare.c"
$Hz3Dir = Join-Path $RepoRoot "hakozuna"
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$Hz3Lib = Join-Path $Hz3Dir "out_win\hz3_win.lib"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\hz4_win.lib"

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$Hz3Dir\include",
    "/I$Hz3Dir\win\include",
    "/I$Hz4Dir\core",
    "/I$Hz4Dir\include",
    "/I$Hz4Dir\win"
)

Write-Host "Building: redis workload (CRT baseline)"
$BenchCrtOut = Join-Path $OutDir "bench_redis_workload_crt.exe"
Invoke-Checked $Cc ($BaseFlags + @($BenchSrc, "/link", "/out:$BenchCrtOut"))

if (Test-Path $Hz3Lib) {
    Write-Host "Building: redis workload (hz3)"
    $BenchHz3Out = Join-Path $OutDir "bench_redis_workload_hz3.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ3=1", $BenchSrc, $Hz3Lib, "/link", "/out:$BenchHz3Out"))
} else {
    Write-Warning "hz3_win.lib not found; skipping hz3 redis workload bench."
}

$Hz4ApiObj = Join-Path $ObjDir "hz4_win_api_redis.obj"
$Hz4Flags = $BaseFlags + @(
    "/D__thread=__declspec(thread)",
    "/DHZ4_TLS_DIRECT=0",
    "/DHZ4_PAGE_META_SEPARATE=0",
    "/DHZ4_RSSRETURN=0",
    "/DHZ4_MID_PAGE_SUPPLY_RESV_BOX=0"
)
if ((Test-Path $Hz4Lib) -and (Test-Path (Join-Path $Hz4Dir "win\hz4_win_api.c"))) {
    Invoke-Checked $Cc ($Hz4Flags + @("/c", (Join-Path $Hz4Dir "win\hz4_win_api.c"), "/Fo$Hz4ApiObj"))
    Write-Host "Building: redis workload (hz4)"
    $BenchHz4Out = Join-Path $OutDir "bench_redis_workload_hz4.exe"
    Invoke-Checked $Cc ($Hz4Flags + @("/DHZ_BENCH_USE_HZ4=1", $BenchSrc, $Hz4ApiObj, $Hz4Lib, "/link", "/out:$BenchHz4Out"))
} else {
    Write-Warning "hz4_win.lib not found; skipping hz4 redis workload bench."
}

$MiHeader = Join-Path $VcpkgInclude "mimalloc.h"
$MiLib = Join-Path $VcpkgLib "mimalloc.dll.lib"
if ((Test-Path $MiHeader) -and (Test-Path $MiLib)) {
    Write-Host "Building: redis workload (mimalloc)"
    $BenchMiOut = Join-Path $OutDir "bench_redis_workload_mimalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_MIMALLOC=1", $BenchSrc, $MiLib, "/link", "/out:$BenchMiOut"))
} else {
    Write-Warning "mimalloc not found in $VcpkgRoot; skipping mimalloc redis workload bench."
}

$TcHeader = Join-Path $VcpkgInclude "gperftools\tcmalloc.h"
$TcLib = Join-Path $VcpkgLib "tcmalloc_minimal.lib"
if ((Test-Path $TcHeader) -and (Test-Path $TcLib)) {
    Write-Host "Building: redis workload (tcmalloc)"
    $BenchTcOut = Join-Path $OutDir "bench_redis_workload_tcmalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_TCMALLOC=1", $BenchSrc, $TcLib, "/link", "/out:$BenchTcOut"))
} else {
    Write-Warning "tcmalloc not found in $VcpkgRoot; skipping tcmalloc redis workload bench."
}

$RuntimeDlls = @(
    (Join-Path $VcpkgBin "mimalloc.dll"),
    (Join-Path $VcpkgBin "mimalloc-redirect.dll"),
    (Join-Path $VcpkgBin "tcmalloc_minimal.dll")
)

foreach ($dll in $RuntimeDlls) {
    if (Test-Path $dll) {
        Copy-Item -Force $dll $OutDir | Out-Null
    }
}

Write-Host "Built Redis workload artifacts in: $OutDir"
