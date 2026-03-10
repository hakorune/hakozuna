[CmdletBinding()]
param(
    [string]$SourceRoot,
    [string]$VcpkgRoot,
    [string]$Triplet = "x64-windows",
    [string]$OutDir = "",
    [switch]$KeepExe
)

$ErrorActionPreference = "Stop"

if (-not $OutDir) {
    $OutDir = ".\private\raw-results\windows\memcached\link_probe"
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ResolvedOutDir = (Resolve-Path (New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot $OutDir))).Path
$ExePath = Join-Path $ResolvedOutDir "memcached_win_min_main.exe"
$BuildScript = Join-Path $RepoRoot "win\build_win_memcached_min_main.ps1"
& $BuildScript -SourceRoot $SourceRoot -VcpkgRoot $VcpkgRoot -Triplet $Triplet -OutDir $OutDir -ExeName (Split-Path $ExePath -Leaf)
$ExitCode = $LASTEXITCODE

if ((-not $KeepExe) -and (Test-Path $ExePath)) {
    Remove-Item -Force $ExePath
}

exit $ExitCode
