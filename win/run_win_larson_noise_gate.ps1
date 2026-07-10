param(
    [Parameter(Mandatory = $true)]
    [string]$BaselinePath,
    [Parameter(Mandatory = $true)]
    [string]$CandidatePath,
    [string]$OutputPath,
    [int]$Runs = 5,
    [int]$RuntimeSeconds = 10,
    [int]$MinSize = 8,
    [int]$MaxSize = 1024,
    [int]$ChunksPerThread = 10000,
    [int]$Rounds = 1,
    [int]$Seed = 12345,
    [int]$Threads = 8,
    [int]$TimeoutSeconds = 120,
    [switch]$SkipAa
)

$ErrorActionPreference = "Stop"

if ($Runs -lt 1 -or $RuntimeSeconds -lt 1 -or $Threads -lt 1) {
    throw "Runs, RuntimeSeconds, and Threads must be positive."
}
if (-not (Test-Path -LiteralPath $BaselinePath)) {
    throw "Baseline executable not found: $BaselinePath"
}
if (-not (Test-Path -LiteralPath $CandidatePath)) {
    throw "Candidate executable not found: $CandidatePath"
}

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path (Join-Path (Split-Path -Parent $PSScriptRoot) `
        "docs\benchmarks\windows\noise-gates") "larson_noise_gate_$stamp.md"
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath) | Out-Null

function Get-Median {
    param([double[]]$Values)
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -eq 0) { return [double]::NaN }
    $middle = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return [double]$sorted[$middle] }
    return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}

function Invoke-Larson {
    param(
        [string]$Path,
        [int]$WarmupMode,
        [string]$Label
    )
    $temp = Join-Path ([IO.Path]::GetTempPath()) ("hz11-noise-{0}" -f [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $temp | Out-Null
    $stdout = Join-Path $temp "stdout.log"
    $stderr = Join-Path $temp "stderr.log"
    $args = @($RuntimeSeconds, $MinSize, $MaxSize, $ChunksPerThread,
        $Rounds, $Seed, $Threads, $WarmupMode)
    try {
        $proc = Start-Process -FilePath $Path -ArgumentList $args `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
            -PassThru -WindowStyle Hidden
        if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
            try { $proc.Kill($true) } catch { try { $proc.Kill() } catch {} }
            throw "$Label timed out after ${TimeoutSeconds}s"
        }
        $lines = @((Get-Content -LiteralPath $stdout -ErrorAction SilentlyContinue) +
            (Get-Content -LiteralPath $stderr -ErrorAction SilentlyContinue))
        $match = $lines | Select-String -Pattern 'Throughput = ([0-9.]+)'
        if (-not $match) {
            throw "$Label did not report throughput (exit=$($proc.ExitCode))"
        }
        $value = [double]$match.Matches[0].Groups[1].Value
        [pscustomobject]@{
            Label = $Label
            Warmup = if ($WarmupMode -ne 0) { "worker" } else { "main" }
            Throughput = $value
            ExitCode = $proc.ExitCode
        }
    } finally {
        Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

$aaRows = @()
$candidateRows = @()
foreach ($mode in @(1, 0)) {
    $warmup = if ($mode -ne 0) { "worker" } else { "main" }
    for ($pair = 1; $pair -le $Runs; $pair++) {
        if (-not $SkipAa) {
            $aaRows += Invoke-Larson $BaselinePath $mode "aa-a1-$warmup-$pair"
            $aaRows += Invoke-Larson $BaselinePath $mode "aa-a2-$warmup-$pair"
        }

        $firstBaseline = (($pair % 2) -eq 1)
        if ($firstBaseline) {
            $candidateRows += Invoke-Larson $BaselinePath $mode "candidate-base-$warmup-$pair"
            $candidateRows += Invoke-Larson $CandidatePath $mode "candidate-new-$warmup-$pair"
        } else {
            $candidateRows += Invoke-Larson $CandidatePath $mode "candidate-new-$warmup-$pair"
            $candidateRows += Invoke-Larson $BaselinePath $mode "candidate-base-$warmup-$pair"
        }
    }
}

function Get-RowsForWarmup {
    param([object[]]$Rows, [string]$Warmup, [string]$Prefix)
    return @($Rows | Where-Object { $_.Warmup -eq $Warmup -and $_.Label -like "$Prefix*" })
}

$md = New-Object System.Collections.Generic.List[string]
$md.Add("# Windows Larson Noise Gate")
$md.Add("")
$md.Add("- Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss K')")
$md.Add("- Runs: $Runs paired runs per warmup mode")
$md.Add("- Runtime: ${RuntimeSeconds}s, threads: $Threads, size: $MinSize..$MaxSize")
$md.Add("- Baseline: $BaselinePath")
$md.Add("- Candidate: $CandidatePath")
$md.Add("")
$md.Add("## Summary")
$md.Add("")
$md.Add("| Warmup | A/A baseline median | A/A delta | Candidate baseline median | Candidate median | Candidate delta |")
$md.Add("| --- | ---: | ---: | ---: | ---: | ---: |")

foreach ($warmup in @("worker", "main")) {
    $aa1 = @(Get-RowsForWarmup $aaRows $warmup "aa-a1")
    $aa2 = @(Get-RowsForWarmup $aaRows $warmup "aa-a2")
    $cb = @(Get-RowsForWarmup $candidateRows $warmup "candidate-base")
    $cn = @(Get-RowsForWarmup $candidateRows $warmup "candidate-new")
    $aa1Median = Get-Median @($aa1.Throughput)
    $aa2Median = Get-Median @($aa2.Throughput)
    $cbMedian = Get-Median @($cb.Throughput)
    $cnMedian = Get-Median @($cn.Throughput)
    $aaDelta = if ($aa1Median) { 100.0 * ($aa2Median / $aa1Median - 1.0) } else { [double]::NaN }
    $candidateDelta = if ($cbMedian) { 100.0 * ($cnMedian / $cbMedian - 1.0) } else { [double]::NaN }
    $md.Add(("| {0} | {1:N3}M | {2:+0.0;-0.0;0.0}% | {3:N3}M | {4:N3}M | {5:+0.0;-0.0;0.0}% |" -f `
        $warmup, ($aa1Median / 1000000.0), $aaDelta, ($cbMedian / 1000000.0),
        ($cnMedian / 1000000.0), $candidateDelta))
}

$md.Add("")
$md.Add("## Pair Data")
$md.Add("")
$md.Add("| Warmup | Pair | A/A A1 | A/A A2 | Candidate baseline | Candidate | A/A delta | Candidate delta |")
$md.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
foreach ($warmup in @("worker", "main")) {
    for ($pair = 1; $pair -le $Runs; $pair++) {
        $a1 = $aaRows | Where-Object { $_.Label -eq "aa-a1-$warmup-$pair" }
        $a2 = $aaRows | Where-Object { $_.Label -eq "aa-a2-$warmup-$pair" }
        $cb = $candidateRows | Where-Object { $_.Label -eq "candidate-base-$warmup-$pair" }
        $cn = $candidateRows | Where-Object { $_.Label -eq "candidate-new-$warmup-$pair" }
        $aaDelta = if ($a1.Throughput) { 100.0 * ($a2.Throughput / $a1.Throughput - 1.0) } else { [double]::NaN }
        $candidateDelta = if ($cb.Throughput) { 100.0 * ($cn.Throughput / $cb.Throughput - 1.0) } else { [double]::NaN }
        $md.Add(("| {0} | {1} | {2:N3}M | {3:N3}M | {4:N3}M | {5:N3}M | {6:+0.0;-0.0;0.0}% | {7:+0.0;-0.0;0.0}% |" -f `
            $warmup, $pair, ($a1.Throughput / 1000000.0), ($a2.Throughput / 1000000.0),
            ($cb.Throughput / 1000000.0), ($cn.Throughput / 1000000.0), $aaDelta,
            $candidateDelta))
    }
}

$md.Add("")
$md.Add("Interpret candidate deltas against the A/A noise band; do not use a fixed 3% Larson threshold in isolation.")
[IO.File]::WriteAllLines($OutputPath, $md)
Write-Host "Wrote $OutputPath"
