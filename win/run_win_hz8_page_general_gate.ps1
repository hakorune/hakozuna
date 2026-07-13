param(
    [int]$Runs = 5,
    [ValidateRange(1, 100)]
    [int]$WorkScale = 1,
    [string]$OutputDir,
    [ValidateSet("v2", "general", "default", "tcmalloc")]
    [string]$Baseline = "v2",
    [ValidateSet("general", "cap128", "entry-boundary", "default", "partial-depot")]
    [string]$Candidate = "general",
    [switch]$ForceBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"
$BaselineName = switch ($Baseline) {
    "general" { "hz8-r3-page-general" }
    "default" { "hz8" }
    "tcmalloc" { "tcmalloc" }
    default { "hz8-v2" }
}
$BaselineFile = switch ($Baseline) {
    "general" { "bench_mixed_ws_hz8_medium_page_general.exe" }
    "default" { "bench_mixed_ws_hz8.exe" }
    "tcmalloc" { "bench_mixed_ws_tcmalloc.exe" }
    default { "bench_mixed_ws_hz8_v2.exe" }
}
$BaselinePath = Join-Path $SuiteDir $BaselineFile
$CandidateName = switch ($Candidate) {
    "default" { "hz8" }
    "partial-depot" { "hz8-small-partial-depot" }
    "cap128" { "hz8-r3-page-general-cap128" }
    "entry-boundary" { "hz8-r3-page-general-entry-boundary" }
    default { "hz8-r3-page-general" }
}
$CandidateFile = switch ($Candidate) {
    "default" { "bench_mixed_ws_hz8.exe" }
    "partial-depot" { "bench_mixed_ws_hz8_small_partial_depot.exe" }
    "cap128" { "bench_mixed_ws_hz8_medium_page_general_cap128.exe" }
    "entry-boundary" { "bench_mixed_ws_hz8_medium_page_general_entry_boundary.exe" }
    default { "bench_mixed_ws_hz8_medium_page_general.exe" }
}
$CandidatePath = Join-Path $SuiteDir $CandidateFile

if (-not $OutputDir) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDir = Join-Path $RepoRoot "results\hz8-page-general-gate\$stamp"
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null

if ($ForceBuild -or -not (Test-Path $BaselinePath) -or -not (Test-Path $CandidatePath)) {
    & $BuildScript
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

$Allocators = @(
    @{ Name = $BaselineName; Path = $BaselinePath },
    @{ Name = $CandidateName; Path = $CandidatePath }
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
    param(
        [string]$Path,
        [string[]]$BenchArgs
    )

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
    $peakKb = [int64][Math]::Ceiling($peakBytes / 1024.0)
    if ($raw -match 'peak_kb=([0-9]+)') { $peakKb = [int64]$Matches[1] }

    return [pscustomobject]@{
        ExitCode = $proc.ExitCode
        Ops = $ops
        AllocFail = $allocFail
        PeakKb = $peakKb
        Raw = $raw
    }
}

$Records = New-Object System.Collections.Generic.List[object]
$Log = New-Object System.Collections.Generic.List[string]

foreach ($row in $Rows) {
    for ($run = 1; $run -le $Runs; $run++) {
        $order = if (($run % 2) -eq 1) { $Allocators } else { @($Allocators[1], $Allocators[0]) }
        foreach ($allocator in $order) {
            $benchArgs = @(
                [string]$row.Threads,
                [string]([int64]$row.Iters * $WorkScale),
                [string]$row.WorkingSet,
                [string]$row.Min,
                [string]$row.Max
            )
            Write-Host ("[{0}] run={1}/{2} allocator={3}" -f $row.Name, $run, $Runs, $allocator.Name)
            $result = Invoke-BenchProcess -Path $allocator.Path -BenchArgs $benchArgs
            $Records.Add([pscustomobject]@{
                row = $row.Name
                run = $run
                allocator = $allocator.Name
                order = if (($run % 2) -eq 1) { "AB" } else { "BA" }
                ops_per_sec = $result.Ops
                peak_rss_kb = $result.PeakKb
                alloc_fail = $result.AllocFail
                exit_code = $result.ExitCode
            })
            $Log.Add(("=== {0} run={1} allocator={2} ===" -f $row.Name, $run, $allocator.Name))
            $Log.Add(("cmd: {0} {1}" -f $allocator.Path, ($benchArgs -join " ")))
            $Log.Add(("rc: {0}" -f $result.ExitCode))
            $Log.Add($result.Raw)
            $Log.Add("")
            $allocFailInvalid = ($allocator.Name -ne "tcmalloc" -and $result.AllocFail -ne 0)
            if ($result.ExitCode -ne 0 -or [double]::IsNaN($result.Ops) -or $allocFailInvalid) {
                throw ("Gate run failed: row={0} run={1} allocator={2} rc={3} alloc_fail={4}" -f $row.Name, $run, $allocator.Name, $result.ExitCode, $result.AllocFail)
            }
        }
    }
}

$Records | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $OutputDir "raw.csv")
Set-Content -Encoding UTF8 -Path (Join-Path $OutputDir "runs.log") -Value $Log

$SummaryRows = New-Object System.Collections.Generic.List[object]
foreach ($row in $Rows) {
    $base = @($Records | Where-Object { $_.row -eq $row.Name -and $_.allocator -eq $BaselineName })
    $candidateRecords = @($Records | Where-Object { $_.row -eq $row.Name -and $_.allocator -eq $CandidateName })
    $baseOps = Get-Median ([double[]]@($base.ops_per_sec))
    $candidateOps = Get-Median ([double[]]@($candidateRecords.ops_per_sec))
    $baseRss = Get-Median ([double[]]@($base.peak_rss_kb))
    $candidateRss = Get-Median ([double[]]@($candidateRecords.peak_rss_kb))
    $SummaryRows.Add([pscustomobject]@{
        row = $row.Name
        baseline_ops = $baseOps
        candidate_ops = $candidateOps
        throughput_delta_pct = (($candidateOps / $baseOps) - 1.0) * 100.0
        baseline_peak_rss_kb = $baseRss
        candidate_peak_rss_kb = $candidateRss
        peak_rss_delta_kb = $candidateRss - $baseRss
        peak_rss_delta_pct = (($candidateRss / $baseRss) - 1.0) * 100.0
    })
}
$SummaryRows | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $OutputDir "summary.csv")

