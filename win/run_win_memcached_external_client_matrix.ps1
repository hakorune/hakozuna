[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExePath,
    [string]$ClientExePath,
    [int]$ServerThreads = 4,
    [int]$MemMb = 64,
    [int]$ServerVerbose = 1,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\external_client_matrix"
}
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot "out_win_memcached\memcached_win_min_main.exe"
}
if (-not $ClientExePath) {
    $ClientExePath = Join-Path $RepoRoot "out_win_memtier\memtier_benchmark.exe"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

$Runner = Join-Path $RepoRoot "win\run_win_memcached_external_client.ps1"
$Profiles = @(
    @{
        Name = "balanced_1x64"
        Port = 11460
        ClientThreads = 1
        ClientConnections = 64
        Ratio = "1:1"
        DataSize = 64
        TestTime = 2
        KeyMaximum = 10000
    },
    @{
        Name = "balanced_2x32"
        Port = 11461
        ClientThreads = 2
        ClientConnections = 32
        Ratio = "1:1"
        DataSize = 64
        TestTime = 2
        KeyMaximum = 10000
    },
    @{
        Name = "balanced_4x16"
        Port = 11462
        ClientThreads = 4
        ClientConnections = 16
        Ratio = "1:1"
        DataSize = 64
        TestTime = 2
        KeyMaximum = 10000
    },
    @{
        Name = "read_heavy_4x16"
        Port = 11463
        ClientThreads = 4
        ClientConnections = 16
        Ratio = "1:9"
        DataSize = 64
        TestTime = 3
        KeyMaximum = 10000
    },
    @{
        Name = "write_heavy_4x16"
        Port = 11464
        ClientThreads = 4
        ClientConnections = 16
        Ratio = "4:1"
        DataSize = 64
        TestTime = 3
        KeyMaximum = 10000
    },
    @{
        Name = "larger_payload_4x16"
        Port = 11465
        ClientThreads = 4
        ClientConnections = 16
        Ratio = "1:1"
        DataSize = 256
        TestTime = 3
        KeyMaximum = 8000
    },
    @{
        Name = "long_balanced_4x16"
        Port = 11466
        ClientThreads = 4
        ClientConnections = 16
        Ratio = "1:1"
        DataSize = 64
        TestTime = 10
        KeyMaximum = 10000
    },
    @{
        Name = "scale_8x16"
        Port = 11467
        ClientThreads = 8
        ClientConnections = 16
        Ratio = "1:1"
        DataSize = 64
        TestTime = 5
        KeyMaximum = 20000
    }
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_external_client_matrix.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client_matrix.log")
$summary = New-Object System.Collections.Generic.List[string]
$raw = New-Object System.Collections.Generic.List[string]

$summary.Add("# Windows Memcached External Client Matrix")
$summary.Add("")
$summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$summary.Add("")
$summary.Add("References:")
$summary.Add("- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)")
$summary.Add("- [win/run_win_memcached_external_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client_matrix.ps1)")
$summary.Add("- [docs/WINDOWS_BUILD.md](/C:/git/hakozuna-win/docs/WINDOWS_BUILD.md)")
$summary.Add("")
$summary.Add("| profile | ops/sec | drop count | slot count | threads | conns/thread | ratio | size | secs |")
$summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |")

$builtOnce = $false
foreach ($profile in $Profiles) {
    $profileOutputDir = Join-Path $OutputDir ("external_client_" + $profile.Name)
    $profileRawDir = Join-Path $RawOutputDir ("external_client_" + $profile.Name)

    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $Runner,
        "-OutputDir", $profileOutputDir,
        "-RawOutputDir", $profileRawDir,
        "-ExePath", $ExePath,
        "-ClientExePath", $ClientExePath,
        "-Port", [string]$profile.Port,
        "-ServerThreads", [string]$ServerThreads,
        "-MemMb", [string]$MemMb,
        "-ServerVerbose", [string]$ServerVerbose,
        "-ClientThreads", [string]$profile.ClientThreads,
        "-ClientConnections", [string]$profile.ClientConnections,
        "-Ratio", $profile.Ratio,
        "-DataSize", [string]$profile.DataSize,
        "-TestTime", [string]$profile.TestTime,
        "-KeyMaximum", [string]$profile.KeyMaximum
    )
    if ($SkipBuild -or $builtOnce) {
        $args += "-SkipBuild"
    }

    $output = & powershell @args 2>&1
    $rc = $LASTEXITCODE

    $raw.Add(("=== profile {0} ===" -f $profile.Name))
    $raw.Add("command: powershell " + ($args -join " "))
    $raw.Add("rc: $rc")
    foreach ($line in $output) {
        $raw.Add($line.ToString())
    }
    $raw.Add("")

    if ($rc -ne 0) {
        throw "memcached external-client profile failed: $($profile.Name)"
    }

    $profileSummary = Get-ChildItem $profileOutputDir -Filter "*_memcached_external_client.md" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    $profileRaw = Get-ChildItem $profileRawDir -Filter "*_memcached_external_client.log" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $profileSummary -or -not $profileRaw) {
        throw "external-client profile outputs not found: $($profile.Name)"
    }

    $opsPerSec = ""
    foreach ($line in (Get-Content $profileSummary.FullName)) {
        if ($line -match '^\- `Totals\s+([0-9,]+\.[0-9]+)\s') {
            $opsPerSec = $Matches[1]
            break
        }
    }
    if (-not $opsPerSec) {
        throw "external-client ops/sec not found in summary: $($profileSummary.FullName)"
    }

    $dropCount = ""
    $slotCount = ""
    foreach ($line in (Get-Content $profileRaw.FullName)) {
        if ($line -match '^client_connection_dropped_count:\s+(\d+)$') {
            $dropCount = $Matches[1]
        }
        if ($line -match '^server_slot_count:\s+(\d+)$') {
            $slotCount = $Matches[1]
        }
    }
    if ($dropCount -eq "") {
        $dropCount = "?"
    }
    if ($slotCount -eq "") {
        $slotCount = "?"
    }

    $summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | `{6}` | {7} | {8} |' -f `
        $profile.Name, $opsPerSec, $dropCount, $slotCount,
        $profile.ClientThreads, $profile.ClientConnections, $profile.Ratio,
        $profile.DataSize, $profile.TestTime))
    $builtOnce = $true
}

$summary.Add("")
$summary.Add(('Raw log: `{0}`' -f $RawLogPath))

Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8

Write-Host "Wrote summary: $SummaryPath"
Write-Host "Wrote raw log: $RawLogPath"

[pscustomobject]@{
    SummaryPath = $SummaryPath
    RawLogPath = $RawLogPath
}
