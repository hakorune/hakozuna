[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExeDir,
    [string]$ClientExePath,
    [int]$ServerThreads = 4,
    [int]$MemMb = 64,
    [int]$ServerVerbose = 1,
    [int]$Runs = 1,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\external_client_allocator_matrix"
}
if (-not $ExeDir) {
    $ExeDir = Join-Path $RepoRoot "out_win_memcached_allocators"
}
if (-not $ClientExePath) {
    $ClientExePath = Join-Path $RepoRoot "out_win_memtier\memtier_benchmark.exe"
}
if ($Runs -lt 1) {
    throw "Runs must be >= 1"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

if (-not $SkipBuild) {
    & (Join-Path $RepoRoot "win\build_win_memcached_allocator_variants.ps1") | Out-Null
}

$Runner = Join-Path $RepoRoot "win\run_win_memcached_external_client.ps1"
$Allocators = @(
    @{ Name = "crt"; Exe = (Join-Path $ExeDir "memcached_win_min_main_crt.exe") },
    @{ Name = "hz3"; Exe = (Join-Path $ExeDir "memcached_win_min_main_hz3.exe") },
    @{ Name = "hz4"; Exe = (Join-Path $ExeDir "memcached_win_min_main_hz4.exe") },
    @{ Name = "mimalloc"; Exe = (Join-Path $ExeDir "memcached_win_min_main_mimalloc.exe") },
    @{ Name = "tcmalloc"; Exe = (Join-Path $ExeDir "memcached_win_min_main_tcmalloc.exe") }
)
$Profiles = @(
    @{ Name = "balanced_1x64"; Port = 11460; ClientThreads = 1; ClientConnections = 64; Ratio = "1:1"; DataSize = 64; TestTime = 2; KeyMaximum = 10000 },
    @{ Name = "balanced_2x32"; Port = 11465; ClientThreads = 2; ClientConnections = 32; Ratio = "1:1"; DataSize = 64; TestTime = 2; KeyMaximum = 10000 },
    @{ Name = "balanced_4x16"; Port = 11470; ClientThreads = 4; ClientConnections = 16; Ratio = "1:1"; DataSize = 64; TestTime = 3; KeyMaximum = 10000 },
    @{ Name = "read_heavy_4x16"; Port = 11475; ClientThreads = 4; ClientConnections = 16; Ratio = "1:9"; DataSize = 64; TestTime = 3; KeyMaximum = 10000 },
    @{ Name = "write_heavy_4x16"; Port = 11480; ClientThreads = 4; ClientConnections = 16; Ratio = "4:1"; DataSize = 64; TestTime = 3; KeyMaximum = 10000 },
    @{ Name = "larger_payload_4x16"; Port = 11482; ClientThreads = 4; ClientConnections = 16; Ratio = "1:1"; DataSize = 256; TestTime = 3; KeyMaximum = 8000 },
    @{ Name = "long_balanced_4x16"; Port = 11485; ClientThreads = 4; ClientConnections = 16; Ratio = "1:1"; DataSize = 64; TestTime = 10; KeyMaximum = 10000 },
    @{ Name = "scale_8x16"; Port = 11490; ClientThreads = 8; ClientConnections = 16; Ratio = "1:1"; DataSize = 64; TestTime = 5; KeyMaximum = 20000 }
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_external_client_allocator_matrix.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client_allocator_matrix.log")
$summary = New-Object System.Collections.Generic.List[string]
$raw = New-Object System.Collections.Generic.List[string]
$details = New-Object System.Collections.Generic.List[string]

function Get-MedianValue {
    param(
        [Parameter(Mandatory = $true)]
        [double[]]$Values
    )

    if ($Values.Count -eq 0) {
        throw "median requested for empty set"
    }

    $sorted = $Values | Sort-Object
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

$summary.Add("# Windows Memcached External Client Allocator Matrix")
$summary.Add("")
$summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$summary.Add("Runs per profile/allocator: $Runs")
$summary.Add("")
$summary.Add("References:")
$summary.Add("- [win/build_win_memcached_allocator_variants.ps1](/C:/git/hakozuna-win/win/build_win_memcached_allocator_variants.ps1)")
$summary.Add("- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)")
$summary.Add("- [win/run_win_memcached_external_client_allocator_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_allocator_matrix.ps1)")
$summary.Add("")
$summary.Add("| profile | allocator | ops/sec median | drop median | slot median | size | secs | runs |")
$summary.Add("| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |")

foreach ($profile in $Profiles) {
    foreach ($allocator in $Allocators) {
        if (-not (Test-Path $allocator.Exe)) {
            throw "allocator exe not found: $($allocator.Exe)"
        }

        $profileOutputDir = Join-Path $OutputDir ("external_client_alloc_" + $profile.Name + "_" + $allocator.Name)
        $profileRawDir = Join-Path $RawOutputDir ("external_client_alloc_" + $profile.Name + "_" + $allocator.Name)
        $port = $profile.Port
        switch ($allocator.Name) {
            "hz3" { $port += 1 }
            "hz4" { $port += 2 }
            "mimalloc" { $port += 3 }
            "tcmalloc" { $port += 4 }
        }

        $opsValues = New-Object System.Collections.Generic.List[double]
        $dropValues = New-Object System.Collections.Generic.List[double]
        $slotValues = New-Object System.Collections.Generic.List[double]
        $opsFormatted = New-Object System.Collections.Generic.List[string]
        $dropFormatted = New-Object System.Collections.Generic.List[string]
        $slotFormatted = New-Object System.Collections.Generic.List[string]

        for ($runIndex = 1; $runIndex -le $Runs; $runIndex++) {
            $runnerParams = @{
                OutputDir = $profileOutputDir
                RawOutputDir = $profileRawDir
                ExePath = $allocator.Exe
                ClientExePath = $ClientExePath
                Port = $port
                ServerThreads = $ServerThreads
                MemMb = $MemMb
                ServerVerbose = $ServerVerbose
                ClientThreads = $profile.ClientThreads
                ClientConnections = $profile.ClientConnections
                Ratio = $profile.Ratio
                DataSize = $profile.DataSize
                TestTime = $profile.TestTime
                KeyMaximum = $profile.KeyMaximum
                SkipBuild = $true
            }

            $global:LASTEXITCODE = 0
            $output = & $Runner @runnerParams 2>&1
            $rc = $LASTEXITCODE

            $raw.Add(("=== profile {0} allocator {1} run {2}/{3} ===" -f $profile.Name, $allocator.Name, $runIndex, $Runs))
            $raw.Add(("command: {0} -OutputDir {1} -RawOutputDir {2} -ExePath {3} -ClientExePath {4} -Port {5} -ServerThreads {6} -MemMb {7} -ServerVerbose {8} -ClientThreads {9} -ClientConnections {10} -Ratio {11} -DataSize {12} -TestTime {13} -KeyMaximum {14} -SkipBuild" -f `
                $Runner, $profileOutputDir, $profileRawDir, $allocator.Exe, $ClientExePath, $port, $ServerThreads, $MemMb,
                $ServerVerbose, $profile.ClientThreads, $profile.ClientConnections, $profile.Ratio, $profile.DataSize,
                $profile.TestTime, $profile.KeyMaximum))
            $raw.Add("rc: $rc")
            foreach ($line in $output) {
                $raw.Add($line.ToString())
            }
            $raw.Add("")

            if ($rc -ne 0) {
                throw "memcached external-client allocator run failed: $($profile.Name) / $($allocator.Name) / run $runIndex"
            }

            $profileSummary = Get-ChildItem $profileOutputDir -Filter "*_memcached_external_client.md" |
                Sort-Object LastWriteTime -Descending |
                Select-Object -First 1
            $profileRaw = Get-ChildItem $profileRawDir -Filter "*_memcached_external_client.log" |
                Sort-Object LastWriteTime -Descending |
                Select-Object -First 1
            if (-not $profileSummary -or -not $profileRaw) {
                throw "allocator run outputs not found: $($profile.Name) / $($allocator.Name) / run $runIndex"
            }

            $opsPerSec = ""
            foreach ($line in (Get-Content $profileSummary.FullName)) {
                if ($line -match '^\- `Totals\s+([0-9,]+\.[0-9]+)\s') {
                    $opsPerSec = $Matches[1]
                    break
                }
            }
            if (-not $opsPerSec) {
                throw "allocator ops/sec not found in summary: $($profileSummary.FullName)"
            }

            $dropCount = "0"
            $slotCount = "0"
            foreach ($line in (Get-Content $profileRaw.FullName)) {
                if ($line -match '^client_connection_dropped_count:\s+(\d+)$') {
                    $dropCount = $Matches[1]
                }
                if ($line -match '^server_slot_count:\s+(\d+)$') {
                    $slotCount = $Matches[1]
                }
            }

            $opsValue = [double]($opsPerSec -replace ',', '')
            $dropValue = [double]$dropCount
            $slotValue = [double]$slotCount

            $opsValues.Add($opsValue)
            $dropValues.Add($dropValue)
            $slotValues.Add($slotValue)
            $opsFormatted.Add(("{0:N2}" -f $opsValue))
            $dropFormatted.Add([string][int]$dropValue)
            $slotFormatted.Add([string][int]$slotValue)
        }

        $opsMedian = Get-MedianValue -Values $opsValues.ToArray()
        $dropMedian = [int][Math]::Round((Get-MedianValue -Values $dropValues.ToArray()))
        $slotMedian = [int][Math]::Round((Get-MedianValue -Values $slotValues.ToArray()))

        $summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} |' -f `
            $profile.Name, $allocator.Name, ("{0:N2}" -f $opsMedian), $dropMedian, $slotMedian,
            $profile.DataSize, $profile.TestTime, $Runs))
        $details.Add(('- `{0} / {1}`: ops/sec [{2}] drop [{3}] slots [{4}]' -f `
            $profile.Name, $allocator.Name,
            ($opsFormatted -join ", "), ($dropFormatted -join ", "), ($slotFormatted -join ", ")))
    }
}

$summary.Add("")
$summary.Add("Per-run detail:")
foreach ($line in $details) {
    $summary.Add($line)
}
$summary.Add("")
$summary.Add(('Raw log: `{0}`' -f $RawLogPath))

Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8

Write-Host "Wrote summary: $SummaryPath"
Write-Host "Wrote raw log: $RawLogPath"
