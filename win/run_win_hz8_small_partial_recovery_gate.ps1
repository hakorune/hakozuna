param(
    [ValidateRange(1, 30)]
    [int]$Runs = 5,
    [ValidateRange(1, 100)]
    [int]$WorkScale = 1,
    [ValidateSet("lcg", "xorshift", "both")]
    [string]$Trace = "both",
    [string]$OutputDir,
    [switch]$ForceBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"

$Allocators = @(
    @{ Name = "hz8"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz8.exe") },
    @{ Name = "original_depot"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz8_small_partial_depot.exe") },
    @{ Name = "transition_only"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz8_small_partial_transition_only.exe") },
    @{ Name = "tier_membership"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz8_small_tier_membership.exe") }
)

if (-not $OutputDir) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDir = Join-Path $RepoRoot "results\hz8-small-partial-recovery-win\$stamp"
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null

$missing = @($Allocators | Where-Object { -not (Test-Path $_.Path) })
if ($ForceBuild -or $missing.Count -ne 0) {
    & $BuildScript -OnlyHz8
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

$Rows = @(
    @{ Name = "fixed8k"; Threads = 4; Iters = 2000000; WorkingSet = 256; Min = 8192; Max = 8192 },
    @{ Name = "fixed16k"; Threads = 4; Iters = 1200000; WorkingSet = 256; Min = 16384; Max = 16384 },
    @{ Name = "fixed32k"; Threads = 4; Iters = 800000; WorkingSet = 256; Min = 32768; Max = 32768 },
    @{ Name = "balanced"; Threads = 8; Iters = 250000; WorkingSet = 4096; Min = 16; Max = 2048 },
    @{ Name = "wide_ws"; Threads = 8; Iters = 200000; WorkingSet = 16384; Min = 16; Max = 1024 },
    @{ Name = "larger_sizes"; Threads = 4; Iters = 150000; WorkingSet = 4096; Min = 256; Max = 8192 }
)

function Get-Median {
    param([double[]]$Values)
    $ordered = @($Values | Sort-Object)
    if ($ordered.Count -eq 0) { return [double]::NaN }
    $middle = [int][Math]::Floor($ordered.Count / 2)
    if (($ordered.Count % 2) -eq 1) { return [double]$ordered[$middle] }
    return ([double]$ordered[$middle - 1] + [double]$ordered[$middle]) / 2.0
}

function Invoke-BenchProcess {
    param([string]$Path, [string[]]$BenchArgs)

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $Path
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = ($BenchArgs -join " ")

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
    $stderrTask = $proc.StandardError.ReadToEndAsync()
    $peakBytes = [Int64]0
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

    $raw = (($stdout + " " + $stderr) -replace '[\r\n]+', ' ').Trim()
    $ops = [double]::NaN
    $allocFail = -1
    if ($raw -match 'ops/s=([0-9.]+)') { $ops = [double]$Matches[1] }
    if ($raw -match 'alloc_fail=([0-9]+)') { $allocFail = [int64]$Matches[1] }
    return [pscustomobject]@{
        ExitCode = $proc.ExitCode
        Ops = $ops
        AllocFail = $allocFail
        PeakKb = [int64][Math]::Ceiling($peakBytes / 1024.0)
        Raw = $raw
    }
}

$TraceNames = if ($Trace -eq "both") { @("lcg", "xorshift") } else { @($Trace) }
$Manifest = @(
    "runs=$Runs",
    "work_scale=$WorkScale",
    "trace=$Trace",
    "default_flags=HZ8 Windows default",
    "original_flags=H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1",
    "p1_flags=H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1,H8_SMALL_PARTIAL_TRANSITION_ONLY_L1B",
    "membership_flags=H8_SMALL_PARTIAL_TRANSITION_DEPOT_L1,H8_SMALL_PARTIAL_TRANSITION_ONLY_L1B,H8_SMALL_TIER_MEMBERSHIP_L1",
    "diagnostic_counters=disabled"
)
Set-Content -Encoding UTF8 -Path (Join-Path $OutputDir "manifest.txt") -Value $Manifest

foreach ($traceName in $TraceNames) {
    $traceMode = if ($traceName -eq "xorshift") { 1 } else { 0 }
    $TraceDir = Join-Path $OutputDir $traceName
    New-Item -ItemType Directory -Force $TraceDir | Out-Null
    $Records = New-Object System.Collections.Generic.List[object]
    $Log = New-Object System.Collections.Generic.List[string]

    foreach ($row in $Rows) {
        for ($run = 1; $run -le $Runs; ++$run) {
            $offset = ($run - 1) % $Allocators.Count
            $order = @()
            for ($i = 0; $i -lt $Allocators.Count; ++$i) {
                $order += $Allocators[($offset + $i) % $Allocators.Count]
            }
            foreach ($allocator in $order) {
                $benchArgs = @(
                    [string]$row.Threads,
                    [string]([int64]$row.Iters * $WorkScale),
                    [string]$row.WorkingSet,
                    [string]$row.Min,
                    [string]$row.Max,
                    [string]$traceMode
                )
                Write-Host ("[{0}/{1}] run={2}/{3} allocator={4}" -f $traceName, $row.Name, $run, $Runs, $allocator.Name)
                $result = Invoke-BenchProcess -Path $allocator.Path -BenchArgs $benchArgs
                $Records.Add([pscustomobject]@{
                    trace = $traceName
                    row = $row.Name
                    run = $run
                    allocator = $allocator.Name
                    order_offset = $offset
                    ops_per_sec = $result.Ops
                    peak_rss_kb = $result.PeakKb
                    alloc_fail = $result.AllocFail
                    exit_code = $result.ExitCode
                })
                $Log.Add(("=== {0}/{1} run={2} allocator={3} ===" -f $traceName, $row.Name, $run, $allocator.Name))
                $Log.Add(("cmd: {0} {1}" -f $allocator.Path, ($benchArgs -join " ")))
                $Log.Add($result.Raw)
                $Log.Add("")
                if ($result.ExitCode -ne 0 -or [double]::IsNaN($result.Ops) -or $result.AllocFail -ne 0) {
                    throw ("Gate failed: trace={0} row={1} run={2} allocator={3} rc={4} alloc_fail={5}" -f $traceName, $row.Name, $run, $allocator.Name, $result.ExitCode, $result.AllocFail)
                }
            }
        }
    }

    $Records | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $TraceDir "raw.csv")
    Set-Content -Encoding UTF8 -Path (Join-Path $TraceDir "runs.log") -Value $Log

    $Summary = New-Object System.Collections.Generic.List[object]
    foreach ($row in $Rows) {
        $medians = @{}
        foreach ($allocator in $Allocators) {
            $samples = @($Records | Where-Object { $_.row -eq $row.Name -and $_.allocator -eq $allocator.Name })
            $medians[$allocator.Name] = [pscustomobject]@{
                Ops = Get-Median ([double[]]@($samples.ops_per_sec))
                Peak = Get-Median ([double[]]@($samples.peak_rss_kb))
            }
        }
        $default = $medians["hz8"]
        $original = $medians["original_depot"]
        $p1 = $medians["transition_only"]
        $membership = $medians["tier_membership"]
        $Summary.Add([pscustomobject]@{
            row = $row.Name
            default_ops = $default.Ops
            original_ops = $original.Ops
            p1_ops = $p1.Ops
            membership_ops = $membership.Ops
            original_vs_default_pct = (($original.Ops / $default.Ops) - 1.0) * 100.0
            p1_vs_default_pct = (($p1.Ops / $default.Ops) - 1.0) * 100.0
            p1_vs_original_pct = (($p1.Ops / $original.Ops) - 1.0) * 100.0
            membership_vs_default_pct = (($membership.Ops / $default.Ops) - 1.0) * 100.0
            membership_vs_p1_pct = (($membership.Ops / $p1.Ops) - 1.0) * 100.0
            default_peak_kb = $default.Peak
            original_peak_kb = $original.Peak
            p1_peak_kb = $p1.Peak
            membership_peak_kb = $membership.Peak
        })
    }
    $Summary | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $TraceDir "summary.csv")

    $Markdown = New-Object System.Collections.Generic.List[string]
    $Markdown.Add("# HZ8 Small Partial Recovery Windows Gate")
    $Markdown.Add("")
    $Markdown.Add("- Trace: $traceName")
    $Markdown.Add("- Fresh-process ABC/BCA/CAB rotation")
    $Markdown.Add("- Runs: $Runs")
    $Markdown.Add("- Work scale: ${WorkScale}x")
    $Markdown.Add("- Diagnostic counters: disabled")
    $Markdown.Add("")
    $Markdown.Add("| row | default | original | P1 | membership | original/default | P1/default | membership/default | membership/P1 | default peak | original peak | P1 peak | membership peak |")
    $Markdown.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
    foreach ($item in $Summary) {
        $Markdown.Add(("| {0} | {1:N3}M | {2:N3}M | {3:N3}M | {4:N3}M | {5:+0.00;-0.00;0.00}% | {6:+0.00;-0.00;0.00}% | {7:+0.00;-0.00;0.00}% | {8:+0.00;-0.00;0.00}% | {9:N2} MiB | {10:N2} MiB | {11:N2} MiB | {12:N2} MiB |" -f
            $item.row, ($item.default_ops / 1e6), ($item.original_ops / 1e6), ($item.p1_ops / 1e6), ($item.membership_ops / 1e6),
            $item.original_vs_default_pct, $item.p1_vs_default_pct,
            $item.membership_vs_default_pct, $item.membership_vs_p1_pct,
            ($item.default_peak_kb / 1024.0), ($item.original_peak_kb / 1024.0),
            ($item.p1_peak_kb / 1024.0), ($item.membership_peak_kb / 1024.0)))
    }
    Set-Content -Encoding UTF8 -Path (Join-Path $TraceDir "summary.md") -Value $Markdown
    $Summary | Format-Table -AutoSize
}

Write-Host "Wrote recovery results: $OutputDir"
