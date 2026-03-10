[CmdletBinding()]
param(
    [string]$VcpkgRoot,
    [string]$Triplet = "x64-windows",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}
if (-not $OutDir) {
    $OutDir = ".\private\raw-results\windows\memcached\compile_probe"
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ResolvedOutDir = (Resolve-Path (New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot $OutDir))).Path
$CompatIncludeRoot = Join-Path $RepoRoot "win\memcached\include"
$SharedCompatIncludeRoot = Join-Path $RepoRoot "win\include"
$WrapperPath = Join-Path $RepoRoot "win\memcached\memcached_win_min_main.c"

$Compiler = Get-Command "clang-cl" -ErrorAction SilentlyContinue
if (-not $Compiler) {
    throw "clang-cl not found in PATH"
}

$IncludeRoot = Join-Path $VcpkgRoot ("installed\" + $Triplet + "\include")
$CompatHeader = Join-Path $IncludeRoot "event.h"
if (-not (Test-Path $CompatHeader)) {
    throw "libevent compatibility header not found at $CompatHeader. Run win/prepare_win_memcached_libevent.ps1 first."
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$ObjPath = Join-Path $ResolvedOutDir "memcached_win_min_main.obj"
$StdoutPath = Join-Path $ResolvedOutDir ($Stamp + "_memcached_win_min_main_stdout.log")
$StderrPath = Join-Path $ResolvedOutDir ($Stamp + "_memcached_win_min_main_stderr.log")
$LogPath = Join-Path $ResolvedOutDir ($Stamp + "_memcached_win_min_main_clang_cl.log")

$Args = @(
    "/nologo",
    "/c",
    "/W3",
    "/I", $CompatIncludeRoot,
    "/I", $SharedCompatIncludeRoot,
    "/I", $IncludeRoot,
    "/DHAVE_CONFIG_H",
    "/D_WIN32",
    "/DWIN32",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/Fo" + $ObjPath,
    $WrapperPath
)

$Proc = Start-Process -FilePath $Compiler.Source -ArgumentList $Args -NoNewWindow -Wait -PassThru `
    -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath
$ExitCode = $Proc.ExitCode

$Merged = @()
if (Test-Path $StdoutPath) {
    $Merged += Get-Content -Path $StdoutPath
}
if (Test-Path $StderrPath) {
    $Merged += Get-Content -Path $StderrPath
}
$Merged | Set-Content -Path $LogPath -Encoding utf8

Write-Host ("compiler: {0}" -f $Compiler.Source)
Write-Host ("translation_unit: {0}" -f $WrapperPath)
Write-Host ("log: {0}" -f $LogPath)
Write-Host ("exit_code: {0}" -f $ExitCode)

if ($Merged) {
    $Merged | Select-Object -First 60 | ForEach-Object { Write-Host $_ }
}

exit $ExitCode
