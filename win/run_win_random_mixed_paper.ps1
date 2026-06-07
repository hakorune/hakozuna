param(
    [string]$OutputDir,
    [int]$Runs = 10,
    [string[]]$Profiles,
    [string[]]$Allocators,
    [switch]$ContinueOnFailure,
    [switch]$DiagnosticHz6Probes
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDirName = if ($DiagnosticHz6Probes) {
    "out_win_random_mixed_diag"
} else {
    "out_win_random_mixed"
}
$SuiteDir = Join-Path $RepoRoot $SuiteDirName
$BuildScript = Join-Path $PSScriptRoot "build_win_random_mixed_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_random_mixed_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz4.exe") },
    @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz5_policy.exe") },
    @{ Name = "hz7-tinyroute"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz7.exe") },
    @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_strict.exe") },
    @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_speed.exe") },
    @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_rss.exe") },
    @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_strict_broad.exe") },
    @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_speed_broad.exe") },
    @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_rss_broad.exe") },
    @{ Name = "hz6-strict-control"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_strict_control.exe") },
    @{ Name = "hz6-speed-control"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_speed_control.exe") },
    @{ Name = "hz6-rss-control"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_rss_control.exe") },
    @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_strict_route4k.exe") },
    @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_speed_route4k.exe") },
    @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_rss_route4k.exe") },
    @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_strict_appcap.exe") },
    @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_speed_appcap.exe") },
    @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_random_mixed_hz6_rss_appcap.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_random_mixed_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_random_mixed_tcmalloc.exe") }
)

if ($Allocators -and $Allocators.Count -gt 0) {
    $AllocatorNames = @()
    foreach ($name in $Allocators) {
        foreach ($part in @($name -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })) {
            $AllocatorNames += $part
        }
    }
    $SelectedExecutables = @()
    foreach ($name in $AllocatorNames) {
        $match = $Executables | Where-Object { $_.Name -eq $name }
        if (-not $match) {
            throw "Unknown allocator: $name"
        }
        $SelectedExecutables += $match
    }
    $Executables = $SelectedExecutables
}

if ($DiagnosticHz6Probes -or ($Executables | Where-Object { -not (Test-Path $_.Path) })) {
    & $BuildScript -DiagnosticHz6Probes:$DiagnosticHz6Probes -OutDirName $SuiteDirName
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_random_mixed_suite.ps1 failed with exit code $LASTEXITCODE"
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
                if ($line -ne "") { $lines.Add($line) }
            }
        }
    }

    return @{ ExitCode = $proc.ExitCode; Lines = $lines }
}

$AllProfiles = @(
    @{ Name = "small"; Iters = 20000000; WorkingSet = 400; MinSize = 16; MaxSize = 2048; Note = "paper SSOT small range" },
    @{ Name = "medium"; Iters = 20000000; WorkingSet = 400; MinSize = 4096; MaxSize = 32768; Note = "paper SSOT medium range" },
    @{ Name = "mixed"; Iters = 20000000; WorkingSet = 400; MinSize = 16; MaxSize = 32768; Note = "paper SSOT mixed range" }
)

if ($Profiles -and $Profiles.Count -gt 0) {
    $ProfileNames = @()
    foreach ($name in $Profiles) {
        foreach ($part in @($name -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })) {
            $ProfileNames += $part
        }
    }
    $Selected = @()
    foreach ($name in $ProfileNames) {
        $match = $AllProfiles | Where-Object { $_.Name -eq $name }
        if (-not $match) {
            throw "Unknown profile: $name"
        }
        $Selected += $match
    }
} else {
    $Selected = $AllProfiles
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_random_mixed_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_random_mixed_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Paper-Aligned Windows Random Mixed")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L151)")
$Summary.Add("- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/ssot_stats.csv)")
$Summary.Add("")
$Summary.Add("Windows native note:")
$Summary.Add('- benchmark: `bench_random_mixed_compare`')
$Summary.Add('- allocator model: per-allocator link-mode executables, no `LD_PRELOAD`')
$Summary.Add('- throughput statistic: `median ops/s`')
$Summary.Add('- memory note: Windows reports `PeakWorkingSetSize` as `[RSS] peak_kb`, which is not identical to Linux `ru_maxrss`')
$Summary.Add(('- profiles: `small`, `medium`, `mixed` with `RUNS={0}`, `ITERS=20,000,000`, `WS=400`' -f $Runs))
$Summary.Add(('- selected allocators: `{0}`' -f (($Executables | ForEach-Object { $_.Name }) -join ', ')))
$Summary.Add('- `hz7-tinyroute` is a direct-API TinyRoute row: span classes currently cover `<=16KiB`, `>16KiB` uses direct OS regions, and it is not an interposer/general allocator row yet.')
$Summary.Add('- HZ6 rows now include `broad`, `control`, `route4k`, and `appcap` capacity lanes; `route4k` isolates route-table capacity while keeping the other control capacities.')
$Summary.Add("")

