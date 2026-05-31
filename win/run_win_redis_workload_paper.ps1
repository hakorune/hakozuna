param(
    [string]$OutputDir,
    [int]$Runs = 5,
    [int]$TimeoutSeconds = 60,
    [switch]$IncludeHz6CapacityControls,
    [switch]$ContinueOnFailure
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_redis"
$BuildScript = Join-Path $PSScriptRoot "build_win_redis_workload_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_redis_workload_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz4.exe") },
    @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz5_policy.exe") },
    @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_strict_appcap.exe") },
    @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_speed_appcap.exe") },
    @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_rss_appcap.exe") },
    @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_strict_route4k.exe") },
    @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_speed_route4k.exe") },
    @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_rss_route4k.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_redis_workload_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_redis_workload_tcmalloc.exe") }
)

if ($IncludeHz6CapacityControls) {
    $Executables = @(
        @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_redis_workload_crt.exe") },
        @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz3.exe") },
        @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz4.exe") },
        @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz5_policy.exe") },
        @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_strict.exe") },
        @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_speed.exe") },
        @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_rss.exe") },
        @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_strict_broad.exe") },
        @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_speed_broad.exe") },
        @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_rss_broad.exe") },
        @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_strict_route4k.exe") },
        @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_speed_route4k.exe") },
        @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_rss_route4k.exe") },
        @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_strict_appcap.exe") },
        @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_speed_appcap.exe") },
        @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_redis_workload_hz6_rss_appcap.exe") },
        @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_redis_workload_mimalloc.exe") },
        @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_redis_workload_tcmalloc.exe") }
    )
}

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_redis_workload_suite.ps1 failed with exit code $LASTEXITCODE"
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
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [int]$TimeoutSeconds = 60
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
    $peakWorkingSetBytes = [Int64]0
    $timedOut = $false
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
            $proc.Kill($true)
        } catch {
            try { $proc.Kill() } catch {}
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
                if ($line -ne "") { $lines.Add($line) }
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

$Threads = 4
$Cycles = 500
$Ops = 2000
$MinSize = 16
$MaxSize = 256
$Patterns = @("SET", "GET", "LPUSH", "LPOP", "RANDOM")

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_redis_workload_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_redis_workload_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Paper-Aligned Windows Redis Workload")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L86)")
$Summary.Add("- [private/hakmem/benchmarks/redis/workload_bench_fixed.c](/C:/git/hakozuna-win/private/hakmem/benchmarks/redis/workload_bench_fixed.c)")
$Summary.Add("")
$Summary.Add("Windows native note:")
$Summary.Add('- benchmark: `bench_redis_workload_compare`')
$Summary.Add('- params: `threads=4 cycles=500 ops=2000 size=16..256`')
$Summary.Add(('- runs: `{0}`' -f $Runs))
$Summary.Add(('- per-run timeout: `{0}s`' -f $TimeoutSeconds))
$Summary.Add('- statistic: `median M ops/sec` per pattern')
$Summary.Add('- note: paper originally compares `hz3` and `tcmalloc`; this runner also records `crt`, `hz4`, and `mimalloc`')
$Summary.Add("")

$PatternResults = @{}
$PatternPeakResults = @{}
$Failures = New-Object System.Collections.Generic.List[string]
foreach ($pattern in $Patterns) {
    $PatternResults[$pattern] = @{}
    $PatternPeakResults[$pattern] = @{}
    foreach ($exe in $Executables) {
        $PatternResults[$pattern][$exe.Name] = New-Object System.Collections.Generic.List[double]
        $PatternPeakResults[$pattern][$exe.Name] = New-Object System.Collections.Generic.List[double]
    }
}

foreach ($exe in $Executables) {
    for ($run = 1; $run -le $Runs; $run++) {
        $args = @(
            [string]$Threads,
            [string]$Cycles,
            [string]$Ops,
            [string]$MinSize,
            [string]$MaxSize
        )
        $captured = Invoke-CapturedProcess -FilePath $exe.Path -Arguments $args -TimeoutSeconds $TimeoutSeconds
        $output = $captured.Lines
        $rc = [int]$captured.ExitCode

        $RawLines.Add("=== " + $exe.Name + " / run " + $run + " ===")
        $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
        $RawLines.Add("rc: " + $rc)
        foreach ($line in $output) {
            $RawLines.Add($line.ToString())
        }
        $RawLines.Add("")

        if ($rc -ne 0) {
            $failure = "Redis workload runner allocator $($exe.Name) run $run failed with exit code $rc"
            $Failures.Add($failure) | Out-Null
            if ($ContinueOnFailure) {
                continue
            }
            throw $failure
        }

        $currentPattern = $null
        foreach ($line in $output) {
            $text = $line.ToString().Trim()
            if ($text -match "^Pattern:\s+(\S+)$") {
                $currentPattern = $Matches[1]
                continue
            }
            if ($text -match "^Throughput:\s+([0-9.]+)\s+M ops/sec$") {
                if (-not $currentPattern) {
                    throw "Throughput line without pattern for allocator $($exe.Name)"
                }
                if (-not $PatternResults.ContainsKey($currentPattern)) {
                    throw "Unknown pattern parsed: $currentPattern"
                }
                $PatternResults[$currentPattern][$exe.Name].Add([double]$Matches[1])
                if ($captured.PeakKb -match '^[0-9]+$') {
                    $PatternPeakResults[$currentPattern][$exe.Name].Add([double]$captured.PeakKb)
                }
            }
        }
    }
}

foreach ($pattern in $Patterns) {
    $Summary.Add("## " + $pattern)
    $Summary.Add("")
    $Summary.Add("| allocator | median M ops/sec | median peak_kb | runs |")
    $Summary.Add("| --- | ---: | ---: | --- |")
    foreach ($exe in $Executables) {
        $runsList = $PatternResults[$pattern][$exe.Name]
        $peakList = $PatternPeakResults[$pattern][$exe.Name]
        $median = Get-Median -Values $runsList.ToArray()
        $medianPeak = if ($peakList.Count -gt 0) { Get-Median -Values $peakList.ToArray() } else { [double]::NaN }
        $runText = ($runsList | ForEach-Object { ('{0:N2}' -f $_) }) -join ", "
        $Summary.Add(('| {0} | {1:N2} | {2} | `{3}` |' -f $exe.Name, $median, ("{0:N0}" -f $medianPeak), $runText))
    }
    $Summary.Add("")
}

$Summary.Add("## Failures")
$Summary.Add("")
if ($Failures.Count -eq 0) {
    $Summary.Add("- none")
} else {
    foreach ($failure in $Failures) {
        $Summary.Add(("- {0}" -f $failure))
    }
}
$Summary.Add("")

$Summary.Add("Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
