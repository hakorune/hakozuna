param(
    [ValidateSet("Compare", "Hz12AA", "AccountingAB", "CounterAB", "SpeedCompare", "RoutingAB", "OwnerMapAB", "OwnerFastCompare")]
    [string]$Mode = "Compare",
    [string]$OutputPath,
    [int]$Rounds = 5,
    [int]$RuntimeSeconds = 5,
    [int]$Producers = 4,
    [int]$Consumers = 4,
    [int]$RingCapacity = 4096,
    [int]$MinSize = 8,
    [int]$MaxSize = 1024,
    [UInt64]$AffinityMask = 0xFF,
    [ValidateSet("Normal", "AboveNormal", "High")]
    [string]$PriorityClass = "High",
    [int]$CooldownMilliseconds = 250,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz12Root
$Hz11Suite = Join-Path $RepoRoot "out_win_xowner"
$Hz11Build = Join-Path $RepoRoot "win\build_win_xowner_pipeline.ps1"
$Hz12Build = Join-Path $PSScriptRoot "build_hz12_windows_shadow.ps1"
$Hz12Suite = Join-Path $Hz12Root "out_win_shadow"
$TcDll = Join-Path $Hz11Suite "tcmalloc_minimal.dll"

if ($Rounds -lt 1) { throw "Rounds must be positive." }
if ($RuntimeSeconds -lt 1) { throw "RuntimeSeconds must be positive." }
if ($AffinityMask -eq 0) { throw "AffinityMask must select at least one CPU." }
if ($CooldownMilliseconds -lt 0) { throw "CooldownMilliseconds cannot be negative." }

if (-not $SkipBuild) {
    if ($Mode -eq "Compare" -or $Mode -eq "SpeedCompare" -or
        $Mode -eq "OwnerFastCompare") {
        & $Hz11Build
        if ($LASTEXITCODE -ne 0) { throw "HZ11/tcmalloc build failed: $LASTEXITCODE" }
    }
    & $Hz12Build
    if ($LASTEXITCODE -ne 0) { throw "HZ12 build failed: $LASTEXITCODE" }
}

$rowCatalog = @{
    "hz11-ownerless" = @{
        Path = (Join-Path $Hz11Suite "bench_xowner_hz11.exe")
        Pattern = "\[XOWNER_PIPELINE\]"
        Flags = "/O2 /DNDEBUG HZ_BENCH_USE_HZ11=1 HZ11_CLASSIFY_SPAN=1 HZ11_CACHE_CAP=32(default)"
    }
    "hz12-owner-inbox-diag" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_inbox.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "/O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=on diag_counters=on"
    }
    "hz12-owner-inbox-diag-a" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_inbox.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "A/A alias of hz12-owner-inbox-diag"
    }
    "hz12-owner-inbox-diag-b" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_inbox.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "A/A alias of hz12-owner-inbox-diag"
    }
    "hz12-owner-inbox-noaccounting" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_inbox_noaccounting.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "/O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=on"
    }
    "hz12-owner-inbox-speed" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_inbox_speed.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "/O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 shadow=on accounting=off diag_counters=off"
    }
    "hz12-bare-core" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_bare_core.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "/O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 bare_core=on owner_mapping=off inbox=off accounting=off diag_counters=off"
    }
    "hz12-owner-inbox-ownerfastload" = @{
        Path = (Join-Path $Hz12Suite "bench_hz12_xowner_inbox_ownerfastload.exe")
        Pattern = "\[HZ12_XOWNER_INBOX\]"
        Flags = "/O2 /DNDEBUG HZ12_CLASSIFY_SPAN=1 HZ12_CACHE_CAP=256 HZ12_INBOX_CAP=1024 HZ12_DRAIN_INTERVAL=256 accounting=off diag_counters=off owner_fast_load=on"
    }
    "tcmalloc" = @{
        Path = (Join-Path $Hz11Suite "bench_xowner_tcmalloc.exe")
        Pattern = "\[XOWNER_PIPELINE\]"
        Flags = "/O2 /DNDEBUG HZ_BENCH_USE_TCMALLOC=1 gperftools=2.16 tcmalloc_minimal.dll"
    }
}

