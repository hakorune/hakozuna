param(
    [string]$VcpkgRoot,
    [switch]$DiagnosticHz6Probes
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_larson"
$ObjDir = Join-Path $OutDir "obj"
$SuiteBuild = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"
$ModernBuildCommon = Join-Path $PSScriptRoot "bench_app_like_allocator_build_common.ps1"

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

. $ModernBuildCommon

if ($DiagnosticHz6Probes) {
    & $SuiteBuild -DiagnosticHz6Probes
} else {
    & $SuiteBuild
}
if ($LASTEXITCODE -ne 0) {
    throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
}

$VcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"

$BenchSrc = Join-Path $RepoRoot "win\bench_larson_compare.c"
$Hz3Dir = Join-Path $RepoRoot "hakozuna"
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$Hz11Root = Join-Path $RepoRoot "hakozuna-hz11"
$Hz3Lib = Join-Path $Hz3Dir "out_win\hz3_win.lib"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\hz4_win.lib"

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$PSScriptRoot",
    "/I$Hz3Dir\include",
    "/I$Hz3Dir\win\include",
    "/I$Hz4Dir\core",
    "/I$Hz4Dir\include",
    "/I$Hz4Dir\win"
)

Write-Host "Building: larson (CRT baseline)"
$BenchCrtOut = Join-Path $OutDir "bench_larson_crt.exe"
Invoke-Checked $Cc ($BaseFlags + @($BenchSrc, "/link", "/out:$BenchCrtOut"))

if (Test-Path $Hz3Lib) {
    Write-Host "Building: larson (hz3)"
    $BenchHz3Out = Join-Path $OutDir "bench_larson_hz3.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ3=1", $BenchSrc, $Hz3Lib, "/link", "/out:$BenchHz3Out"))
} else {
    Write-Warning "hz3_win.lib not found; skipping hz3 larson bench."
}

$Hz4ApiObj = Join-Path $ObjDir "hz4_win_api_larson.obj"
$Hz4Flags = $BaseFlags + @(
    "/D__thread=__declspec(thread)",
    "/DHZ4_TLS_DIRECT=0",
    "/DHZ4_PAGE_META_SEPARATE=0",
    "/DHZ4_RSSRETURN=0",
    "/DHZ4_MID_PAGE_SUPPLY_RESV_BOX=0",
    "/DHZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1",
    "/DHZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1"
)
if ((Test-Path $Hz4Lib) -and (Test-Path (Join-Path $Hz4Dir "win\hz4_win_api.c"))) {
    Invoke-Checked $Cc ($Hz4Flags + @("/c", (Join-Path $Hz4Dir "win\hz4_win_api.c"), "/Fo$Hz4ApiObj"))
    Write-Host "Building: larson (hz4)"
    $BenchHz4Out = Join-Path $OutDir "bench_larson_hz4.exe"
    Invoke-Checked $Cc ($Hz4Flags + @("/DHZ_BENCH_USE_HZ4=1", $BenchSrc, $Hz4ApiObj, $Hz4Lib, "/link", "/out:$BenchHz4Out"))
} else {
    Write-Warning "hz4_win.lib not found; skipping hz4 larson bench."
}

$MiHeader = Join-Path $VcpkgInclude "mimalloc.h"
$MiLib = Join-Path $VcpkgLib "mimalloc.dll.lib"
if ((Test-Path $MiHeader) -and (Test-Path $MiLib)) {
    Write-Host "Building: larson (mimalloc)"
    $BenchMiOut = Join-Path $OutDir "bench_larson_mimalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_MIMALLOC=1", $BenchSrc, $MiLib, "/link", "/out:$BenchMiOut"))
} else {
    Write-Warning "mimalloc not found in $VcpkgRoot; skipping mimalloc larson bench."
}

$TcHeader = Join-Path $VcpkgInclude "gperftools\tcmalloc.h"
$TcLib = Join-Path $VcpkgLib "tcmalloc_minimal.lib"
if ((Test-Path $TcHeader) -and (Test-Path $TcLib)) {
    Write-Host "Building: larson (tcmalloc)"
    $BenchTcOut = Join-Path $OutDir "bench_larson_tcmalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_TCMALLOC=1", $BenchSrc, $TcLib, "/link", "/out:$BenchTcOut"))
} else {
    Write-Warning "tcmalloc not found in $VcpkgRoot; skipping tcmalloc larson bench."
}

