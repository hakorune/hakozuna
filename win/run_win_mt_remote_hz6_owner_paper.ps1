param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Threads = 16,
    [int]$Iters = 2000000,
    [int]$WorkingSet = 400,
    [int]$MinSize = 16,
    [int]$MaxSize = 2048,
    [int]$RemotePct = 90,
    [int]$RingSlots = 65536,
    [int]$TimeoutSeconds = 900,
    [string[]]$Profiles,
    [switch]$ContinueOnFailure,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_mt_remote_hz6_owner"
$BuildScript = Join-Path $PSScriptRoot "build_win_mt_remote_hz6_owner_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "results\windows-hz6-owner-mt-remote"
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

function Invoke-CapturedProcessWithTimeout {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds
    )

    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = ($Arguments | ForEach-Object {
        if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }) -join ' '

    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $peakWorkingSetBytes = [Int64]0
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)

    while (-not $proc.HasExited) {
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
        if ([DateTime]::UtcNow -ge $deadline) {
            try { $proc.Kill($true) } catch {}
            $proc.WaitForExit()
            return @{
                ExitCode = 124
                TimedOut = $true
                PeakWorkingSetKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
                Lines = @("[TIMEOUT] exceeded ${TimeoutSeconds}s")
            }
        }
        Start-Sleep -Milliseconds 10
    }

    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
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
        TimedOut = $false
        PeakWorkingSetKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
        Lines = $lines
    }
}

if (-not $Profiles -or $Profiles.Count -eq 0) {
    $Profiles = @("strict", "speed", "rss", "remote")
}

if (-not $SkipBuild) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_mt_remote_hz6_owner_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

$Executables = @()
foreach ($profile in $Profiles) {
    $exe = Join-Path $SuiteDir ("bench_random_mixed_mt_remote_hz6_owner_{0}.exe" -f $profile)
    if (-not (Test-Path $exe)) {
        throw "missing executable: $exe"
    }
    $Executables += @{ Name = $profile; Path = $exe }
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_mt_remote_hz6_owner_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_mt_remote_hz6_owner_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# HZ6 Owner-Aware MT Remote")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("Benchmark:")
$Summary.Add('benchmark: `bench_random_mixed_mt_remote_hz6_owner_compare`')
$Summary.Add('- owner-aware remote frees use per-thread HZ6 allocators and owner-slot switching')
$Summary.Add('- peak working set is sampled from Windows process counters')
$Summary.Add(('runs: `{0}` threads=`{1}` iters=`{2}` ws=`{3}` size=`{4}..{5}` remote_pct=`{6}` ring_slots=`{7}` timeout=`{8}s`' -f $Runs, $Threads, $Iters, $WorkingSet, $MinSize, $MaxSize, $RemotePct, $RingSlots, $TimeoutSeconds))
$Summary.Add("")
$Summary.Add("| allocator | median ops/s | median peak_kb | median alloc_failures | median remote_frees | runs |")
$Summary.Add("| --- | ---: | ---: | ---: | ---: | --- |")

foreach ($exe in $Executables) {
    $opsRuns = New-Object System.Collections.Generic.List[double]
    $rssRuns = New-Object System.Collections.Generic.List[double]
    $allocFailRuns = New-Object System.Collections.Generic.List[double]
    $remoteFreeRuns = New-Object System.Collections.Generic.List[double]
    $runTexts = New-Object System.Collections.Generic.List[string]

    for ($run = 1; $run -le $Runs; $run++) {
        $args = @(
            $exe.Name,
            [string]$Threads,
            [string]$Iters,
            [string]$WorkingSet,
            [string]$MinSize,
            [string]$MaxSize,
            [string]$RemotePct,
            [string]$RingSlots
        )
        $result = Invoke-CapturedProcessWithTimeout -FilePath $exe.Path -Arguments $args -TimeoutSeconds $TimeoutSeconds
        $output = $result.Lines
        $rc = $result.ExitCode
        $raw = (($output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
        if (-not $raw) {
            $raw = "(no output)"
        }

        $RawLines.Add("=== " + $exe.Name + " / run " + $run + " ===")
        $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
        $RawLines.Add("rc: " + $rc)
        foreach ($line in $output) {
            $RawLines.Add($line.ToString())
        }
        $RawLines.Add("")

        if ($rc -ne 0) {
            if ($ContinueOnFailure) {
                $runTexts.Add(("failed:rc{0}" -f $rc))
                continue
            }
            throw "HZ6 owner-aware mt-remote allocator $($exe.Name) failed with exit code $rc"
        }
        if ($raw -notmatch "ops/s=([0-9.]+)") {
            if ($ContinueOnFailure) {
                $runTexts.Add("failed:parse")
                continue
            }
            throw "Could not parse ops/s for allocator $($exe.Name)"
        }
        $ops = [double]$Matches[1]
        $opsRuns.Add($ops)

        $peakKb = [double]::NaN
        if ($result.PeakWorkingSetKb -match '^[0-9]+$') {
            $peakKb = [double]$result.PeakWorkingSetKb
            $rssRuns.Add($peakKb)
        }

        $allocFails = 0.0
        if ($raw -match "alloc_failures=([0-9]+)") {
            $allocFails = [double]$Matches[1]
            $allocFailRuns.Add($allocFails)
        }

        $remoteFrees = 0.0
        if ($raw -match "remote_frees=([0-9]+)") {
            $remoteFrees = [double]$Matches[1]
            $remoteFreeRuns.Add($remoteFrees)
        }

        $runText = ("{0:N3}M / {1} KB / alloc_fail={2:N0} / remote_frees={3:N0}" -f ($ops / 1000000.0), $result.PeakWorkingSetKb, $allocFails, $remoteFrees)
        $runTexts.Add($runText)
    }

    if ($opsRuns.Count -eq 0) {
        $Summary.Add(('| {0} | failed | n/a | n/a | n/a | `{1}` |' -f $exe.Name, ($runTexts -join ", ")))
        continue
    }

    $medianOps = Get-Median -Values $opsRuns.ToArray()
    $medianRss = if ($rssRuns.Count -gt 0) { ("{0:N0}" -f (Get-Median -Values $rssRuns.ToArray())) } else { "n/a" }
    $medianAllocFails = if ($allocFailRuns.Count -gt 0) { ("{0:N0}" -f (Get-Median -Values $allocFailRuns.ToArray())) } else { "n/a" }
    $medianRemoteFrees = if ($remoteFreeRuns.Count -gt 0) { ("{0:N0}" -f (Get-Median -Values $remoteFreeRuns.ToArray())) } else { "n/a" }

    $Summary.Add(('| {0} | {1:N3}M | {2} | {3} | {4} | `{5}` |' -f $exe.Name, ($medianOps / 1000000.0), $medianRss, $medianAllocFails, $medianRemoteFrees, ($runTexts -join ", ")))
}

$Summary.Add("")
$Summary.Add("Artifacts: [out_win_mt_remote_hz6_owner](/C:/git/hakozuna-win/out_win_mt_remote_hz6_owner)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
