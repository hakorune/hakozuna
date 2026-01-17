param(
    [switch]$RebuildHz3,
    [switch]$SkipHz3Build,
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

$Hz3Dir = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $Hz3Dir "out_win_bench"
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

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\\vcpkg"
}

$VcpkgInclude = Join-Path $VcpkgRoot "installed\\x64-windows\\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\\x64-windows\\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\\x64-windows\\bin"

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$Hz3Dir\\include",
    "/I$Hz3Dir\\win\\include"
)

$BenchSrc = Join-Path $Hz3Dir "bench\\bench_mixed_ws.c"

Write-Host "Building: bench_mixed_ws (CRT baseline)"
$BenchCrtOut = Join-Path $OutDir "bench_mixed_ws_crt.exe"
Invoke-Checked $Cc ($BaseFlags + @("/DHZ3_BENCH_USE_CRT=1", $BenchSrc, "/link", "/out:$BenchCrtOut"))

$Hz3Lib = Join-Path $Hz3Dir "out_win\\hz3_win.lib"
if (-not $SkipHz3Build) {
    if ($RebuildHz3 -or -not (Test-Path $Hz3Lib)) {
        $BuildHz3 = Join-Path $Hz3Dir "win\\build_win_min.ps1"
        & $BuildHz3 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "build_win_min.ps1 failed with exit code $LASTEXITCODE"
        }
    }
}

if (Test-Path $Hz3Lib) {
    Write-Host "Building: bench_mixed_ws (hz3)"
    $BenchHz3Out = Join-Path $OutDir "bench_mixed_ws_hz3.exe"
    Invoke-Checked $Cc ($BaseFlags + @($BenchSrc, $Hz3Lib, "/link", "/out:$BenchHz3Out"))
} else {
    Write-Warning "hz3_win.lib not found; skipping hz3 bench."
}

$MiHeader = Join-Path $VcpkgInclude "mimalloc.h"
$MiLib = Join-Path $VcpkgLib "mimalloc.dll.lib"
if ((Test-Path $MiHeader) -and (Test-Path $MiLib)) {
    Write-Host "Building: bench_mixed_ws (mimalloc)"
    $BenchMiOut = Join-Path $OutDir "bench_mixed_ws_mimalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ3_BENCH_USE_MIMALLOC=1", $BenchSrc, $MiLib, "/link", "/out:$BenchMiOut"))
} else {
    Write-Warning "mimalloc not found in $VcpkgRoot; skipping mimalloc bench."
}

$TcHeader = Join-Path $VcpkgInclude "gperftools\\tcmalloc.h"
$TcLib = Join-Path $VcpkgLib "tcmalloc_minimal.lib"
if ((Test-Path $TcHeader) -and (Test-Path $TcLib)) {
    Write-Host "Building: bench_mixed_ws (tcmalloc)"
    $BenchTcOut = Join-Path $OutDir "bench_mixed_ws_tcmalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ3_BENCH_USE_TCMALLOC=1", $BenchSrc, $TcLib, "/link", "/out:$BenchTcOut"))
} else {
    Write-Warning "tcmalloc not found in $VcpkgRoot; skipping tcmalloc bench."
}

$MiDll = Join-Path $VcpkgBin "mimalloc.dll"
if (Test-Path $MiDll) {
    Copy-Item -Force $MiDll $OutDir | Out-Null
}
$MiRedirect = Join-Path $VcpkgBin "mimalloc-redirect.dll"
if (Test-Path $MiRedirect) {
    Copy-Item -Force $MiRedirect $OutDir | Out-Null
}
$TcDll = Join-Path $VcpkgBin "tcmalloc_minimal.dll"
if (Test-Path $TcDll) {
    Copy-Item -Force $TcDll $OutDir | Out-Null
}

Write-Host "Built: $BenchCrtOut"
if ($BenchHz3Out -and (Test-Path $BenchHz3Out)) { Write-Host "Built: $BenchHz3Out" }
if ($BenchMiOut -and (Test-Path $BenchMiOut)) { Write-Host "Built: $BenchMiOut" }
if ($BenchTcOut -and (Test-Path $BenchTcOut)) { Write-Host "Built: $BenchTcOut" }