foreach ($profile in $Selected) {
    $Summary.Add("## " + $profile.Name)
    $Summary.Add("")
    $Summary.Add("- Note: " + $profile.Note)
    $Summary.Add(('- Params: `iters={0} ws={1} size={2}..{3}`' -f $profile.Iters, $profile.WorkingSet, $profile.MinSize, $profile.MaxSize))
    $Summary.Add(('- Runs: `{0}`' -f $Runs))
    $Summary.Add("")
    $Summary.Add("| allocator | median ops/s | median peak_kb | runs |")
    $Summary.Add("| --- | ---: | ---: | --- |")

    foreach ($exe in $Executables) {
        $opsRuns = New-Object System.Collections.Generic.List[double]
        $rssRuns = New-Object System.Collections.Generic.List[double]
        $runTexts = New-Object System.Collections.Generic.List[string]

        for ($run = 1; $run -le $Runs; $run++) {
            $args = @(
                [string]$profile.Iters,
                [string]$profile.WorkingSet,
                [string]$profile.MinSize,
                [string]$profile.MaxSize
            )
            $result = Invoke-CapturedProcess -FilePath $exe.Path -Arguments $args
            $output = $result.Lines
            $rc = $result.ExitCode
            $raw = (($output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
            if (-not $raw) {
                $raw = "(no output)"
            }

            $RawLines.Add("=== " + $profile.Name + " / " + $exe.Name + " / run " + $run + " ===")
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
                throw "Profile $($profile.Name) allocator $($exe.Name) failed with exit code $rc"
            }
            if ($raw -notmatch "ops/s=([0-9.]+)") {
                if ($ContinueOnFailure) {
                    $runTexts.Add("failed:parse")
                    continue
                }
                throw "Could not parse ops/s for profile $($profile.Name) allocator $($exe.Name)"
            }
            $ops = [double]$Matches[1]
            $opsRuns.Add($ops)

            $rssKb = [double]::NaN
            if ($raw -match "peak_kb=([0-9.]+)") {
                $rssKb = [double]$Matches[1]
                $rssRuns.Add($rssKb)
                $runTexts.Add(("{0:N3}M / {1:N0} KB" -f ($ops / 1000000.0), $rssKb))
            } else {
                $runTexts.Add(("{0:N3}M" -f ($ops / 1000000.0)))
            }
        }

        if ($opsRuns.Count -eq 0) {
            $Summary.Add(('| {0} | failed | n/a | `{1}` |' -f $exe.Name, ($runTexts -join ", ")))
            continue
        }

        $medianOps = Get-Median -Values $opsRuns.ToArray()
        $medianRss = ""
        if ($rssRuns.Count -gt 0) {
            $medianRss = ("{0:N0}" -f (Get-Median -Values $rssRuns.ToArray()))
        } else {
            $medianRss = "n/a"
        }

        $Summary.Add(('| {0} | {1:N3}M | {2} | `{3}` |' -f $exe.Name, ($medianOps / 1000000.0), $medianRss, ($runTexts -join ", ")))
    }

    $Summary.Add("")
}

$Summary.Add(("Artifacts: [{0}](/C:/git/hakozuna-win/{0})" -f $SuiteDirName))

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
