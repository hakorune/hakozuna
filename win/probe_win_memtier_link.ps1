[CmdletBinding()]
param(
    [string]$SourceRoot,
    [string]$OutDir,
    [string]$CompilerPath
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$CompatIncludeRoot = Join-Path $RepoRoot "win\memtier\include"
$MemcachedCompatIncludeRoot = Join-Path $RepoRoot "win\memcached\include"

if (-not $SourceRoot) {
    $SourceRoot = Join-Path $RepoRoot "private\bench-assets\windows\memcached\client\memtier_benchmark-2.2.1"
}
if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\client_link_probe"
}
if (-not $CompilerPath) {
    $CompilerPath = "C:\LLVM-18\bin\clang-cl.exe"
}

foreach ($path in @($SourceRoot, $CompatIncludeRoot, $MemcachedCompatIncludeRoot)) {
    if (-not (Test-Path $path)) {
        throw "required path not found: $path"
    }
}
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
}

$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\vcpkg" }
$IncludeRoot = Join-Path $VcpkgRoot "installed\x64-windows\include"
$LibRoot = Join-Path $VcpkgRoot "installed\x64-windows\lib"
if (-not (Test-Path $IncludeRoot)) {
    throw "vcpkg include root not found: $IncludeRoot"
}
if (-not (Test-Path $LibRoot)) {
    throw "vcpkg lib root not found: $LibRoot"
}

$VersionHeaderPath = Join-Path $SourceRoot "version.h"
if (-not (Test-Path $VersionHeaderPath)) {
    $ProbeScript = Join-Path $RepoRoot "win\probe_win_memtier_msvc.ps1"
    & $ProbeScript | Out-Null
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$ExePath = Join-Path $OutDir ($Stamp + "_memtier_benchmark.exe")
$LogPath = Join-Path $OutDir ($Stamp + "_memtier_link_clang_cl.log")

$Sources = @(
    "memtier_benchmark.cpp",
    "client.cpp",
    "cluster_client.cpp",
    "shard_connection.cpp",
    "run_stats_types.cpp",
    "run_stats.cpp",
    "JSON_handler.cpp",
    "protocol.cpp",
    "obj_gen.cpp",
    "item.cpp",
    "file_io.cpp",
    "config_types.cpp",
    "deps\hdr_histogram\hdr_histogram_log.c",
    "deps\hdr_histogram\hdr_histogram.c",
    "deps\hdr_histogram\hdr_time.c",
    "deps\hdr_histogram\hdr_encoding.c"
) | ForEach-Object { Join-Path $SourceRoot $_ }
$Sources += Join-Path $RepoRoot "win\memtier\getopt_shim.c"

foreach ($source in $Sources) {
    if (-not (Test-Path $source)) {
        throw "source file not found: $source"
    }
}

$Args = @(
    "/nologo",
    "/std:c++20",
    "/EHsc",
    "/I", $CompatIncludeRoot,
    "/I", $MemcachedCompatIncludeRoot,
    "/I", $SourceRoot,
    "/I", $IncludeRoot,
    "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH",
    "/DNOMINMAX",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/D_CRT_NONSTDC_NO_WARNINGS",
    "/DHAVE_CONFIG_H",
    "/FIpthread.h",
    "/FIassert.h",
    "/Fe$ExePath"
)
$Args += $Sources
$Args += @(
    (Join-Path $LibRoot "event.lib"),
    (Join-Path $LibRoot "event_core.lib"),
    (Join-Path $LibRoot "event_extra.lib"),
    (Join-Path $LibRoot "zlib.lib"),
    "ws2_32.lib",
    "iphlpapi.lib",
    "advapi32.lib",
    "user32.lib",
    "shell32.lib"
)

$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$output = & $CompilerPath @Args 2>&1
$rc = $LASTEXITCODE
$ErrorActionPreference = $previousErrorActionPreference

$log = New-Object System.Collections.Generic.List[string]
$log.Add("compiler: $CompilerPath")
$log.Add("exe: $ExePath")
$log.Add("command: " + ($Args -join " "))
$log.Add("rc: $rc")
$log.Add("")
foreach ($line in $output) {
    $log.Add($line.ToString())
}
Set-Content -Path $LogPath -Value $log -Encoding UTF8

Write-Host "Wrote log: $LogPath"

if ($rc -ne 0) {
    throw "memtier link probe failed (see $LogPath)"
}

[pscustomobject]@{
    ExePath = $ExePath
    LogPath = $LogPath
}
