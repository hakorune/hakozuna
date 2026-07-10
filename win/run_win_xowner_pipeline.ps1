param(
    [string]$OutputPath,
    [string]$SuiteDirName = "out_win_xowner",
    [int]$Runs = 3,
    [int]$RuntimeSeconds = 5,
    [int]$Producers = 4,
    [int]$Consumers = 4,
    [int]$RingCapacity = 4096,
    [int]$MinSize = 8,
    [int]$MaxSize = 1024
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot $SuiteDirName
$BuildScript = Join-Path $PSScriptRoot "build_win_xowner_pipeline.ps1"
if (-not (Test-Path (Join-Path $SuiteDir "bench_xowner_hz11.exe"))) {
    & $BuildScript -OutDirName $SuiteDirName
    if ($LASTEXITCODE -ne 0) { throw "xowner pipeline build failed: $LASTEXITCODE" }
}
$executables = @(
    @{ Name = "hz11"; Path = (Join-Path $SuiteDir "bench_xowner_hz11.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_xowner_tcmalloc.exe") }
)
if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path $RepoRoot "docs\benchmarks\windows\xowner_pipeline_$stamp.md"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $OutputPath) | Out-Null

function Get-Median([double[]]$values) {
    $sorted = @($values | Sort-Object)
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return $sorted[$mid] }
    return ($sorted[$mid - 1] + $sorted[$mid]) / 2.0
}

$rows = @()
$env:Path = (Join-Path (Resolve-Path $SuiteDir) ".") + ";" + $env:Path
foreach ($exe in $executables) {
    if (-not (Test-Path $exe.Path)) { continue }
    for ($run = 1; $run -le $Runs; $run++) {
        $output = & $exe.Path $RuntimeSeconds $Producers $Consumers $RingCapacity $MinSize $MaxSize 2>&1
        $line = ($output | Select-String '\[XOWNER_PIPELINE\]').Line
        if (-not $line) { throw "$($exe.Name) produced no xowner result" }
        if ($line -notmatch 'ops/s=([0-9.]+)') { throw "Cannot parse ops/s: $line" }
        $rows += [pscustomobject]@{
            Name = $exe.Name
            Run = $run
            Ops = [double]$Matches[1]
            Line = $line
        }
        Write-Host "[XOWNER_PIPELINE] allocator=$($exe.Name) run=$run $line"
    }
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# Windows Cross-Owner Pipeline")
$lines.Add("")
$lines.Add("- Runs: $Runs, runtime: ${RuntimeSeconds}s, producers/consumers: $Producers/$Consumers")
$lines.Add("- Ring: $RingCapacity, size: $MinSize..$MaxSize")
$lines.Add("")
$lines.Add("| Allocator | Median ops/s | Runs |")
$lines.Add("| --- | ---: | ---: |")
foreach ($group in ($rows | Group-Object Name)) {
    $median = Get-Median @($group.Group.Ops)
    $lines.Add(("| {0} | {1:N3}M | {2} |" -f $group.Name, ($median / 1000000.0), $group.Count))
}
$lines.Add("")
$lines.Add("Every completed object is allocated by a producer and freed by a different consumer; sharing_factor=2 and cross_owner_rate=100% are structural properties of this lane.")
[IO.File]::WriteAllLines($OutputPath, $lines)
Write-Host "Wrote $OutputPath"