$Markdown = New-Object System.Collections.Generic.List[string]
$Markdown.Add("# HZ8 General Medium Page Windows Gate")
$Markdown.Add("")
$Markdown.Add("- Fresh process AB/BA rotation")
$Markdown.Add("- Runs: $Runs")
$Markdown.Add("- Work scale: ${WorkScale}x")
$Markdown.Add(('- Baseline: `{0}`' -f $BaselineName))
$Markdown.Add(('- Candidate: `{0}`' -f $CandidateName))
$Markdown.Add("- Diagnostic counters: disabled")
$Markdown.Add("")
$Markdown.Add(("| row | {0} ops/s | {1} ops/s | delta | {0} peak RSS | {1} peak RSS | RSS delta |" -f $BaselineName, $CandidateName))
$Markdown.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
foreach ($item in $SummaryRows) {
    $Markdown.Add(("| {0} | {1:N3}M | {2:N3}M | {3:+0.00;-0.00;0.00}% | {4:N2} MiB | {5:N2} MiB | {6:+0.00;-0.00;0.00}% |" -f
        $item.row,
        ($item.baseline_ops / 1000000.0),
        ($item.candidate_ops / 1000000.0),
        $item.throughput_delta_pct,
        ($item.baseline_peak_rss_kb / 1024.0),
        ($item.candidate_peak_rss_kb / 1024.0),
        $item.peak_rss_delta_pct))
}
$Markdown.Add("")
if ($Baseline -eq "tcmalloc") {
    $Markdown.Add("Comparison only: throughput-first tcmalloc is the reference; HZ8 keeps its fail-closed and low-RSS contract. HZ8 runs require alloc_fail=0; tcmalloc does not expose that field in this harness.")
} else {
    $Markdown.Add("Acceptance guide: exact medium rows should improve materially; control rows must remain within -3%; peak RSS must remain within max(+5%, +1 MiB); all runs require alloc_fail=0.")
}
Set-Content -Encoding UTF8 -Path (Join-Path $OutputDir "summary.md") -Value $Markdown

Write-Host "Wrote gate results: $OutputDir"
$SummaryRows | Format-Table -AutoSize
