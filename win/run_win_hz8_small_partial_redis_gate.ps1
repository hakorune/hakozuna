param(
    [ValidateRange(1, 30)]
    [int]$Runs = 5,
    [string]$OutputDir,
    [switch]$ForceBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_redis"
$BuildScript = Join-Path $PSScriptRoot "build_win_hz8_redis_r3_gate.ps1"
$Allocators = @(
    @{ Name = "hz8"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz8.exe") },
    @{ Name = "transition_only"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz8_small_partial_transition_only.exe") }
)
$Patterns = @("SET", "GET", "LPUSH", "LPOP", "RANDOM")

if (-not $OutputDir) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDir = Join-Path $RepoRoot "results\hz8-small-partial-redis-win\$stamp"
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null

$missing = @($Allocators | Where-Object { -not (Test-Path $_.Path) })
if ($ForceBuild -or $missing.Count -ne 0) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_hz8_redis_r3_gate.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Get-Median {
    param([double[]]$Values)
    $ordered = @($Values | Sort-Object)
    if ($ordered.Count -eq 0) { return [double]::NaN }
    $middle = [int][Math]::Floor($ordered.Count / 2)
    if (($ordered.Count % 2) -eq 1) { return [double]$ordered[$middle] }
    return ([double]$ordered[$middle - 1] + [double]$ordered[$middle]) / 2.0
}

function Invoke-RedisBench {
    param([string]$Path)

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $Path
    $psi.Arguments = "4 500 2000 16 256"
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
    $stderrTask = $proc.StandardError.ReadToEndAsync()
    [int64]$peakBytes = 0
    while (-not $proc.HasExited) {
        try {
            $proc.Refresh()
            $peakBytes = [Math]::Max($peakBytes, $proc.WorkingSet64)
            $peakBytes = [Math]::Max($peakBytes, $proc.PeakWorkingSet64)
        } catch {
        }
        Start-Sleep -Milliseconds 1
    }
    $proc.WaitForExit()
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    try {
        $proc.Refresh()
        $peakBytes = [Math]::Max($peakBytes, $proc.PeakWorkingSet64)
    } catch {
    }

    $values = @{}
    $currentPattern = $null
    foreach ($line in (($stdout + "`n" + $stderr) -split "`r?`n")) {
        $text = $line.Trim()
        if ($text -match '^Pattern:\s+(\S+)$') {
            $currentPattern = $Matches[1]
        } elseif ($text -match '^Throughput:\s+([0-9.]+)\s+M ops/sec$') {
            if (-not $currentPattern) { throw "throughput without pattern: $Path" }
            $values[$currentPattern] = [double]$Matches[1]
        }
    }
    if ($proc.ExitCode -ne 0 -or $values.Count -ne $Patterns.Count) {
        throw "Redis-like gate failed for $Path (rc=$($proc.ExitCode), patterns=$($values.Count))"
    }
    return [pscustomobject]@{
        Values = $values
        PeakKb = [int64][Math]::Ceiling($peakBytes / 1024.0)
        Raw = ($stdout + $stderr).Trim()
    }
}

$records = New-Object System.Collections.Generic.List[object]
$log = New-Object System.Collections.Generic.List[string]
for ($run = 1; $run -le $Runs; ++$run) {
    $order = if (($run % 2) -eq 1) { $Allocators } else { @($Allocators[1], $Allocators[0]) }
    foreach ($allocator in $order) {
        Write-Host ("[redis-like] run={0}/{1} allocator={2}" -f $run, $Runs, $allocator.Name)
        $result = Invoke-RedisBench -Path $allocator.Path
        foreach ($pattern in $Patterns) {
            $records.Add([pscustomobject]@{
                run = $run
                allocator = $allocator.Name
                pattern = $pattern
                ops_m = $result.Values[$pattern]
                peak_rss_kb = $result.PeakKb
            })
        }
        $log.Add("=== run=$run allocator=$($allocator.Name) ===")
        $log.Add($result.Raw)
        $log.Add("")
    }
}

$summary = New-Object System.Collections.Generic.List[object]
foreach ($pattern in $Patterns) {
    $defaultRows = @($records | Where-Object { $_.pattern -eq $pattern -and $_.allocator -eq "hz8" })
    $p1Rows = @($records | Where-Object { $_.pattern -eq $pattern -and $_.allocator -eq "transition_only" })
    $defaultOps = Get-Median ([double[]]@($defaultRows.ops_m))
    $p1Ops = Get-Median ([double[]]@($p1Rows.ops_m))
    $defaultPeak = Get-Median ([double[]]@($defaultRows.peak_rss_kb))
    $p1Peak = Get-Median ([double[]]@($p1Rows.peak_rss_kb))
    $summary.Add([pscustomobject]@{
        pattern = $pattern
        default_mops = $defaultOps
        p1_mops = $p1Ops
        delta_pct = (($p1Ops / $defaultOps) - 1.0) * 100.0
        default_peak_kb = $defaultPeak
        p1_peak_kb = $p1Peak
        peak_delta_pct = (($p1Peak / $defaultPeak) - 1.0) * 100.0
    })
}

$records | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $OutputDir "raw.csv")
$summary | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $OutputDir "summary.csv")
Set-Content -Encoding UTF8 -Path (Join-Path $OutputDir "runs.log") -Value $log

$markdown = New-Object System.Collections.Generic.List[string]
$markdown.Add("# HZ8 Small Partial Redis-Like Gate")
$markdown.Add("")
$markdown.Add("- Fresh-process AB/BA")
$markdown.Add("- Runs: $Runs")
$markdown.Add("- Diagnostic counters: disabled")
$markdown.Add("")
$markdown.Add("| pattern | default | P1 | delta | default peak | P1 peak | peak delta |")
$markdown.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
foreach ($item in $summary) {
    $markdown.Add(("| {0} | {1:N2}M | {2:N2}M | {3:+0.00;-0.00;0.00}% | {4:N2} MiB | {5:N2} MiB | {6:+0.00;-0.00;0.00}% |" -f
        $item.pattern, $item.default_mops, $item.p1_mops, $item.delta_pct,
        ($item.default_peak_kb / 1024.0), ($item.p1_peak_kb / 1024.0), $item.peak_delta_pct))
}
Set-Content -Encoding UTF8 -Path (Join-Path $OutputDir "summary.md") -Value $markdown

$summary | Format-Table -AutoSize
Write-Host "Wrote Redis-like gate: $OutputDir"
