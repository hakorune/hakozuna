param(
    [string]$OutputDir,
    [int]$Runs = 5
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
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_redis_workload_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_redis_workload_tcmalloc.exe") }
)

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_redis_workload_suite.ps1 failed with exit code $LASTEXITCODE"
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
$Summary.Add('- runs: `5`')
$Summary.Add('- statistic: `median M ops/sec` per pattern')
$Summary.Add('- note: paper originally compares `hz3` and `tcmalloc`; this runner also records `crt`, `hz4`, and `mimalloc`')
$Summary.Add("")

$PatternResults = @{}
foreach ($pattern in $Patterns) {
    $PatternResults[$pattern] = @{}
    foreach ($exe in $Executables) {
        $PatternResults[$pattern][$exe.Name] = New-Object System.Collections.Generic.List[double]
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
        $output = & $exe.Path @args 2>&1
        $rc = $LASTEXITCODE

        $RawLines.Add("=== " + $exe.Name + " / run " + $run + " ===")
        $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
        $RawLines.Add("rc: " + $rc)
        foreach ($line in $output) {
            $RawLines.Add($line.ToString())
        }
        $RawLines.Add("")

        if ($rc -ne 0) {
            throw "Redis workload runner allocator $($exe.Name) failed with exit code $rc"
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
            }
        }
    }
}

foreach ($pattern in $Patterns) {
    $Summary.Add("## " + $pattern)
    $Summary.Add("")
    $Summary.Add("| allocator | median M ops/sec | runs |")
    $Summary.Add("| --- | ---: | --- |")
    foreach ($exe in $Executables) {
        $runsList = $PatternResults[$pattern][$exe.Name]
        $median = Get-Median -Values $runsList.ToArray()
        $runText = ($runsList | ForEach-Object { ('{0:N2}' -f $_) }) -join ", "
        $Summary.Add(('| {0} | {1:N2} | `{2}` |' -f $exe.Name, $median, $runText))
    }
    $Summary.Add("")
}

$Summary.Add("Artifacts: [out_win_redis](/C:/git/hakozuna-win/out_win_redis)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
