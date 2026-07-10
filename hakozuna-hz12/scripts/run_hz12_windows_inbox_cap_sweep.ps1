param(
    [string]$OutputPath,
    [int[]]$InboxCaps = @(512, 1024, 2048),
    [int]$DrainInterval = 256,
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
$Build = Join-Path $PSScriptRoot "build_hz12_windows_shadow.ps1"
if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path $Hz12Root "bench_results\windows_xowner_inbox_cap_sweep_$stamp.md"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $OutputPath) | Out-Null

function Get-Median([double[]]$values) {
    $sorted = @($values | Sort-Object)
    $middle = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

$results = @()
foreach ($cap in $InboxCaps) {
    $name = "bench_hz12_xowner_inbox_cap$cap.exe"
    & $Build -InboxCap $cap -DrainInterval $DrainInterval -InboxOutputName $name
    if ($LASTEXITCODE -ne 0) { throw "HZ12 inbox cap build failed: $LASTEXITCODE" }
    $exe = Join-Path $Hz12Root "out_win_shadow\$name"
    for ($run = 1; $run -le $Runs; $run++) {
        $output = & $exe $RuntimeSeconds $Producers $Consumers $RingCapacity $MinSize $MaxSize 2>&1
        $pipe = ($output | Select-String '\[HZ12_XOWNER_INBOX\]').Line
        $inbox = ($output | Select-String '\[HZ12_OWNER_INBOX\]').Line
        if (-not $pipe -or -not $inbox -or $pipe -notmatch 'ops/s=([0-9.]+)') {
            throw "Cannot parse cap $cap run $run"
        }
        $ops = [double]$Matches[1]
        if ($pipe -notmatch 'peak_rss_mib=([0-9.]+).*rss_mib=([0-9.]+)') {
            throw "Cannot parse RSS cap $cap run $run"
        }
        $peak = [double]$Matches[1]
        $rss = [double]$Matches[2]
        if ($inbox -notmatch 'fallback_overflow=([0-9]+).*inbox_current_max=([0-9]+)') {
            throw "Cannot parse inbox cap $cap run $run"
        }
        $overflow = [double]$Matches[1]
        $inboxMax = [double]$Matches[2]
        $results += [pscustomobject]@{ Cap = $cap; Ops = $ops; Peak = $peak; Rss = $rss; Overflow = $overflow; InboxMax = $inboxMax }
        Write-Host "[HZ12_INBOX_CAP] cap=$cap run=$run $pipe"
        Write-Host $inbox
    }
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# HZ12 Windows Owner Inbox Cap Sweep")
$lines.Add("")
$lines.Add("- Runs: $Runs, runtime: ${RuntimeSeconds}s, drain interval: $DrainInterval")
$lines.Add("- Producers/consumers: $Producers/$Consumers, ring: $RingCapacity, size: $MinSize..$MaxSize")
$lines.Add("")
$lines.Add("| Inbox cap | Median ops/s | Peak RSS MiB | Final RSS MiB | Overflow objects | Inbox max |")
$lines.Add("| ---: | ---: | ---: | ---: | ---: | ---: |")
foreach ($group in ($results | Group-Object Cap | Sort-Object Name)) {
    $rows = $group.Group
    $lines.Add(("| {0} | {1:N3}M | {2:N2} | {3:N2} | {4:N0} | {5:N0} |" -f `
        $group.Name, ((Get-Median @($rows.Ops)) / 1000000.0),
        (Get-Median @($rows.Peak)), (Get-Median @($rows.Rss)),
        (Get-Median @($rows.Overflow)), (Get-Median @($rows.InboxMax))))
}
[IO.File]::WriteAllLines($OutputPath, $lines)
Write-Host "Wrote $OutputPath"
