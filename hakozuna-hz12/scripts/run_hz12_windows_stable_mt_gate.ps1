param(
    [string]$OutputPath,
    [int]$Rounds = 3,
    [double]$TargetSeconds = 3.0,
    [UInt64]$AffinityMask = 0xFF,
    [ValidateSet("Normal", "AboveNormal", "High")]
    [string]$PriorityClass = "High",
    [Alias("Profiles")]
    [string[]]$ProfileNames,
    [switch]$IncludeRefillBatchProbe,
    [switch]$IncludeHz11Fine128TransferProbe,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz12Root
$Hz12Out = Join-Path $Hz12Root "out_win_broad"
$SuiteOut = Join-Path $RepoRoot "out_win_suite"
$Hz12Build = Join-Path $PSScriptRoot "build_hz12_windows_broad_controls.ps1"
$SuiteBuild = Join-Path $RepoRoot "win\build_win_allocator_suite.ps1"

if ($Rounds -lt 1) { throw "Rounds must be positive." }
if ($TargetSeconds -lt 0.5) { throw "TargetSeconds must be at least 0.5." }
if ($AffinityMask -eq 0u) { throw "AffinityMask must select at least one CPU." }

$Rows = @(
    [pscustomobject]@{ Name = "hz11-span-cache256"; Path = (Join-Path $SuiteOut "bench_mixed_ws_hz11_span_cache256.exe") },
    [pscustomobject]@{ Name = "hz12-core"; Path = (Join-Path $Hz12Out "bench_mixed_ws_hz12_core.exe") },
    [pscustomobject]@{ Name = "hz12-coldspanowner"; Path = (Join-Path $Hz12Out "bench_mixed_ws_hz12_coldspanowner.exe") },
    [pscustomobject]@{ Name = "tcmalloc"; Path = (Join-Path $SuiteOut "bench_mixed_ws_tcmalloc.exe") }
)
if ($IncludeRefillBatchProbe) {
    $Rows += [pscustomobject]@{
        Name = "hz12-coldspanowner-batch32"
        Path = (Join-Path $Hz12Out "bench_mixed_ws_hz12_coldspanowner_batch32.exe")
    }
}
if ($IncludeHz11Fine128TransferProbe) {
    $Rows += [pscustomobject]@{
        Name = "hz11-span-transfer-fine128-win"
        Path = (Join-Path $SuiteOut "bench_mixed_ws_hz11_span_transfer_fine128_win.exe")
    }
}

$Profiles = @(
    [pscustomobject]@{ Name = "balanced"; Threads = 8; PilotIters = 500000; WorkingSet = 4096; MinSize = 16; MaxSize = 2048 },
    [pscustomobject]@{ Name = "wide_ws"; Threads = 8; PilotIters = 500000; WorkingSet = 16384; MinSize = 16; MaxSize = 1024 },
    [pscustomobject]@{ Name = "larger_sizes"; Threads = 4; PilotIters = 300000; WorkingSet = 4096; MinSize = 256; MaxSize = 8192 }
)
if ($ProfileNames -and $ProfileNames.Count -gt 0) {
    $wanted = @($ProfileNames | ForEach-Object { $_ -split ',' } |
                ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $Profiles = @($Profiles | Where-Object { $wanted -contains $_.Name })
    if ($Profiles.Count -ne $wanted.Count) {
        throw "Unknown or duplicate profile in: $($wanted -join ', ')"
    }
}

if (-not $SkipBuild) {
    & $Hz12Build
    if ($LASTEXITCODE -ne 0) { throw "HZ12 build failed: $LASTEXITCODE" }
    & $SuiteBuild
    if ($LASTEXITCODE -ne 0) { throw "allocator suite build failed: $LASTEXITCODE" }
}
foreach ($row in $Rows) {
    if (-not (Test-Path $row.Path)) { throw "Missing benchmark: $($row.Path)" }
}

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path $Hz12Root "bench_results\windows_stable_mt_gate_$stamp.md"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $OutputPath) | Out-Null

function Get-Percentile([double[]]$Values, [double]$Fraction) {
    $sorted = @($Values | Sort-Object)
    if ($sorted.Count -eq 1) { return $sorted[0] }
    $position = ($sorted.Count - 1) * $Fraction
    $lower = [int][Math]::Floor($position)
    $upper = [int][Math]::Ceiling($position)
    if ($lower -eq $upper) { return $sorted[$lower] }
    return $sorted[$lower] + ($sorted[$upper] - $sorted[$lower]) *
        ($position - $lower)
}

function Invoke-StableProcess([string]$Path, [string[]]$Arguments) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Path
    $startInfo.WorkingDirectory = Split-Path -Parent $Path
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) { [void]$startInfo.ArgumentList.Add($argument) }
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) { throw "Failed to start $Path" }
    $process.ProcessorAffinity = [IntPtr]([Int64]$AffinityMask)
    $process.PriorityClass = [System.Diagnostics.ProcessPriorityClass]$PriorityClass
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $peak = [int64]0
    while (-not $process.HasExited) {
        try {
            $process.Refresh()
            if ($process.PeakWorkingSet64 -gt $peak) { $peak = $process.PeakWorkingSet64 }
        } catch {}
        Start-Sleep -Milliseconds 5
    }
    $process.WaitForExit()
    $output = ($stdoutTask.Result + "`n" + $stderrTask.Result).Trim()
    if ($process.ExitCode -ne 0) {
        throw "Benchmark failed rc=$($process.ExitCode): $Path`n$output"
    }
    if ($output -notmatch 'time=([0-9.]+)') { throw "Missing time field: $output" }
    $elapsed = [double]$Matches[1]
    if ($output -notmatch 'ops/s=([0-9.]+)') { throw "Missing ops/s field: $output" }
    $ops = [double]$Matches[1]
    return [pscustomobject]@{
        Elapsed = $elapsed
        Ops = $ops
        PeakMiB = $peak / 1MB
        Raw = $output -replace '[\r\n]+', ' '
    }
}

