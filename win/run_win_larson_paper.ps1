param(
    [string]$OutputDir,
    [int]$Runs = 5,
    [int[]]$ThreadCounts
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_larson"
$BuildScript = Join-Path $PSScriptRoot "build_win_larson_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_larson_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_larson_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_larson_hz4.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_larson_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_larson_tcmalloc.exe") }
)

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_larson_suite.ps1 failed with exit code $LASTEXITCODE"
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

if (-not $ThreadCounts -or $ThreadCounts.Count -eq 0) {
    $ThreadCounts = @(1, 4, 8, 16)
}

$RuntimeSec = 10
$MinSize = 8
$MaxSize = 1024
$ChunksPerThread = 10000
$Rounds = 1
$Seed = 12345

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_larson_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_larson_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Paper-Aligned Windows Larson")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)")
$Summary.Add("- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)")
$Summary.Add("")
$Summary.Add("Windows native note:")
$Summary.Add('- benchmark: `bench_larson_compare`')
$Summary.Add('- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`')
$Summary.Add('- thread sweep: `1, 4, 8, 16`')
$Summary.Add('- runs: `5`')
$Summary.Add('- statistic: `median alloc/s` from the benchmark''s `Throughput = ...` line')
$Summary.Add('- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`')
$Summary.Add("")

foreach ($threads in $ThreadCounts) {
    $Summary.Add("## T=" + $threads)
    $Summary.Add("")
    $Summary.Add("| allocator | median ops/s | runs |")
    $Summary.Add("| --- | ---: | --- |")

    foreach ($exe in $Executables) {
        $opsRuns = New-Object System.Collections.Generic.List[double]
        $runTexts = New-Object System.Collections.Generic.List[string]

        for ($run = 1; $run -le $Runs; $run++) {
            $args = @(
                [string]$RuntimeSec,
                [string]$MinSize,
                [string]$MaxSize,
                [string]$ChunksPerThread,
                [string]$Rounds,
                [string]$Seed,
                [string]$threads
            )
            $output = & $exe.Path @args 2>&1
            $rc = $LASTEXITCODE

            $RawLines.Add("=== T=" + $threads + " / " + $exe.Name + " / run " + $run + " ===")
            $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
            $RawLines.Add("rc: " + $rc)
            foreach ($line in $output) {
                $RawLines.Add($line.ToString())
            }
            $RawLines.Add("")

            if ($rc -ne 0) {
                throw "Larson runner allocator $($exe.Name) failed at T=$threads with exit code $rc"
            }

            $raw = (($output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
            if ($raw -notmatch "Throughput = ([0-9.]+) operations per second") {
                throw "Could not parse throughput for allocator $($exe.Name) at T=$threads"
            }
            $ops = [double]$Matches[1]
            $opsRuns.Add($ops)
            $runTexts.Add(("{0:N3}M" -f ($ops / 1000000.0)))
        }

        $medianOps = Get-Median -Values $opsRuns.ToArray()
        $Summary.Add(('| {0} | {1:N3}M | `{2}` |' -f $exe.Name, ($medianOps / 1000000.0), ($runTexts -join ", ")))
    }

    $Summary.Add("")
}

$Summary.Add("Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