$rowNames = switch ($Mode) {
    "Compare" { @("hz11-ownerless", "hz12-owner-inbox-diag", "tcmalloc") }
    "Hz12AA" { @("hz12-owner-inbox-diag-a", "hz12-owner-inbox-diag-b") }
    "AccountingAB" { @("hz12-owner-inbox-diag", "hz12-owner-inbox-noaccounting") }
    "CounterAB" { @("hz12-owner-inbox-noaccounting", "hz12-owner-inbox-speed") }
    "SpeedCompare" { @("hz11-ownerless", "hz12-owner-inbox-speed", "tcmalloc") }
    "RoutingAB" { @("hz12-owner-inbox-speed", "hz12-bare-core") }
    "OwnerMapAB" { @("hz12-owner-inbox-speed", "hz12-owner-inbox-ownerfastload") }
    "OwnerFastCompare" { @("hz12-owner-inbox-ownerfastload", "tcmalloc") }
}

foreach ($name in $rowNames) {
    if (-not (Test-Path $rowCatalog[$name].Path)) {
        throw "Missing benchmark artifact for ${name}: $($rowCatalog[$name].Path)"
    }
}
if (($Mode -eq "Compare" -or $Mode -eq "SpeedCompare" -or
     $Mode -eq "OwnerFastCompare") -and -not (Test-Path $TcDll)) {
    throw "Missing tcmalloc runtime: $TcDll"
}

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputPath = Join-Path $Hz12Root "bench_results\windows_xowner_roundrobin_${Mode}_$stamp.md"
}
New-Item -ItemType Directory -Force (Split-Path -Parent $OutputPath) | Out-Null

function Get-Median([double[]]$Values) {
    $sorted = @($Values | Sort-Object)
    $middle = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

function Get-FileManifest([string]$Name, [string]$Path, [string]$Flags) {
    $item = Get-Item $Path
    $hash = Get-FileHash $Path -Algorithm SHA256
    return [pscustomobject]@{
        Name = $Name
        Path = $item.FullName
        Bytes = $item.Length
        LastWrite = $item.LastWriteTime.ToString("o")
        Sha256 = $hash.Hash
        Flags = $Flags
    }
}

function Invoke-AffinedBenchmark([string]$Path, [string[]]$Arguments,
                                 [UInt64]$Mask) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Path
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) { [void]$startInfo.ArgumentList.Add($argument) }
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) { throw "Failed to start $Path" }
    try {
        $process.ProcessorAffinity = [IntPtr]([Int64]$Mask)
        $process.PriorityClass = [System.Diagnostics.ProcessPriorityClass]$PriorityClass
    } catch {
        try { $process.Kill($true) } catch {}
        throw "Failed to apply affinity mask 0x$($Mask.ToString('X')) to ${Path}: $_"
    }
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        throw "Benchmark failed rc=$($process.ExitCode): $Path`n$stdout`n$stderr"
    }
    return ($stdout + "`n" + $stderr)
}

$manifests = @()
foreach ($name in $rowNames) {
    $row = $rowCatalog[$name]
    $manifests += Get-FileManifest $name $row.Path $row.Flags
}
if ($Mode -eq "Compare" -or $Mode -eq "SpeedCompare" -or
    $Mode -eq "OwnerFastCompare") {
    $manifests += Get-FileManifest "tcmalloc_minimal.dll" $TcDll "gperftools 2.16 minimal runtime"
}

$gitHead = (& git -C $RepoRoot rev-parse HEAD).Trim()
$compiler = ((& clang-cl --version | Select-Object -First 1) -join " ").Trim()
$logicalCpuCount = [Environment]::ProcessorCount
$arguments = @("$RuntimeSeconds", "$Producers", "$Consumers", "$RingCapacity", "$MinSize", "$MaxSize")
$env:Path = $Hz11Suite + ";" + $env:Path
$results = @()

