param(
    [string]$OutputDir,
    [int]$Runs = 10,
    [int]$TimeoutSeconds = 900,
    [string[]]$Allocators,
    [switch]$IncludeHz6Legacy,
    [switch]$IncludeHz8Research,
    [switch]$SkipHz7TinyRoute,
    [switch]$ContinueOnFailure
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_mt_remote"
$BuildScript = Join-Path $PSScriptRoot "build_win_mt_remote_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$LegacyExecutables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz4.exe") },
    @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz5_policy.exe") },
    @{ Name = "hz7-tinyroute"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz7.exe") },
    @{ Name = "hz7-v2-remote-natural"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz7_v2_remote_natural.exe") },
    @{ Name = "hz8-v2"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz8_v2.exe") },
    @{ Name = "hz8-reusable-span-mag16"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz8_reusable_span_mag16.exe") },
    @{ Name = "hz8-v3-adaptive-shadow"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz8_v3_adaptive_shadow.exe"); Hz8Research = $true },
    @{ Name = "hz8-reclaim-shadow"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz8_reclaim_shadow.exe"); Hz8Research = $true },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_tcmalloc.exe") }
)

$Hz6LegacyExecutables = @(
    @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_strict.exe") },
    @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_speed.exe") },
    @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_rss.exe") },
    @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_strict_broad.exe") },
    @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_speed_broad.exe") },
    @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_rss_broad.exe") },
    @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_strict_appcap.exe") },
    @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_speed_appcap.exe") },
    @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz6_rss_appcap.exe") }
)

$Executables = $LegacyExecutables
if (-not $IncludeHz8Research) {
    $Executables = @($Executables | Where-Object { -not $_.Hz8Research })
}
if ($SkipHz7TinyRoute) {
    $Executables = @($Executables | Where-Object { $_.Name -ne "hz7-tinyroute" })
}
if ($IncludeHz6Legacy) {
    $Executables = $LegacyExecutables + $Hz6LegacyExecutables
    if (-not $IncludeHz8Research) {
        $Executables = @($Executables | Where-Object { -not $_.Hz8Research })
    }
    if ($SkipHz7TinyRoute) {
        $Executables = @($Executables | Where-Object { $_.Name -ne "hz7-tinyroute" })
    }
}
if ($Allocators -and $Allocators.Count -gt 0) {
    $wanted = @($Allocators | ForEach-Object { $_ -split ',' } |
        ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $selected = @($Executables | Where-Object { $wanted -contains $_.Name })
    if ($selected.Count -ne $wanted.Count) {
        throw "Unknown or duplicate allocator in: $($wanted -join ', ')"
    }
    $Executables = $selected
}

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    if (($Executables | Where-Object { $_.Name -notlike "hz8-*" }).Count -eq 0) {
        & $BuildScript -OnlyHz8
    } else {
        & $BuildScript
    }
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_mt_remote_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

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

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [int]$TimeoutSeconds
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $quotedArgs = $Arguments | ForEach-Object {
        if ($_ -match '[\s"]') {
            '"' + ($_ -replace '"', '\"') + '"'
        } else {
            $_
        }
    }
    $psi.Arguments = ($quotedArgs -join ' ')

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $timedOut = $false
    $peakWorkingSetBytes = [Int64]0
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    try {
        $proc.Refresh()
        if ($proc.WorkingSet64 -gt $peakWorkingSetBytes) {
            $peakWorkingSetBytes = $proc.WorkingSet64
        }
        if ($proc.PeakWorkingSet64 -gt $peakWorkingSetBytes) {
            $peakWorkingSetBytes = $proc.PeakWorkingSet64
        }
    } catch {
    }

    while (-not $proc.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        try {
            $proc.Refresh()
            if ($proc.WorkingSet64 -gt $peakWorkingSetBytes) {
                $peakWorkingSetBytes = $proc.WorkingSet64
            }
            if ($proc.PeakWorkingSet64 -gt $peakWorkingSetBytes) {
                $peakWorkingSetBytes = $proc.PeakWorkingSet64
            }
        } catch {
        }
        Start-Sleep -Milliseconds 1
    }

    if (-not $proc.HasExited) {
        $timedOut = $true
        try {
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        } catch {
            # Best-effort timeout kill.
        }
        $proc.WaitForExit()
        return @{
            ExitCode = 124
            TimedOut = $true
            Lines = @("[TIMEOUT] exceeded ${TimeoutSeconds}s")
            PeakKb = "NA"
        }
    }

    try {
        $proc.Refresh()
        if ($proc.WorkingSet64 -gt $peakWorkingSetBytes) {
            $peakWorkingSetBytes = $proc.WorkingSet64
        }
        if ($proc.PeakWorkingSet64 -gt $peakWorkingSetBytes) {
            $peakWorkingSetBytes = $proc.PeakWorkingSet64
        }
    } catch {
    }

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

    return @{
        ExitCode = $proc.ExitCode
        TimedOut = $timedOut
        Lines = $lines
        PeakKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
    }
}

$Threads = 16
$Iters = 2000000
$WorkingSet = 400
$MinSize = 16
$MaxSize = 2048
$RemotePct = 90
$RingSlots = 65536

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_mt_remote_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_mt_remote_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Paper-Aligned Windows MT Remote")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L199)")
$Summary.Add("- [private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md](/C:/git/hakozuna-win/private/hakmem/docs/benchmarks/2026-01-25_MT_REMOTE_MATRIX_SSOT_WORK_ORDER.md)")
$Summary.Add("")
$Summary.Add("Windows native note:")
$Summary.Add('- benchmark: `bench_random_mixed_mt_remote_compare`')
$Summary.Add('- params: `threads=16 iters=2000000 ws=400 size=16..2048 remote_pct=90 ring_slots=65536`')
$Summary.Add(('- runs: `{0}`' -f $Runs))
$Summary.Add(('- timeout_seconds: `{0}`' -f $TimeoutSeconds))
$Summary.Add('- statistic: `median ops/s`')
$Summary.Add('- hz3 profile: `scale + S97-2 direct-map bucketize + skip_tail_null`')
$Summary.Add('- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt`, `hz4`, `hz5-policy`, `hz7-tinyroute`, and `hz7-v2-remote-natural`')
$Summary.Add('- hz7 note: `hz7-tinyroute` is a direct-API coarse-lock safety baseline here, not a lock-free remote allocator.')
$Summary.Add('- hz7-v2 note: `hz7-v2-remote-natural` enables `H7_REMOTE_NATURAL_PRESET`, widening the route table for bounded cross-thread pressure without owner inboxes or lock-free remote queues.')
if ($SkipHz7TinyRoute) {
    $Summary.Add('- hz7 skip note: `hz7-tinyroute` was skipped for this run; use it only as a legacy control row.')
}
if (-not $IncludeHz6Legacy) {
    $Summary.Add('- hz6 note: HZ6 rows are skipped by default because this legacy benchmark frees cross-thread pointers through per-thread allocator instances; use `-IncludeHz6Legacy` only for debugging the mismatch, and use the HZ6 standalone remote/reuse runner for HZ6 contract numbers.')
}
$Summary.Add("")
$Summary.Add("| allocator | median ops/s | median actual remote % | median fallback % | median peak_kb | runs |")
$Summary.Add("| --- | ---: | ---: | ---: | ---: | --- |")

foreach ($exe in $Executables) {
    $opsRuns = New-Object System.Collections.Generic.List[double]
    $actualRuns = New-Object System.Collections.Generic.List[double]
    $fallbackRuns = New-Object System.Collections.Generic.List[double]
    $peakRuns = New-Object System.Collections.Generic.List[double]
    $runTexts = New-Object System.Collections.Generic.List[string]

    for ($run = 1; $run -le $Runs; $run++) {
        $args = @(
            [string]$Threads,
            [string]$Iters,
            [string]$WorkingSet,
            [string]$MinSize,
            [string]$MaxSize,
            [string]$RemotePct,
            [string]$RingSlots
        )
        $result = Invoke-CapturedProcess -FilePath $exe.Path -Arguments $args -TimeoutSeconds $TimeoutSeconds
        $output = $result.Lines
        $rc = $result.ExitCode
        $raw = (($output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
        if (-not $raw) {
            $raw = "(no output)"
        }

        $RawLines.Add("=== " + $exe.Name + " / run " + $run + " ===")
        $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
        $RawLines.Add("rc: " + $rc)
        if ($result.TimedOut) {
            $RawLines.Add("timeout: " + $TimeoutSeconds)
        }
        foreach ($line in $output) {
            $RawLines.Add($line.ToString())
        }
        $RawLines.Add("")

        if ($rc -ne 0) {
            if ($result.TimedOut) {
                if ($ContinueOnFailure) {
                    $runTexts.Add(("failed:timeout{0}" -f $TimeoutSeconds))
                    continue
                }
                throw "MT remote runner allocator $($exe.Name) timed out after $TimeoutSeconds seconds"
            }
            if ($ContinueOnFailure) {
                $runTexts.Add(("failed:rc{0}" -f $rc))
                continue
            }
            throw "MT remote runner allocator $($exe.Name) failed with exit code $rc"
        }
        if ($raw -notmatch "ops/s=([0-9.]+)") {
            throw "Could not parse ops/s for allocator $($exe.Name)"
        }
        $ops = [double]$Matches[1]
        $opsRuns.Add($ops)

        $actual = [double]::NaN
        $fallback = [double]::NaN
        if ($raw -match "actual=([0-9.]+)") {
            $actual = [double]$Matches[1]
            $actualRuns.Add($actual)
        }
        if ($raw -match "fallback_rate=([0-9.]+)") {
            $fallback = [double]$Matches[1]
            $fallbackRuns.Add($fallback)
        }
        if ($result.PeakKb -match '^[0-9]+$') {
            $peakRuns.Add([double]$result.PeakKb)
        }
        $runTexts.Add(("{0:N3}M / actual {1:N2}% / fallback {2:N2}% / {3} KB" -f ($ops / 1000000.0), $actual, $fallback, $result.PeakKb))
    }

    if ($opsRuns.Count -eq 0) {
        $Summary.Add(('| {0} | failed | n/a | n/a | n/a | `{1}` |' -f $exe.Name, ($runTexts -join ", ")))
        continue
    }

    $medianOps = Get-Median -Values $opsRuns.ToArray()
    $medianActual = Get-Median -Values $actualRuns.ToArray()
    $medianFallback = Get-Median -Values $fallbackRuns.ToArray()
    $medianPeak = if ($peakRuns.Count -gt 0) { Get-Median -Values $peakRuns.ToArray() } else { [double]::NaN }
    $Summary.Add(('| {0} | {1:N3}M | {2:N2} | {3:N2} | {4:N0} | `{5}` |' -f $exe.Name, ($medianOps / 1000000.0), $medianActual, $medianFallback, $medianPeak, ($runTexts -join ", ")))
}

$Summary.Add("")
$Summary.Add("Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
