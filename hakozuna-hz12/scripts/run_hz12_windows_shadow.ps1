param(
    [string]$OutputPath,
    [int]$Runs = 3,
    [int]$RuntimeSeconds = 5,
    [int]$Producers = 4,
    [int]$Consumers = 4,
    [int]$RingCapacity = 4096,
    [int]$MinSize = 8,
    [int]$MaxSize = 1024
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$Exe = Join-Path $Hz12Root "out_win_shadow\bench_hz12_xowner_shadow.exe"
$Build = Join-Path $PSScriptRoot "build_hz12_windows_shadow.ps1"
if (-not (Test-Path $Exe)) {
    & $Build
    if ($LASTEXITCODE -ne 0) { throw "HZ12 shadow build failed: $LASTEXITCODE" }
}
if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path $Hz12Root "bench_results\windows_xowner_shadow_$stamp.md"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $OutputPath) | Out-Null

function Get-Median([double[]]$values) {
    $sorted = @($values | Sort-Object)
    $middle = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

$ops = @()
$shadow = @()
for ($run = 1; $run -le $Runs; $run++) {
    $output = & $Exe $RuntimeSeconds $Producers $Consumers $RingCapacity $MinSize $MaxSize 2>&1
    $pipeline = ($output | Select-String '\[HZ12_XOWNER_SHADOW\]').Line
    $shadowLine = ($output | Select-String '\[HZ12_OWNER_SHADOW\]').Line
    if (-not $pipeline -or -not $shadowLine) { throw "HZ12 shadow output incomplete" }
    if ($pipeline -notmatch 'ops/s=([0-9.]+)') { throw "Cannot parse throughput: $pipeline" }
    $ops += [double]$Matches[1]
    $shadow += $shadowLine
    Write-Host "[HZ12_SHADOW_RUN] run=$run $pipeline"
    Write-Host $shadowLine
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# HZ12 Windows Owner Routing Shadow L0")
$lines.Add("")
$lines.Add("- Runs: $Runs, runtime: ${RuntimeSeconds}s")
$lines.Add("- Producers/consumers: $Producers/$Consumers, ring: $RingCapacity")
$lines.Add("- Size: $MinSize..$MaxSize")
$lines.Add("- Median diagnostic throughput: $([math]::Round((Get-Median $ops) / 1000000.0, 3))M ops/s")
$lines.Add("")
$lines.Add("## Raw Shadow Counters")
$lines.Add("")
foreach ($line in $shadow) { $lines.Add('```text'); $lines.Add($line); $lines.Add('```') }
[IO.File]::WriteAllLines($OutputPath, $lines)
Write-Host "Wrote $OutputPath"
