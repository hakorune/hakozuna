param(
    [string]$LegacyWindowsBenchRoot,
    [switch]$CopyLegacyPrebuilt,
    [switch]$CopyLegacySource,
    [switch]$CopyLegacyResults
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$PrivateRoot = Join-Path $RepoRoot "private"
$BenchAssetsRoot = Join-Path $PrivateRoot "bench-assets\windows\redis"
$PrebuiltRoot = Join-Path $BenchAssetsRoot "prebuilt"
$SourceRoot = Join-Path $BenchAssetsRoot "source"
$RawResultsRoot = Join-Path $PrivateRoot "raw-results\windows\redis"

if (-not $LegacyWindowsBenchRoot) {
    $LegacyWindowsBenchRoot = Join-Path $RepoRoot "private\hakmem\hakozuna\bench\windows"
}

$LegacyPrebuiltRoot = Join-Path $LegacyWindowsBenchRoot "redis"
$LegacySourceRoot = Join-Path $LegacyWindowsBenchRoot "redis-src"
$LegacyResultsRoot = Join-Path $LegacyWindowsBenchRoot "results"

foreach ($path in @($BenchAssetsRoot, $PrebuiltRoot, $SourceRoot, $RawResultsRoot)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force $path | Out-Null
    }
}

$notes = @(
    "Private Windows Redis layout",
    "",
    "Canonical paths:",
    ("- prebuilt: {0}" -f $PrebuiltRoot),
    ("- source: {0}" -f $SourceRoot),
    ("- raw results: {0}" -f $RawResultsRoot),
    "",
    "Legacy source, if still present:",
    ("- {0}" -f $LegacyWindowsBenchRoot),
    "",
    "Public scripts should prefer this layout and only fall back to the legacy tree when needed."
)
Set-Content -Path (Join-Path $BenchAssetsRoot "README.txt") -Value $notes -Encoding ASCII

function Copy-TreeIfPresent {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        Write-Host "skip missing path: $Source"
        return
    }
    Copy-Item -Path (Join-Path $Source "*") -Destination $Destination -Recurse -Force
    Write-Host "copied: $Source -> $Destination"
}

if ($CopyLegacyPrebuilt) {
    Copy-TreeIfPresent -Source $LegacyPrebuiltRoot -Destination $PrebuiltRoot
}
if ($CopyLegacySource) {
    Copy-TreeIfPresent -Source $LegacySourceRoot -Destination $SourceRoot
}
if ($CopyLegacyResults) {
    Copy-TreeIfPresent -Source $LegacyResultsRoot -Destination $RawResultsRoot
}

Write-Host "Prepared private Redis layout:"
Write-Host ("  prebuilt    {0}" -f $PrebuiltRoot)
Write-Host ("  source      {0}" -f $SourceRoot)
Write-Host ("  raw-results {0}" -f $RawResultsRoot)
