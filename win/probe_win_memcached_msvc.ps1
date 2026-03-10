[CmdletBinding()]
param(
    [string]$SourceRoot,
    [string]$VcpkgRoot,
    [string]$Triplet = "x64-windows",
    [string]$TranslationUnit = "memcached.c",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $SourceRoot) {
    $SourceRoot = ".\private\bench-assets\windows\memcached\source\memcached-1.6.40"
}
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
$ResolvedSourceRoot = (Resolve-Path (Join-Path $RepoRoot $SourceRoot)).Path
$ResolvedOutDir = (Resolve-Path (New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot $OutDir))).Path
$CompatIncludeRoot = Join-Path $RepoRoot "win\memcached\include"
$SharedCompatIncludeRoot = Join-Path $RepoRoot "win\include"

$Compiler = Get-Command "clang-cl" -ErrorAction SilentlyContinue
if (-not $Compiler) {
    throw "clang-cl not found in PATH"
}

$IncludeRoot = Join-Path $VcpkgRoot ("installed\" + $Triplet + "\include")
$CompatHeader = Join-Path $IncludeRoot "event.h"
if (-not (Test-Path $CompatHeader)) {
    throw "libevent compatibility header not found at $CompatHeader. Run win/prepare_win_memcached_libevent.ps1 first."
}

$TuPath = Join-Path $ResolvedSourceRoot $TranslationUnit
if (-not (Test-Path $TuPath)) {
    throw "translation unit not found: $TuPath"
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$ObjPath = Join-Path $ResolvedOutDir ([IO.Path]::GetFileNameWithoutExtension($TranslationUnit) + ".obj")
$StdoutPath = Join-Path $ResolvedOutDir ($Stamp + "_" + [IO.Path]::GetFileNameWithoutExtension($TranslationUnit) + "_stdout.log")
$StderrPath = Join-Path $ResolvedOutDir ($Stamp + "_" + [IO.Path]::GetFileNameWithoutExtension($TranslationUnit) + "_stderr.log")
$LogPath = Join-Path $ResolvedOutDir ($Stamp + "_" + [IO.Path]::GetFileNameWithoutExtension($TranslationUnit) + "_clang_cl.log")

$Args = @(
    "/nologo",
    "/c",
    "/W3",
    "/I", $CompatIncludeRoot,
    "/I", $SharedCompatIncludeRoot,
    "/I", $ResolvedSourceRoot,
    "/I", $IncludeRoot,
    "/DHAVE_CONFIG_H",
    "/D_WIN32",
    "/DWIN32",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/Fo" + $ObjPath,
    $TuPath
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
Write-Host ("translation_unit: {0}" -f $TuPath)
Write-Host ("log: {0}" -f $LogPath)
Write-Host ("exit_code: {0}" -f $ExitCode)

if ($Merged) {
    $Merged | Select-Object -First 40 | ForEach-Object { Write-Host $_ }
}

exit $ExitCode
