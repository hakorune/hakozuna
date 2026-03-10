[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$SourceRoot,
    [string]$CompilerPath
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_memtier"
}
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
}

$ProbeScript = Join-Path $RepoRoot "win\probe_win_memtier_link.ps1"
$probeResult = & $ProbeScript -OutDir $OutputDir -SourceRoot $SourceRoot -CompilerPath $CompilerPath

$BuiltExePath = $probeResult.ExePath
$BuiltLogPath = $probeResult.LogPath
$CanonicalExePath = Join-Path $OutputDir "memtier_benchmark.exe"
$CanonicalLogPath = Join-Path $OutputDir "memtier_benchmark_build.log"

Copy-Item -Path $BuiltExePath -Destination $CanonicalExePath -Force
Copy-Item -Path $BuiltLogPath -Destination $CanonicalLogPath -Force

[pscustomobject]@{
    ExePath = $CanonicalExePath
    LogPath = $CanonicalLogPath
    SourceExePath = $BuiltExePath
    SourceLogPath = $BuiltLogPath
}
