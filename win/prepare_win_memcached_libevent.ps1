[CmdletBinding()]
param(
    [string]$VcpkgRoot,
    [string]$Triplet = "x64-windows",
    [switch]$InstallIfMissing
)

$ErrorActionPreference = "Stop"

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}

$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $VcpkgExe)) {
    throw "vcpkg.exe not found at $VcpkgExe"
}

$InstallRoot = Join-Path $VcpkgRoot ("installed\" + $Triplet)
$IncludeRoot = Join-Path $InstallRoot "include"
$LibRoot = Join-Path $InstallRoot "lib"
$BinRoot = Join-Path $InstallRoot "bin"

$HeaderEvent2 = Join-Path $IncludeRoot "event2\event.h"
$HeaderThread = Join-Path $IncludeRoot "event2\thread.h"
$HeaderCompat = Join-Path $IncludeRoot "event.h"
$LibBase = Join-Path $LibRoot "event.lib"
$LibCore = Join-Path $LibRoot "event_core.lib"
$LibExtra = Join-Path $LibRoot "event_extra.lib"
$DllBase = Join-Path $BinRoot "event.dll"
$DllCore = Join-Path $BinRoot "event_core.dll"
$DllExtra = Join-Path $BinRoot "event_extra.dll"

function Test-LibeventReady {
    return (Test-Path $HeaderEvent2) -and (Test-Path $HeaderThread) -and (Test-Path $HeaderCompat) -and
        (Test-Path $LibBase) -and (Test-Path $LibCore) -and (Test-Path $LibExtra)
}

if ((-not (Test-LibeventReady)) -and $InstallIfMissing) {
    & $VcpkgExe install "libevent[thread]:$Triplet"
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg install libevent[thread]:$Triplet failed with exit code $LASTEXITCODE"
    }
}

$Ready = Test-LibeventReady
$Status = [pscustomobject]@{
    vcpkg_root = $VcpkgRoot
    triplet = $Triplet
    ready = $Ready
    include_root = $IncludeRoot
    lib_root = $LibRoot
    bin_root = $BinRoot
    header_event2 = $HeaderEvent2
    header_thread = $HeaderThread
    header_compat = $HeaderCompat
    lib_base = $LibBase
    lib_core = $LibCore
    lib_extra = $LibExtra
    dll_base = $DllBase
    dll_core = $DllCore
    dll_extra = $DllExtra
}

$Status | Format-List | Out-String | Write-Host

if (-not $Ready) {
    Write-Host ""
    Write-Host "libevent for Windows is not ready."
    Write-Host "Next step:"
    Write-Host ("  powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_libevent.ps1 -InstallIfMissing -Triplet {0}" -f $Triplet)
    exit 1
}

Write-Host ""
Write-Host "libevent Windows dependency box looks ready."
Write-Host "Use these paths for the native memcached MSVC box:"
Write-Host ("  include: {0}" -f $IncludeRoot)
Write-Host ("  lib:     {0}" -f $LibRoot)
Write-Host ("  bin:     {0}" -f $BinRoot)
exit 0