Invoke-AppLikeHz5BenchBuild `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutputPath (Join-Path $OutDir "bench_larson_hz5_policy.exe")

if (Test-Path $Hz11Root) {
    $Hz11Sources = @(
        (Join-Path $Hz11Root "src\hz11_size_class.c"),
        (Join-Path $Hz11Root "src\hz11_sys_alloc.c"),
        (Join-Path $Hz11Root "src\hz11_thread_cache.c"),
        (Join-Path $Hz11Root "src\hz11_public_entry.c"),
        (Join-Path $Hz11Root "src\hz11_span.c"),
        (Join-Path $Hz11Root "src\hz11_live_footprint.c")
    )
    $missingHz11 = $Hz11Sources | Where-Object { -not (Test-Path $_) }
    if ($missingHz11.Count -gt 0) {
        throw "HZ11 source missing for Larson build: $($missingHz11 -join ', ')"
    }
    foreach ($variant in @(
        @{
            Name = "hz11-span"
            Output = "bench_larson_hz11_span.exe"
            ExtraFlags = @()
        },
        @{
            Name = "hz11-span-cache256"
            Output = "bench_larson_hz11_span_cache256.exe"
            ExtraFlags = @("/DHZ11_CACHE_CAP=256")
        },
        @{
            Name = "hz11-span-cache256-bumpbatch16"
            Output = "bench_larson_hz11_span_cache256_bumpbatch16.exe"
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_SPAN_BUMP_BATCH=1",
                "/DHZ11_SPAN_BUMP_BATCH_COUNT=16"
            )
        },
        @{
            Name = "hz11-span-cache256-bumpbatch16-diag"
            Output = "bench_larson_hz11_span_cache256_bumpbatch16_diag.exe"
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_SPAN_BUMP_BATCH=1",
                "/DHZ11_SPAN_BUMP_BATCH_COUNT=16",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_CLASS_DIAG=1",
                "/DHZ11_MATRIX_ATTRIB_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11-span-cache256-returnedrange"
            Output = "bench_larson_hz11_span_cache256_returnedrange.exe"
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_RETURNED_PUSH_RANGE=1"
            )
        },
        @{
            Name = "hz11-span-cache256-returnedrange-diag"
            Output = "bench_larson_hz11_span_cache256_returnedrange_diag.exe"
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_RETURNED_PUSH_RANGE=1",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_CLASS_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11-span-cache256-returnedrange32"
            Output = "bench_larson_hz11_span_cache256_returnedrange32.exe"
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_RETURNED_PUSH_RANGE=1",
                "/DHZ11_RETURNED_PUSH_RANGE_CHUNK=32"
            )
        },
        @{
            Name = "hz11-span-cache512-classbatch16-coldskip"
            Output = "bench_larson_hz11_span_cache512_classbatch16_coldskip.exe"
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=16",
                "/DHZ11_RETURNED_REFILL_COLD_SKIP=1",
                "/DHZ11_RETURNED_REFILL_COLD_SKIP_BUDGET=8"
            )
        }
    )) {
        Write-Host "Building: larson ($($variant.Name))"
        $out = Join-Path $OutDir $variant.Output
        Invoke-Checked $Cc ($BaseFlags + @(
            "/DHZ_BENCH_USE_HZ11=1",
            "/DHZ11_CLASSIFY_SPAN=1",
            "/I$Hz11Root\include",
            "/I$Hz11Root\src"
        ) + $variant.ExtraFlags + @($BenchSrc) + $Hz11Sources + @("psapi.lib", "/link", "/out:$out"))
    }
} else {
    Write-Warning "HZ11 root not found; skipping HZ11 Larson benches."
}

Invoke-AppLikeHz6BenchBuilds `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutDir $OutDir `
    -OutputPrefix "bench_larson" `
    -IncludeControlCapacity `
    -DiagnosticHz6Probes:$DiagnosticHz6Probes

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

Write-Host "Built Larson artifacts in: $OutDir"
