param(
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_suite"

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Hz3Script = Join-Path $RepoRoot "hakozuna\\win\\build_win_bench_compare.ps1"
$Hz4Script = Join-Path $RepoRoot "hakozuna-mt\\win\\build_win_bench_compare.ps1"

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
