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
$RepoRoot = Split-Path -Parent $Hz12Root
$Hz11Suite = Join-Path $RepoRoot "out_win_xowner"
$Hz11Build = Join-Path $RepoRoot "win\build_win_xowner_pipeline.ps1"
$Hz12Build = Join-Path $PSScriptRoot "build_hz12_windows_shadow.ps1"
$rows = @(
    @{ Name = "hz11-ownerless"; Path = (Join-Path $Hz11Suite "bench_xowner_hz11.exe"); Pattern = "\[XOWNER_PIPELINE\]" },
    @{ Name = "hz12-owner-inbox-l1"; Path = (Join-Path $Hz12Root "out_win_shadow\bench_hz12_xowner_inbox.exe"); Pattern = "\[HZ12_XOWNER_INBOX\]" },
    @{ Name = "tcmalloc"; Path = (Join-Path $Hz11Suite "bench_xowner_tcmalloc.exe"); Pattern = "\[XOWNER_PIPELINE\]" }
)

if (-not (Test-Path $rows[0].Path) -or -not (Test-Path $rows[2].Path)) {
    & $Hz11Build
    if ($LASTEXITCODE -ne 0) { throw "HZ11 xowner build failed: $LASTEXITCODE" }
}
if (-not (Test-Path $rows[1].Path)) {
    & $Hz12Build
    if ($LASTEXITCODE -ne 0) { throw "HZ12 xowner build failed: $LASTEXITCODE" }
}
if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path $Hz12Root "bench_results\windows_xowner_compare_l1_$stamp.md"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $OutputPath) | Out-Null

function Get-Median([double[]]$values) {
    $sorted = @($values | Sort-Object)
    $middle = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

$env:Path = $Hz11Suite + ";" + $env:Path
$results = @()
foreach ($row in $rows) {
    for ($run = 1; $run -le $Runs; $run++) {
        $output = & $row.Path $RuntimeSeconds $Producers $Consumers $RingCapacity $MinSize $MaxSize 2>&1
        $line = ($output | Select-String $row.Pattern).Line
        if (-not $line -or $line -notmatch 'ops/s=([0-9.]+)') {
            throw "Cannot parse xowner result for $($row.Name)"
        }
        $results += [pscustomobject]@{ Name = $row.Name; Run = $run; Ops = [double]$Matches[1] }
        Write-Host "[HZ12_XOWNER_COMPARE] allocator=$($row.Name) run=$run $line"
    }
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# HZ12 Windows Cross-Owner Compare L1")
$lines.Add("")
$lines.Add("- Runs: $Runs, runtime: ${RuntimeSeconds}s")
$lines.Add("- Producers/consumers: $Producers/$Consumers, ring: $RingCapacity")
$lines.Add("- Size: $MinSize..$MaxSize, cross-owner free rate: 100%, sharing factor: 2.0")
$lines.Add("")
$lines.Add("| Allocator | Median ops/s | Runs |")
$lines.Add("| --- | ---: | ---: |")
foreach ($group in ($results | Group-Object Name)) {
    $median = Get-Median @($group.Group.Ops)
    $lines.Add(("| {0} | {1:N3}M | {2} |" -f $group.Name, ($median / 1000000.0), $group.Count))
}
[IO.File]::WriteAllLines($OutputPath, $lines)
Write-Host "Wrote $OutputPath"
