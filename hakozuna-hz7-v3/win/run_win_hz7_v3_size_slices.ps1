param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 2000000,
    [int]$DirectRetainCap = 0,
    [int]$SpanClassMax = 0
)

$ErrorActionPreference = "Stop"

$Hz7Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz7Root
$HotpathScript = Join-Path $PSScriptRoot "run_win_hz7_v3_hotpath.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_hz7_v3_size_slices"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Length -eq 0) {
        return [double]::NaN
    }
    $sorted = $Values | Sort-Object
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Format-Rate {
    param([double]$Value)
    if ([double]::IsNaN($Value)) {
        return "NaN"
    }
    if ($Value -ge 1000000.0) {
        return ("{0:N3}M" -f ($Value / 1000000.0))
    }
    if ($Value -ge 1000.0) {
        return ("{0:N3}K" -f ($Value / 1000.0))
    }
    return ("{0:N2}" -f $Value)
}

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = ($Arguments | ForEach-Object {
        if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }) -join ' '

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($chunk in @($stdout, $stderr)) {
        if (-not [string]::IsNullOrEmpty($chunk)) {
            foreach ($line in ($chunk -split "`r?`n")) {
                if ($line -ne "") {
                    $lines.Add($line)
                }
            }
        }
    }

    return @{ ExitCode = $proc.ExitCode; Lines = $lines }
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$RawDir = Join-Path $OutputDir "raw"
New-Item -ItemType Directory -Force $RawDir | Out-Null

$hotpathArgs = @(
    "-Runs", [string]$Runs,
    "-Iters", [string]$Iters,
    "-OutputDir", $RawDir
)
if ($DirectRetainCap -gt 0) {
    $hotpathArgs += @("-DirectRetainCap", [string]$DirectRetainCap)
}
if ($SpanClassMax -gt 0) {
    $hotpathArgs += @("-SpanClassMax", [string]$SpanClassMax)
}

$result = Invoke-CapturedProcess -FilePath "powershell" -Arguments @(
    "-ExecutionPolicy", "Bypass",
    "-File", $HotpathScript
) + $hotpathArgs
if ($result.ExitCode -ne 0) {
    throw "hotpath runner failed with exit code $($result.ExitCode)"
}

$summaryPath = $null
foreach ($line in $result.Lines) {
    if ($line -match '^Wrote summary:\s+(.*)$') {
        $summaryPath = $Matches[1].Trim()
    }
}
if (-not $summaryPath -or -not (Test-Path $summaryPath)) {
    throw "hotpath summary not found"
}

$summaryLines = Get-Content $summaryPath
$rows = @{}
foreach ($line in $summaryLines) {
    if ($line -notmatch '^\|\s*(?<op>[^|]+)\s*\|\s*(?<label>[^|]+)\s*\|\s*(?<size>[^|]+)\s*\|\s*(?<rate>[^|]+)\s*\|\s*(?<unit>[^|]+)\s*\|\s*(?<rss>[^|]+)\s*\|$') {
        continue
    }
    $op = $Matches.op.Trim()
    $label = $Matches.label.Trim()
    $size = $Matches.size.Trim()
    $rate = $Matches.rate.Trim()
    $unit = $Matches.unit.Trim()
    $rss = $Matches.rss.Trim()
    $key = "${op}:${label}"
    if (-not $rows.ContainsKey($key)) {
        $rows[$key] = @{
            Op = $op
            Label = $label
            Size = $size
            Unit = $unit
            Rates = New-Object System.Collections.Generic.List[double]
            Rss = New-Object System.Collections.Generic.List[double]
        }
    }
    if ($rate -match '([0-9.]+)([MK]?)') {
        $rateValue = [double]$Matches[1]
        switch ($Matches[2]) {
            'M' { $rateValue *= 1000000.0 }
            'K' { $rateValue *= 1000.0 }
        }
        $rows[$key].Rates.Add($rateValue)
    }
    if ($rss -match '([0-9.]+)') {
        $rows[$key].Rss.Add([double]$Matches[1])
    }
}

$wanted = @(
    'malloc_free:span4k',
    'malloc_free:span8k',
    'malloc_free:span16k',
    'route_invariant:span4k',
    'route_invariant:span8k',
    'route_invariant:span16k',
    'route_valid:span4k',
    'route_valid:span8k',
    'route_valid:span16k',
    'route_invalid:span4k',
    'route_invalid:span8k',
    'route_invalid:span16k',
    'malloc_batch:span4k',
    'malloc_batch:span8k',
    'malloc_batch:span16k',
    'free_batch:span4k',
    'free_batch:span8k',
    'free_batch:span16k',
    'free_retained_loop:span4k',
    'free_retained_loop:span8k',
    'free_retained_loop:span16k'
)

$Report = New-Object System.Collections.Generic.List[string]
$Report.Add("# HZ7 v3 Windows Size Slices")
$Report.Add("")
$Report.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')")
$Report.Add("")
$Report.Add('- benchmark: `bench_hz7_v3_hotpath`')
$Report.Add('- allocator: `hz7-v3`')
$Report.Add("- runs: $Runs")
$Report.Add("- iters_per_run: $Iters")
if ($DirectRetainCap -gt 0) {
    $Report.Add("- direct_retain_cap: $DirectRetainCap")
}
if ($SpanClassMax -gt 0) {
    $Report.Add("- span_class_max: $SpanClassMax")
}
$Report.Add('- note: filtered from the v3 hotpath probe to emphasize 4K/8K/16K span-audit rows')
$Report.Add("")
$Report.Add("| op | label | size | median rate | rate unit | median peak_kb |")
$Report.Add("| --- | --- | ---: | ---: | --- | ---: |")

foreach ($key in $wanted) {
    if (-not $rows.ContainsKey($key)) {
        continue
    }
    $row = $rows[$key]
    $medianRate = Get-Median $row.Rates.ToArray()
    $medianRss = Get-Median $row.Rss.ToArray()
    $Report.Add("| $($row.Op) | $($row.Label) | $($row.Size) | $(Format-Rate $medianRate) | $($row.Unit) | $([int]$medianRss) |")
}

$Report.Add("")
$Report.Add("Artifacts: $OutputDir")

$summaryOut = Join-Path $OutputDir ($Stamp + "_hz7_v3_size_slices_windows.md")
$rawOut = Join-Path $OutputDir ($Stamp + "_hz7_v3_size_slices_windows.log")
Set-Content -Path $summaryOut -Value $Report -Encoding UTF8
Set-Content -Path $rawOut -Value ($result.Lines -join "`r`n") -Encoding UTF8
Write-Host "Wrote summary: $summaryOut"