for ($round = 0; $round -lt $Rounds; ++$round) {
    for ($position = 0; $position -lt $rowNames.Count; ++$position) {
        $index = ($position + $round) % $rowNames.Count
        $name = $rowNames[$index]
        $row = $rowCatalog[$name]
        $output = Invoke-AffinedBenchmark $row.Path $arguments $AffinityMask
        $line = (($output -split "`r?`n") |
            Select-String $row.Pattern | Select-Object -First 1).Line
        if (-not $line -or $line -notmatch 'ops/s=([0-9.]+)') {
            throw "Cannot parse result for $name in round $($round + 1)."
        }
        $ops = [double]$Matches[1]
        $results += [pscustomobject]@{
            Round = $round + 1
            Position = $position + 1
            Name = $name
            Ops = $ops
            Raw = $line
        }
        Write-Host "[HZ12_ROUNDROBIN] round=$($round + 1) position=$($position + 1) allocator=$name $line"
        if ($CooldownMilliseconds -ne 0) { Start-Sleep -Milliseconds $CooldownMilliseconds }
    }
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("# HZ12 Windows Cross-Owner Round-Robin $Mode")
$lines.Add("")
$lines.Add("- Generated: $(Get-Date -Format o)")
$lines.Add("- Git HEAD: ``$gitHead``")
$lines.Add("- Compiler: ``$compiler``")
$lines.Add("- Rounds: $Rounds; runtime: ${RuntimeSeconds}s")
$lines.Add("- Producers/consumers: $Producers/$Consumers; ring: $RingCapacity")
$lines.Add("- Size: $MinSize..$MaxSize; cross-owner free target: 100%")
$lines.Add("- Logical CPUs: $logicalCpuCount; affinity mask: ``0x$($AffinityMask.ToString('X'))``")
$lines.Add("- Priority class: $PriorityClass")
$lines.Add("- Order: rotated once per round; cooldown: ${CooldownMilliseconds}ms")
$lines.Add("")
$lines.Add("## Build Manifest")
$lines.Add("")
$lines.Add("| Row | Bytes | Last write | SHA-256 | Build flags / identity |")
$lines.Add("| --- | ---: | --- | --- | --- |")
foreach ($manifest in $manifests) {
    $lines.Add("| $($manifest.Name) | $($manifest.Bytes) | $($manifest.LastWrite) | ``$($manifest.Sha256)`` | $($manifest.Flags) |")
}
$lines.Add("")
$lines.Add("## Median")
$lines.Add("")
$lines.Add("| Allocator | Median ops/s | Min | Max | Runs |")
$lines.Add("| --- | ---: | ---: | ---: | ---: |")
foreach ($name in $rowNames) {
    $values = @($results | Where-Object Name -eq $name | ForEach-Object Ops)
    $lines.Add(("| {0} | {1:N3}M | {2:N3}M | {3:N3}M | {4} |" -f
        $name, ((Get-Median $values) / 1000000.0),
        (($values | Measure-Object -Minimum).Minimum / 1000000.0),
        (($values | Measure-Object -Maximum).Maximum / 1000000.0),
        $values.Count))
}

if ($Mode -eq "Hz12AA") {
    $deltas = @()
    for ($round = 1; $round -le $Rounds; ++$round) {
        $a = ($results | Where-Object { $_.Round -eq $round -and $_.Name -eq "hz12-owner-inbox-diag-a" }).Ops
        $b = ($results | Where-Object { $_.Round -eq $round -and $_.Name -eq "hz12-owner-inbox-diag-b" }).Ops
        $deltas += (($b / $a) - 1.0) * 100.0
    }
    $lines.Add("")
    $lines.Add("## A/A Band")
    $lines.Add("")
    $lines.Add(("- B/A median delta: {0:N2}%" -f (Get-Median $deltas)))
    $lines.Add(("- B/A min/max delta: {0:N2}% / {1:N2}%" -f
        (($deltas | Measure-Object -Minimum).Minimum),
        (($deltas | Measure-Object -Maximum).Maximum)))
}

$lines.Add("")
$lines.Add("## Raw Runs")
$lines.Add("")
$lines.Add("| Round | Position | Allocator | ops/s | Raw |")
$lines.Add("| ---: | ---: | --- | ---: | --- |")
foreach ($result in $results) {
    $raw = $result.Raw.Replace("|", "\\|")
    $lines.Add("| $($result.Round) | $($result.Position) | $($result.Name) | $($result.Ops) | ``$raw`` |")
}

[IO.File]::WriteAllLines($OutputPath, $lines)
Write-Host "Wrote $OutputPath"
