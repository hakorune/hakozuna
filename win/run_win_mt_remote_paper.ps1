param(
    [string]$OutputDir,
    [int]$Runs = 10
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_mt_remote"
$BuildScript = Join-Path $PSScriptRoot "build_win_mt_remote_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz4.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_random_mixed_mt_remote_tcmalloc.exe") }
)

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_mt_remote_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Get-Median {
    param([double[]]$Values)
    if (-not $Values -or $Values.Count -eq 0) {
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
        Lines = $lines
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
$Summary.Add('- runs: `10`')
$Summary.Add('- statistic: `median ops/s`')
$Summary.Add('- hz3 profile: `scale + S97-2 direct-map bucketize + skip_tail_null`')
$Summary.Add('- note: paper originally reports `hz3 / mimalloc / tcmalloc`; this runner also records `crt` and `hz4`')
$Summary.Add("")
$Summary.Add("| allocator | median ops/s | median actual remote % | median fallback % | runs |")
$Summary.Add("| --- | ---: | ---: | ---: | --- |")

foreach ($exe in $Executables) {
    $opsRuns = New-Object System.Collections.Generic.List[double]
    $actualRuns = New-Object System.Collections.Generic.List[double]
    $fallbackRuns = New-Object System.Collections.Generic.List[double]
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
        $result = Invoke-CapturedProcess -FilePath $exe.Path -Arguments $args
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
        $runTexts.Add(("{0:N3}M / actual {1:N2}% / fallback {2:N2}%" -f ($ops / 1000000.0), $actual, $fallback))
    }

    $medianOps = Get-Median -Values $opsRuns.ToArray()
    $medianActual = Get-Median -Values $actualRuns.ToArray()
    $medianFallback = Get-Median -Values $fallbackRuns.ToArray()
    $Summary.Add(('| {0} | {1:N3}M | {2:N2} | {3:N2} | `{4}` |' -f $exe.Name, ($medianOps / 1000000.0), $medianActual, $medianFallback, ($runTexts -join ", ")))
}

$Summary.Add("")
$Summary.Add("Artifacts: [out_win_mt_remote](/C:/git/hakozuna-win/out_win_mt_remote)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
