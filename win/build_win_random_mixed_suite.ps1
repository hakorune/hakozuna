param(
    [string]$VcpkgRoot,
    [switch]$DiagnosticHz6Probes,
    [switch]$OnlyHz7V2,
    [switch]$OnlyHz11,
    [int]$Hz7V2DirectRetainCap = 0,
    [int]$Hz7V2EmptySpanCap = 0,
    [int]$Hz7V2SpanClassMax = 0,
    [string]$OutDirName = "out_win_random_mixed"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot $OutDirName
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

$VcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"

$BenchSrc = Join-Path $RepoRoot "win\bench_random_mixed_compare.c"
$Hz3Dir = Join-Path $RepoRoot "hakozuna"
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$Hz7Dir = Join-Path $RepoRoot "hakozuna-hz7"
$Hz7Dir = Join-Path $RepoRoot "hz7"
$Hz7V2Dir = Join-Path $Hz7Dir "v2"
$Hz11Dir = Join-Path $RepoRoot "hakozuna-hz11"
$Hz3Lib = Join-Path $Hz3Dir "out_win\hz3_win.lib"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\hz4_win.lib"
$Hz7Source = Join-Path $Hz7Dir "hz7.c"
$Hz7V2Source = Join-Path $Hz7V2Dir "hz7.c"
$Hz11Sources = @(
    (Join-Path $Hz11Dir "src\hz11_size_class.c"),
    (Join-Path $Hz11Dir "src\hz11_sys_alloc.c"),
    (Join-Path $Hz11Dir "src\hz11_thread_cache.c"),
    (Join-Path $Hz11Dir "src\hz11_public_entry.c")
)

function Invoke-Hz11RandomMixedBuilds {
    if (($Hz11Sources | Where-Object { -not (Test-Path $_) }).Count -ne 0) {
        throw "HZ11 sources not found; cannot build HZ11 random_mixed bench."
    }

    Write-Host "Building: bench_random_mixed (hz11-token)"
    $BenchHz11TokenOut = Join-Path $OutDir "bench_random_mixed_hz11_token.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ11=1", $BenchSrc) + $Hz11Sources + @("psapi.lib", "/link", "/out:$BenchHz11TokenOut"))

    Write-Host "Building: bench_random_mixed (hz11-tlsfast)"
    $BenchHz11TlsfastOut = Join-Path $OutDir "bench_random_mixed_hz11_tlsfast.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ11=1", "/DHZ11_TLS_FASTPATH=1", $BenchSrc) + $Hz11Sources + @("psapi.lib", "/link", "/out:$BenchHz11TlsfastOut"))
}

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
    "/I$Hz4Dir\win",
    "/I$Hz7Dir",
    "/I$Hz7V2Dir",
    "/I$Hz11Dir\include",
    "/I$Hz11Dir\src"
)

if ($OnlyHz7V2) {
    if (Test-Path $Hz7V2Source) {
        Write-Host "Building: bench_random_mixed (hz7-v2 only)"
        $BenchHz7V2Out = Join-Path $OutDir "bench_random_mixed_hz7_v2.exe"
        $Hz7V2Flags = @("/DHZ_BENCH_USE_HZ7=1")
        if ($Hz7V2DirectRetainCap -gt 0) {
            $Hz7V2Flags += "/DH7_DIRECT_RETAIN_CAP=$Hz7V2DirectRetainCap"
        }
        if ($Hz7V2EmptySpanCap -gt 0) {
            $Hz7V2Flags += "/DH7_EMPTY_SPAN_CAP=$Hz7V2EmptySpanCap"
        }
        if ($Hz7V2SpanClassMax -gt 0) {
            $Hz7V2Flags += "/DH7_SPAN_CLASS_MAX=$Hz7V2SpanClassMax"
        }
        Invoke-Checked $Cc ($BaseFlags + $Hz7V2Flags + @($Hz7V2Source, $BenchSrc, "psapi.lib", "/link", "/out:$BenchHz7V2Out"))
        Write-Host "Built HZ7 v2 random_mixed artifact in: $OutDir"
        return
    }
    throw "HZ7 v2 source not found: $Hz7V2Source"
}

if ($OnlyHz11) {
    Invoke-Hz11RandomMixedBuilds
    Write-Host "Built HZ11 random_mixed artifacts in: $OutDir"
    return
}

& $SuiteBuild
if ($LASTEXITCODE -ne 0) {
    throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
}

Write-Host "Building: bench_random_mixed (CRT baseline)"
$BenchCrtOut = Join-Path $OutDir "bench_random_mixed_crt.exe"
Invoke-Checked $Cc ($BaseFlags + @($BenchSrc, "psapi.lib", "/link", "/out:$BenchCrtOut"))

