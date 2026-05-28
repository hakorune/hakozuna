param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutDir
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Hz6Root = Join-Path $RepoRoot "hakozuna-hz6"
if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "out_win_mt_remote_hz6_owner"
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

. (Join-Path $Hz6Root "win\hz6_win_build_common.ps1")

$Compiler = Get-Command $CompilerPath -ErrorAction Stop
$IncludeFlags = Get-Hz6WinIncludeFlags -Hz6Root $Hz6Root -ExtraIncludeRoots @("win")
$LibSources = Get-Hz6WinLibSources -Hz6Root $Hz6Root
$CommonFlags = Get-Hz6WinClangCommonFlags
$BenchSource = Join-Path $PSScriptRoot "bench_random_mixed_mt_remote_hz6_owner_compare.c"

if (-not (Test-Path $BenchSource)) {
    throw "Benchmark source not found: $BenchSource"
}

$Profiles = @(
    @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" },
    @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" },
    @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" },
    @{ Name = "remote"; Define = "HZ6_PROFILE_REMOTE" }
)

foreach ($profile in $Profiles) {
    $OutputPath = Join-Path $OutDir ("bench_random_mixed_mt_remote_hz6_owner_{0}.exe" -f $profile.Name)
    $Args = @()
    $Args += $CommonFlags
    $Args += "/DHZ_BENCH_USE_HZ6=1"
    $Args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
    $Args += $IncludeFlags
    $Args += $LibSources
    $Args += $BenchSource
    $Args += "/Fe:$OutputPath"

    Write-Host "[hz6-win] building $(Split-Path -Leaf $OutputPath)"
    & $Compiler @Args
    if ($LASTEXITCODE -ne 0) {
        throw "HZ6 owner-aware mt-remote build failed for $($profile.Name) with exit code $LASTEXITCODE"
    }
}

Write-Host "Built HZ6 owner-aware mt-remote artifacts in: $OutDir"
