param(
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}

function Test-CommandPath {
    param(
        [string]$CommandName
    )

    $cmd = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    return $null
}

function Write-Status {
    param(
        [string]$Label,
        [bool]$Ok,
        [string]$Detail
    )

    $status = if ($Ok) { "OK" } else { "MISSING" }
    Write-Host ("[{0}] {1}: {2}" -f $status, $Label, $Detail)
}

$clangCl = Test-CommandPath "clang-cl"
$llvmLib = Test-CommandPath "llvm-lib"
$msLib = Test-CommandPath "lib"
$vcpkgExe = if (Test-Path (Join-Path $VcpkgRoot "vcpkg.exe")) { Join-Path $VcpkgRoot "vcpkg.exe" } else { $null }
$miHeader = Join-Path $VcpkgRoot "installed\x64-windows\include\mimalloc.h"
$miLib = Join-Path $VcpkgRoot "installed\x64-windows\lib\mimalloc.dll.lib"
$tcHeader = Join-Path $VcpkgRoot "installed\x64-windows\include\gperftools\tcmalloc.h"
$tcLib = Join-Path $VcpkgRoot "installed\x64-windows\lib\tcmalloc_minimal.lib"

Write-Status "clang-cl" ($null -ne $clangCl) ($(if ($clangCl) { $clangCl } else { "not in PATH" }))
Write-Status "llvm-lib" ($null -ne $llvmLib) ($(if ($llvmLib) { $llvmLib } else { "not in PATH" }))
Write-Status "lib.exe" ($null -ne $msLib) ($(if ($msLib) { $msLib } else { "optional if llvm-lib exists" }))
Write-Status "vcpkg" ($null -ne $vcpkgExe) ($(if ($vcpkgExe) { $vcpkgExe } else { "$VcpkgRoot\vcpkg.exe not found" }))
Write-Status "mimalloc header" (Test-Path $miHeader) $miHeader
Write-Status "mimalloc import lib" (Test-Path $miLib) $miLib
Write-Status "tcmalloc header" (Test-Path $tcHeader) $tcHeader
Write-Status "tcmalloc import lib" (Test-Path $tcLib) $tcLib

$allGood = ($null -ne $clangCl) -and (($null -ne $llvmLib) -or ($null -ne $msLib)) -and
    ($null -ne $vcpkgExe) -and (Test-Path $miHeader) -and (Test-Path $miLib) -and
    (Test-Path $tcHeader) -and (Test-Path $tcLib)

if ($allGood) {
    Write-Host ""
    Write-Host "Windows allocator dev environment looks ready."
    Write-Host "Next steps:"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\win\build_win_min.ps1"
    Write-Host "  powershell -ExecutionPolicy Bypass -File .\win\build_win_bench_compare.ps1"
    exit 0
}

Write-Host ""
Write-Host "One or more prerequisites are missing."
Write-Host "Suggested installs:"
Write-Host "  LLVM with clang-cl in PATH"
Write-Host "  Visual Studio Build Tools or LLVM llvm-lib"
Write-Host "  vcpkg install mimalloc:x64-windows gperftools:x64-windows"
exit 1
