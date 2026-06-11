param(
    [string]$VcpkgRoot,
    [switch]$OnlyHz7V4,
    [int]$Hz7V4DirectRetainCap = 0,
    [int]$Hz7V4EmptySpanCap = 0,
    [int]$Hz7V4SpanClassMax = 0,
    [string]$OutDirName = "out_win_random_mixed_hz7_v4"
)

$ErrorActionPreference = "Stop"

$Hz7V4Root = Split-Path -Parent $PSScriptRoot
$Hz7FamilyRoot = Split-Path -Parent $Hz7V4Root
$ProjectRoot = Split-Path -Parent $Hz7FamilyRoot
$OutDir = Join-Path $Hz7FamilyRoot $OutDirName
New-Item -ItemType Directory -Force $OutDir | Out-Null

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

$VcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"

$BenchSrc = Join-Path $ProjectRoot "win\bench_random_mixed_compare.c"
$Hz7Source = Join-Path $Hz7V4Root "hz7.c"
$BenchOut = Join-Path $OutDir "bench_random_mixed_hz7_v4.exe"

if (-not (Test-Path $BenchSrc)) {
    throw "bench source not found: $BenchSrc"
}
if (-not (Test-Path $Hz7Source)) {
    throw "HZ7 v4 source not found: $Hz7Source"
}

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$ProjectRoot",
    "/I$ProjectRoot\hakozuna\include",
    "/I$ProjectRoot\hakozuna\win\include",
    "/I$ProjectRoot\hakozuna-mt\core",
    "/I$ProjectRoot\hakozuna-mt\include",
    "/I$ProjectRoot\hakozuna-mt\win",
    "/I$Hz7V4Root"
)

$Flags = @("/DHZ_BENCH_USE_HZ7=1")
if ($Hz7V4DirectRetainCap -gt 0) {
    $Flags += "/DH7_DIRECT_RETAIN_CAP=$Hz7V4DirectRetainCap"
}
if ($Hz7V4EmptySpanCap -gt 0) {
    $Flags += "/DH7_EMPTY_SPAN_CAP=$Hz7V4EmptySpanCap"
}
if ($Hz7V4SpanClassMax -gt 0) {
    $Flags += "/DH7_SPAN_CLASS_MAX=$Hz7V4SpanClassMax"
}

Write-Host "[hz7-v4] building bench_random_mixed_hz7_v4.exe"
Invoke-Checked $Cc ($BaseFlags + $Flags + @($Hz7Source, $BenchSrc, "psapi.lib", "/link", "/out:$BenchOut"))
Write-Host "Built HZ7 v4 random_mixed artifact in: $OutDir"
