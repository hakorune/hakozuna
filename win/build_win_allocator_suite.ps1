param(
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_suite"

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Hz3Script = Join-Path $RepoRoot "hakozuna\\win\\build_win_bench_compare.ps1"
$Hz4Script = Join-Path $RepoRoot "hakozuna-mt\\win\\build_win_bench_compare.ps1"
$Hz6Root = Join-Path $RepoRoot "hakozuna-hz6"
$Hz6Common = Join-Path $Hz6Root "win\\hz6_win_build_common.ps1"

$Hz3Args = @()
$Hz4Args = @()
if ($VcpkgRoot) {
    $Hz3Args += @("-VcpkgRoot", $VcpkgRoot)
    $Hz4Args += @("-VcpkgRoot", $VcpkgRoot)
}

& $Hz3Script @Hz3Args
if ($LASTEXITCODE -ne 0) {
    throw "hz3 build script failed with exit code $LASTEXITCODE"
}

& $Hz4Script @Hz4Args
if ($LASTEXITCODE -ne 0) {
    throw "hz4 build script failed with exit code $LASTEXITCODE"
}

$Hz3OutDir = Join-Path $RepoRoot "hakozuna\out_win_bench"
$Hz4OutDir = Join-Path $RepoRoot "hakozuna-mt\out_win_bench"

if (Test-Path $Hz6Common) {
    . $Hz6Common
    $Compiler = Get-Command "clang-cl" -ErrorAction Stop
    $Hz6IncludeFlags = Get-Hz6WinIncludeFlags -Hz6Root $Hz6Root -ExtraIncludeRoots @("win")
    $Hz6LibSources = Get-Hz6WinLibSources -Hz6Root $Hz6Root
    $Hz6CommonFlags = Get-Hz6WinClangCommonFlags
    $CompareSource = Join-Path $PSScriptRoot "bench_allocator_compare.c"
    $Hz6Profiles = @(
        @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" },
        @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" },
        @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" }
    )

    foreach ($profile in $Hz6Profiles) {
        $output = Join-Path $OutDir ("bench_mixed_ws_hz6_{0}.exe" -f $profile.Name)
        $args = @()
        $args += $Hz6CommonFlags
        $args += "/DHZ_BENCH_USE_HZ6"
        $args += "/DHZ_BENCH_DISABLE_REALLOC=1"
        $args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
        $args += $Hz6IncludeFlags
        $args += $Hz6LibSources
        $args += $CompareSource
        $args += "/Fe:$output"
        Write-Host ("[hz6-win] building {0}" -f (Split-Path -Leaf $output))
        & $Compiler.Source @args
        if ($LASTEXITCODE -ne 0) {
            throw "HZ6 mixed_ws build failed for $($profile.Name) with exit code $LASTEXITCODE"
        }
    }
} else {
    Write-Warning "HZ6 Windows build helper not found; skipping HZ6 matrix executables: $Hz6Common"
}

$Artifacts = @(
    (Join-Path $Hz3OutDir "bench_mixed_ws_crt.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_hz3.exe"),
    (Join-Path $Hz4OutDir "bench_mixed_ws_hz4.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_mimalloc.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_tcmalloc.exe"),
    (Join-Path $Hz3OutDir "mimalloc.dll"),
    (Join-Path $Hz3OutDir "mimalloc-redirect.dll"),
    (Join-Path $Hz3OutDir "tcmalloc_minimal.dll")
)

foreach ($artifact in $Artifacts) {
    if (Test-Path $artifact) {
        Copy-Item -Force $artifact $OutDir | Out-Null
    }
}

Write-Host "Built suite artifacts in: $OutDir"
