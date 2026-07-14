param(
    [string]$OutputDir,
    [int]$Blocks = 10,
    [int]$TimeoutSeconds = 900,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_mt_remote"
$BuildScript = Join-Path $PSScriptRoot "build_win_mt_remote_suite.ps1"
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @{
    default = Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz8_pre_transition.exe"
    inventory = Join-Path $SuiteDir "bench_random_mixed_mt_remote_hz8.exe"
}

if (-not $SkipBuild) {
    & $BuildScript -OnlyHz8 -IncludeHz8Research `
        -RequestedHz8Variants @("hz8-pre-transition-rollback", "hz8")
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_mt_remote_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}
foreach ($path in $Executables.Values) {
    if (-not (Test-Path $path)) {
        throw "Missing benchmark executable: $path"
    }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Count -eq 0) {
        return [double]::NaN
    }
    $sorted = @($Values | Sort-Object)
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Get-ParsedDouble {
    param([string]$Text, [string]$Pattern, [string]$Name)
    $match = [regex]::Match($Text, $Pattern)
    if (-not $match.Success) {
        throw "Could not parse $Name"
    }
    return [double]$match.Groups[1].Value
}

function Invoke-CapturedProcess {
    param([string]$FilePath, [string[]]$Arguments, [int]$TimeoutSeconds)

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = ($Arguments -join " ")

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $psi
    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        $process.WaitForExit()
        throw "Benchmark timed out after $TimeoutSeconds seconds: $FilePath"
    }
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        throw "Benchmark failed with exit code $($process.ExitCode): $FilePath`n$stdout`n$stderr"
    }
    return @{
        Text = ($stdout + "`n" + $stderr)
        ExternalPeakKb = 0
    }
}

$Threads = 16
$Iters = 2000000
$WorkingSet = 400
$MinSize = 16
$MaxSize = 2048
$RemotePct = 90
$RingSlots = 65536
$MatchedRemote = 1
$Arguments = @(
    [string]$Threads, [string]$Iters, [string]$WorkingSet,
    [string]$MinSize, [string]$MaxSize, [string]$RemotePct,
    [string]$RingSlots, [string]$MatchedRemote
)

$Samples = New-Object System.Collections.Generic.List[object]
$RawLines = New-Object System.Collections.Generic.List[string]