$Calibration = @{}
foreach ($profile in $Profiles) {
    foreach ($row in $Rows) {
        $args = @([string]$profile.Threads, [string]$profile.PilotIters,
                  [string]$profile.WorkingSet, [string]$profile.MinSize,
                  [string]$profile.MaxSize)
        $pilot = Invoke-StableProcess $row.Path $args
        $elapsed = [Math]::Max($pilot.Elapsed, 0.001)
        $scaled = [Math]::Ceiling($profile.PilotIters * $TargetSeconds / $elapsed)
        $iters = [int64][Math]::Min(200000000, [Math]::Max(100000, $scaled))
        $calArgs = @([string]$profile.Threads, [string]$iters,
                     [string]$profile.WorkingSet, [string]$profile.MinSize,
                     [string]$profile.MaxSize)
        $calSample = Invoke-StableProcess $row.Path $calArgs
        $calElapsed = [Math]::Max($calSample.Elapsed, 0.001)
        $rescaled = [Math]::Ceiling($iters * $TargetSeconds / $calElapsed)
        $iters = [int64][Math]::Min(200000000, [Math]::Max(100000, $rescaled))
        $Calibration["$($profile.Name)|$($row.Name)"] = $iters
        Write-Host ("[HZ12_STABLE_CAL] profile={0} allocator={1} pilot={2:F3}s second={3:F3}s iters={4}" -f
                    $profile.Name, $row.Name, $pilot.Elapsed,
                    $calSample.Elapsed, $iters)
    }
}

$SampleTable = @{}
$RawLines = [System.Collections.Generic.List[string]]::new()
foreach ($profile in $Profiles) {
    foreach ($row in $Rows) { $SampleTable["$($profile.Name)|$($row.Name)"] = @() }
    for ($round = 0; $round -lt $Rounds; ++$round) {
        for ($position = 0; $position -lt $Rows.Count; ++$position) {
            $row = $Rows[($position + $round) % $Rows.Count]
            $key = "$($profile.Name)|$($row.Name)"
            $iters = $Calibration[$key]
            $args = @([string]$profile.Threads, [string]$iters,
                      [string]$profile.WorkingSet, [string]$profile.MinSize,
                      [string]$profile.MaxSize)
            $sample = Invoke-StableProcess $row.Path $args
            $SampleTable[$key] += $sample
            $RawLines.Add(("round={0} profile={1} allocator={2} iters={3} elapsed={4:F3} ops={5:F3} peak_mib={6:F2} raw={7}" -f
                ($round + 1), $profile.Name, $row.Name, $iters,
                $sample.Elapsed, $sample.Ops, $sample.PeakMiB, $sample.Raw))
            Write-Host ("[HZ12_STABLE_MT] round={0} profile={1} allocator={2} elapsed={3:F3}s ops={4:F3} peak={5:F2}MiB" -f
                ($round + 1), $profile.Name, $row.Name, $sample.Elapsed,
                $sample.Ops, $sample.PeakMiB)
        }
    }
}

$Lines = [System.Collections.Generic.List[string]]::new()
$Lines.Add("# HZ12 Windows Stable-Duration MT Gate")
$Lines.Add("")
$Lines.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')")
$Lines.Add("")
$Lines.Add("Target duration: $TargetSeconds seconds per measured process; rounds: $Rounds; affinity: 0x$($AffinityMask.ToString('X')); priority: $PriorityClass.")
$Lines.Add("Each allocator/profile uses a pilot-calibrated iteration count. Row order rotates every round.")
$Lines.Add("")
foreach ($profile in $Profiles) {
    $Lines.Add("## $($profile.Name)")
    $Lines.Add("")
    $Lines.Add("| allocator | median ops/s | p25 | p75 | median elapsed | median peak RSS | calibrated iters/thread |")
    $Lines.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
    foreach ($row in $Rows) {
        $key = "$($profile.Name)|$($row.Name)"
        $rowSamples = @($SampleTable[$key])
        [double[]]$ops = @($rowSamples | ForEach-Object { $_.Ops })
        [double[]]$elapsed = @($rowSamples | ForEach-Object { $_.Elapsed })
        [double[]]$rss = @($rowSamples | ForEach-Object { $_.PeakMiB })
        $medianOps = (Get-Percentile $ops 0.5) / 1e6
        $p25Ops = (Get-Percentile $ops 0.25) / 1e6
        $p75Ops = (Get-Percentile $ops 0.75) / 1e6
        $medianElapsed = Get-Percentile $elapsed 0.5
        $medianRss = Get-Percentile $rss 0.5
        $Lines.Add(("| {0} | {1:F3}M | {2:F3}M | {3:F3}M | {4:F3}s | {5:F2} MiB | {6} |" -f
            $row.Name, $medianOps, $p25Ops, $p75Ops, $medianElapsed,
            $medianRss, $Calibration[$key]))
    }
    $Lines.Add("")
}
$Lines.Add("## Raw Runs")
$Lines.Add("")
$Lines.Add('```text')
foreach ($line in $RawLines) { $Lines.Add($line) }
$Lines.Add('```')
$Lines | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Host "Wrote $OutputPath"
