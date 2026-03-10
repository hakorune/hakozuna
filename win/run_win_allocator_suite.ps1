param(
    [int]$Threads = 4,
    [int]$ItersPerThread = 1000000,
    [int]$WorkingSet = 8192,
    [int]$MinSize = 16,
    [int]$MaxSize = 1024,
    [string]$VcpkgRoot
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"

$Expected = @(
    (Join-Path $OutDir "bench_mixed_ws_crt.exe"),
    (Join-Path $OutDir "bench_mixed_ws_hz3.exe"),
    (Join-Path $OutDir "bench_mixed_ws_hz4.exe"),
    (Join-Path $OutDir "bench_mixed_ws_mimalloc.exe"),
    (Join-Path $OutDir "bench_mixed_ws_tcmalloc.exe")
)

if ($Expected | Where-Object { -not (Test-Path $_) }) {
    $BuildArgs = @()
    if ($VcpkgRoot) {
        $BuildArgs += @("-VcpkgRoot", $VcpkgRoot)
    }
    & $BuildScript @BuildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

$Runs = @(
    @{ Name = "crt"; Path = (Join-Path $OutDir "bench_mixed_ws_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $OutDir "bench_mixed_ws_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $OutDir "bench_mixed_ws_hz4.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $OutDir "bench_mixed_ws_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $OutDir "bench_mixed_ws_tcmalloc.exe") }
)

$Failures = @()
foreach ($run in $Runs) {
    Write-Host "=== $($run.Name) ==="
    & $run.Path $Threads $ItersPerThread $WorkingSet $MinSize $MaxSize
    if ($LASTEXITCODE -ne 0) {
        $Failures += "$($run.Name): rc=$LASTEXITCODE"
        Write-Warning "$($run.Name) failed with exit code $LASTEXITCODE"
    }
}

if ($Failures.Count -gt 0) {
    throw ("Allocator suite completed with failures: " + ($Failures -join ", "))
}