function Invoke-Sample {
    param([int]$Block, [string]$Position, [int]$Pair,
          [string]$Allocator, [string]$Path)

    $result = Invoke-CapturedProcess -FilePath $Path -Arguments $Arguments `
        -TimeoutSeconds $TimeoutSeconds
    $text = $result.Text
    $sample = [pscustomobject]@{
        block = $Block
        position = $Position
        pair = $Pair
        allocator = $Allocator
        ops_sec = Get-ParsedDouble $text 'ops/s=([0-9.]+)' 'ops/s'
        actual_remote_pct = Get-ParsedDouble $text 'actual=([0-9.]+)' 'actual remote'
        fallback_pct = Get-ParsedDouble $text 'fallback_rate=([0-9.]+)' 'fallback rate'
        sends = [UInt64](Get-ParsedDouble $text 'sends=([0-9]+)' 'remote sends')
        local_frees = [UInt64](Get-ParsedDouble $text 'local_frees=([0-9]+)' 'local frees')
        fallback_frees = [UInt64](Get-ParsedDouble $text 'fallback_frees=([0-9]+)' 'fallback frees')
        recv_frees = [UInt64](Get-ParsedDouble $text 'recv_frees=([0-9]+)' 'received frees')
        push_waits = [UInt64](Get-ParsedDouble $text 'push_waits=([0-9]+)' 'push waits')
        allocation_failures = [UInt64](Get-ParsedDouble $text '\[ALLOC_FAILURES\] count=([0-9]+)' 'allocation failures')
        working_set_kb = [UInt64](Get-ParsedDouble $text 'working_set_kb=([0-9]+)' 'working set')
        peak_working_set_kb = [UInt64](Get-ParsedDouble $text 'peak_working_set_kb=([0-9]+)' 'internal peak working set')
        private_usage_kb = [UInt64](Get-ParsedDouble $text 'private_usage_kb=([0-9]+)' 'private usage')
        external_peak_kb = [UInt64]$result.ExternalPeakKb
    }
    $Samples.Add($sample)
    $RawLines.Add("=== block $Block / $Position / $Allocator ===")
    $RawLines.Add("cmd: $Path $($Arguments -join ' ')")
    foreach ($line in ($text -split "`r?`n")) {
        if ($line) { $RawLines.Add($line) }
    }
    $RawLines.Add("")
}

for ($block = 1; $block -le $Blocks; $block++) {
    Invoke-Sample $block "A1" 1 "default" $Executables.default
    Invoke-Sample $block "B1" 1 "inventory" $Executables.inventory
    Invoke-Sample $block "B2" 2 "inventory" $Executables.inventory
    Invoke-Sample $block "A2" 2 "default" $Executables.default
}

$AdmissiblePairs = New-Object System.Collections.Generic.List[object]
for ($block = 1; $block -le $Blocks; $block++) {
    for ($pair = 1; $pair -le 2; $pair++) {
        $default = $Samples | Where-Object {
            $_.block -eq $block -and $_.pair -eq $pair -and $_.allocator -eq "default"
        }
        $inventory = $Samples | Where-Object {
            $_.block -eq $block -and $_.pair -eq $pair -and $_.allocator -eq "inventory"
        }
        $matched = $default.sends -eq $inventory.sends -and
            $default.local_frees -eq $inventory.local_frees -and
            $default.recv_frees -eq $inventory.recv_frees -and
            $default.fallback_frees -eq 0 -and $inventory.fallback_frees -eq 0 -and
            $default.allocation_failures -eq 0 -and $inventory.allocation_failures -eq 0
        if ($matched) {
            $AdmissiblePairs.Add([pscustomobject]@{
                block = $block
                pair = $pair
                throughput_delta_pct = 100.0 * ($inventory.ops_sec / $default.ops_sec - 1.0)
                working_set_delta_pct = 100.0 * ($inventory.working_set_kb / $default.working_set_kb - 1.0)
                peak_delta_pct = 100.0 * ($inventory.peak_working_set_kb / $default.peak_working_set_kb - 1.0)
                private_delta_pct = 100.0 * ($inventory.private_usage_kb / $default.private_usage_kb - 1.0)
            })
        }
    }
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz8_transition_inventory_matched_remote.md")
$RawPath = Join-Path $OutputDir ($Stamp + "_hz8_transition_inventory_matched_remote.log")
$CsvPath = Join-Path $OutputDir ($Stamp + "_hz8_transition_inventory_matched_remote.csv")
$Samples | Export-Csv -Path $CsvPath -NoTypeInformation -Encoding UTF8
Set-Content -Path $RawPath -Value $RawLines -Encoding UTF8

$defaultSamples = @($Samples | Where-Object { $_.allocator -eq "default" })
$inventorySamples = @($Samples | Where-Object { $_.allocator -eq "inventory" })
$defaultOps = Get-Median ([double[]]$defaultSamples.ops_sec)
$inventoryOps = Get-Median ([double[]]$inventorySamples.ops_sec)
$defaultPost = Get-Median ([double[]]$defaultSamples.working_set_kb)
$inventoryPost = Get-Median ([double[]]$inventorySamples.working_set_kb)
$defaultPeak = Get-Median ([double[]]$defaultSamples.peak_working_set_kb)
$inventoryPeak = Get-Median ([double[]]$inventorySamples.peak_working_set_kb)
$defaultPrivate = Get-Median ([double[]]$defaultSamples.private_usage_kb)
$inventoryPrivate = Get-Median ([double[]]$inventorySamples.private_usage_kb)
$pairedThroughput = Get-Median ([double[]]$AdmissiblePairs.throughput_delta_pct)
$pairedPost = Get-Median ([double[]]$AdmissiblePairs.working_set_delta_pct)
$pairedPeak = Get-Median ([double[]]$AdmissiblePairs.peak_delta_pct)
$pairedPrivate = Get-Median ([double[]]$AdmissiblePairs.private_delta_pct)
$GatePass = $AdmissiblePairs.Count -ge 10 -and $pairedThroughput -ge -3.0 -and
    $pairedPost -le 5.0 -and $pairedPeak -le 5.0 -and $pairedPrivate -le 5.0

$Summary = New-Object System.Collections.Generic.List[string]
$Summary.Add("# HZ8 Small Transition Inventory Matched Remote Gate")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add('- mode: fresh-process `A-B-B-A`, matched remote publication, no ring-full local fallback')
$Summary.Add(('- blocks: `{0}`; samples/lane: `{1}`; admissible pairs: `{2}`' -f $Blocks, ($Blocks * 2), $AdmissiblePairs.Count))
$Summary.Add('- allocator behavior flags: unchanged')
$Summary.Add("")
$Summary.Add("| allocator | median ops/s | actual remote | fallback | post working set | peak working set | private usage |")
$Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
$Summary.Add(('| default | {0:N3}M | {1:N2}% | {2:N2}% | {3:N0} KiB | {4:N0} KiB | {5:N0} KiB |' -f ($defaultOps / 1000000.0), (Get-Median ([double[]]$defaultSamples.actual_remote_pct)), (Get-Median ([double[]]$defaultSamples.fallback_pct)), $defaultPost, $defaultPeak, $defaultPrivate))
$Summary.Add(('| inventory | {0:N3}M | {1:N2}% | {2:N2}% | {3:N0} KiB | {4:N0} KiB | {5:N0} KiB |' -f ($inventoryOps / 1000000.0), (Get-Median ([double[]]$inventorySamples.actual_remote_pct)), (Get-Median ([double[]]$inventorySamples.fallback_pct)), $inventoryPost, $inventoryPeak, $inventoryPrivate))
$Summary.Add("")
$Summary.Add(('- independent-median throughput delta: `{0:+0.00;-0.00;0.00}%`' -f (100.0 * ($inventoryOps / $defaultOps - 1.0))))
$Summary.Add(('- paired median throughput delta: `{0:+0.00;-0.00;0.00}%`' -f $pairedThroughput))
$Summary.Add(('- paired median post/peak/private deltas: `{0:+0.00;-0.00;0.00}% / {1:+0.00;-0.00;0.00}% / {2:+0.00;-0.00;0.00}%`' -f $pairedPost, $pairedPeak, $pairedPrivate))
$Summary.Add(('- promotion gate: `{0}`' -f $(if ($GatePass) { "PASS" } else { "HOLD" })))
$Summary.Add("")
$Summary.Add(('Artifacts: `{0}`, `{1}`' -f
    (Split-Path -Leaf $CsvPath), (Split-Path -Leaf $RawPath)))

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
Write-Host ($Summary -join "`n")