if (Test-Path $Hz3Lib) {
    Write-Host "Building: bench_random_mixed (hz3)"
    $BenchHz3Out = Join-Path $OutDir "bench_random_mixed_hz3.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ3=1", $BenchSrc, $Hz3Lib, "psapi.lib", "/link", "/out:$BenchHz3Out"))
} else {
    Write-Warning "hz3_win.lib not found; skipping hz3 random_mixed bench."
}

$Hz4ApiObj = Join-Path $ObjDir "hz4_win_api_random_mixed.obj"
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
    Write-Host "Building: bench_random_mixed (hz4)"
    $BenchHz4Out = Join-Path $OutDir "bench_random_mixed_hz4.exe"
    Invoke-Checked $Cc ($Hz4Flags + @("/DHZ_BENCH_USE_HZ4=1", $BenchSrc, $Hz4ApiObj, $Hz4Lib, "psapi.lib", "/link", "/out:$BenchHz4Out"))
} else {
    Write-Warning "hz4_win.lib not found; skipping hz4 random_mixed bench."
}

$MiHeader = Join-Path $VcpkgInclude "mimalloc.h"
$MiLib = Join-Path $VcpkgLib "mimalloc.dll.lib"
if ((Test-Path $MiHeader) -and (Test-Path $MiLib)) {
    Write-Host "Building: bench_random_mixed (mimalloc)"
    $BenchMiOut = Join-Path $OutDir "bench_random_mixed_mimalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_MIMALLOC=1", $BenchSrc, $MiLib, "psapi.lib", "/link", "/out:$BenchMiOut"))
} else {
    Write-Warning "mimalloc not found in $VcpkgRoot; skipping mimalloc random_mixed bench."
}

$TcHeader = Join-Path $VcpkgInclude "gperftools\tcmalloc.h"
$TcLib = Join-Path $VcpkgLib "tcmalloc_minimal.lib"
if ((Test-Path $TcHeader) -and (Test-Path $TcLib)) {
    Write-Host "Building: bench_random_mixed (tcmalloc)"
    $BenchTcOut = Join-Path $OutDir "bench_random_mixed_tcmalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_TCMALLOC=1", $BenchSrc, $TcLib, "psapi.lib", "/link", "/out:$BenchTcOut"))
} else {
    Write-Warning "tcmalloc not found in $VcpkgRoot; skipping tcmalloc random_mixed bench."
}

Invoke-AppLikeHz5BenchBuild `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutputPath (Join-Path $OutDir "bench_random_mixed_hz5_policy.exe")

if (Test-Path $Hz7Source) {
    Write-Host "Building: bench_random_mixed (hz7-tinyroute)"
    $BenchHz7Out = Join-Path $OutDir "bench_random_mixed_hz7.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ7=1", $Hz7Source, $BenchSrc, "psapi.lib", "/link", "/out:$BenchHz7Out"))
} else {
    Write-Warning "HZ7 source not found; skipping HZ7 random_mixed bench."
}

if (Test-Path $Hz7V2Source) {
    Write-Host "Building: bench_random_mixed (hz7-v2)"
    $BenchHz7V2Out = Join-Path $OutDir "bench_random_mixed_hz7_v2.exe"
    $Hz7V2Flags = @("/DHZ_BENCH_USE_HZ7=1")
    if ($Hz7V2DirectRetainCap -gt 0) {
        $Hz7V2Flags += "/DH7_DIRECT_RETAIN_CAP=$Hz7V2DirectRetainCap"
    }
    if ($Hz7V2EmptySpanCap -gt 0) {
        $Hz7V2Flags += "/DH7_EMPTY_SPAN_CAP=$Hz7V2EmptySpanCap"
    }
    if ($Hz7V2SpanClassMax -gt 0) {
        $Hz7V2Flags += "/DH7_SPAN_CLASS_MAX=$Hz7V2SpanClassMax"
    }
    Invoke-Checked $Cc ($BaseFlags + $Hz7V2Flags + @($Hz7V2Source, $BenchSrc, "psapi.lib", "/link", "/out:$BenchHz7V2Out"))
} else {
    Write-Warning "HZ7 v2 source not found; skipping HZ7 v2 random_mixed bench."
}

if (($Hz11Sources | Where-Object { -not (Test-Path $_) }).Count -eq 0) {
    Invoke-Hz11RandomMixedBuilds
} else {
    Write-Warning "HZ11 sources not found; skipping HZ11 random_mixed bench."
}

Invoke-AppLikeHz6BenchBuilds `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutDir $OutDir `
    -OutputPrefix "bench_random_mixed" `
    -DiagnosticHz6Probes:$DiagnosticHz6Probes `
    -IncludeControlCapacity

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

Write-Host "Built random_mixed artifacts in: $OutDir"
