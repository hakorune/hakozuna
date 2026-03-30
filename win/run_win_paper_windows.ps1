param(
    [string]$OutputDir,
    [int]$Runs = 5
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\\benchmarks\\windows\\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_mixed_ws_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz4.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_tcmalloc.exe") }
)

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Invoke-BenchProcess {
    param(
        [string]$Path,
        [string[]]$Args
    )

    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $Path @Args 2>&1
        $rc = $LASTEXITCODE
        return [pscustomobject]@{
            Output    = $output
            ExitCode  = $rc
        }
    } finally {
        $ErrorActionPreference = $prevEap
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

$Threads = 4
$Iters = 1000000
$WorkingSet = 8192
$MinSize = 16
$MaxSize = 1024
$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Paper-Aligned Windows Bench")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("Reference: [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L130)")
$Summary.Add("")
$Summary.Add("Paper-aligned Windows-native condition:")
$Summary.Add("")
$Summary.Add('- benchmark: `bench_mixed_ws`')
$Summary.Add('- params: `threads=4 iters=1000000 ws=8192 size=16..1024`')
$Summary.Add('- runs: `5`')
$Summary.Add('- statistic: `median ops/s`')
$Summary.Add('- note: paper originally reports `CRT / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`')
$Summary.Add("")
$Summary.Add('| allocator | median ops/s | runs |')
$Summary.Add('| --- | ---: | --- |')

foreach ($exe in $Executables) {
    $opsRuns = New-Object System.Collections.Generic.List[double]
    $runTexts = New-Object System.Collections.Generic.List[string]
    for ($run = 1; $run -le $Runs; $run++) {
        $args = @(
            [string]$Threads,
            [string]$Iters,
            [string]$WorkingSet,
            [string]$MinSize,
            [string]$MaxSize
        )
        $result = Invoke-BenchProcess -Path $exe.Path -Args $args
        $raw = (($result.Output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
        if (-not $raw) {
            $raw = "(no output)"
        }
        $RawLines.Add("=== " + $exe.Name + " run " + $run + " ===")
        $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
        $RawLines.Add("rc: " + $result.ExitCode)
        $RawLines.Add($raw)
        $RawLines.Add("")
        if ($result.ExitCode -ne 0) {
            throw "Paper runner allocator $($exe.Name) failed with exit code $($result.ExitCode)"
        }
        if ($raw -notmatch "ops/s=([0-9.]+)") {
            throw "Could not parse ops/s for allocator $($exe.Name)"
        }
        $ops = [double]$Matches[1]
        $opsRuns.Add($ops)
        $runTexts.Add(("{0:N3}M" -f ($ops / 1000000.0)))
    }

    $median = Get-Median -Values $opsRuns.ToArray()
    $Summary.Add(('| {0} | {1:N3}M | `{2}` |' -f $exe.Name, ($median / 1000000.0), ($runTexts -join ", ")))
}

$Summary.Add("")
$Summary.Add("Artifacts: [out_win_suite](/C:/git/hakozuna-win/out_win_suite)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
