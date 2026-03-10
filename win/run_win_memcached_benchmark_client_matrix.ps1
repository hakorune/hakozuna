[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExePath,
    [int]$ServerThreads = 4,
    [int]$MemMb = 64,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\benchmark_client_matrix"
}
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot "out_win_memcached\memcached_win_min_main.exe"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

$Runner = Join-Path $RepoRoot "win\run_win_memcached_benchmark_client.ps1"
$Profiles = @(
    @{
        Name = "read_heavy"
        Port = 11450
        Clients = 8
        WarmupKeysPerClient = 1000
        RequestsPerClient = 10000
        PayloadSize = 64
        GetPercent = 90
    },
    @{
        Name = "balanced"
        Port = 11451
        Clients = 8
        WarmupKeysPerClient = 1000
        RequestsPerClient = 10000
        PayloadSize = 64
        GetPercent = 50
    },
    @{
        Name = "write_heavy"
        Port = 11452
        Clients = 8
        WarmupKeysPerClient = 1000
        RequestsPerClient = 10000
        PayloadSize = 64
        GetPercent = 20
    },
    @{
        Name = "larger_payload"
        Port = 11453
        Clients = 8
        WarmupKeysPerClient = 800
        RequestsPerClient = 8000
        PayloadSize = 256
        GetPercent = 80
    }
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_benchmark_client_matrix.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_benchmark_client_matrix.log")
$summary = New-Object System.Collections.Generic.List[string]
$raw = New-Object System.Collections.Generic.List[string]

$summary.Add("# Windows Memcached Benchmark Client Matrix")
$summary.Add("")
$summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$summary.Add("")
$summary.Add("References:")
$summary.Add("- [win/run_win_memcached_benchmark_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client.ps1)")
$summary.Add("- [win/run_win_memcached_benchmark_client_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_benchmark_client_matrix.ps1)")
$summary.Add("- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)")
$summary.Add("")
$summary.Add("| profile | total ops | get ops | set ops | ops/sec | clients | req/client | payload | get % |")
$summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")

$builtOnce = $false
foreach ($profile in $Profiles) {
    $profileOutputDir = Join-Path $OutputDir ("benchmark_client_" + $profile.Name)
    $profileRawDir = Join-Path $RawOutputDir ("benchmark_client_" + $profile.Name)

    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $Runner,
        "-Profile", $profile.Name,
        "-OutputDir", $profileOutputDir,
        "-RawOutputDir", $profileRawDir,
        "-ExePath", $ExePath,
        "-Port", [string]$profile.Port,
        "-ServerThreads", [string]$ServerThreads,
        "-MemMb", [string]$MemMb,
        "-Clients", [string]$profile.Clients,
        "-WarmupKeysPerClient", [string]$profile.WarmupKeysPerClient,
        "-RequestsPerClient", [string]$profile.RequestsPerClient,
        "-PayloadSize", [string]$profile.PayloadSize,
        "-GetPercent", [string]$profile.GetPercent
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
        throw "memcached benchmark-client profile failed: $($profile.Name)"
    }

    $profileSummary = Get-ChildItem $profileOutputDir -Filter "*_memcached_benchmark_client.md" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $profileSummary) {
        throw "benchmark-client summary not found: $($profile.Name)"
    }

    $totalOps = ""
    $getOps = ""
    $setOps = ""
    $opsPerSec = ""
    foreach ($line in (Get-Content $profileSummary.FullName)) {
        if ($line -match '^\| [^|]+ \| ([0-9,]+) \| ([0-9,]+) \| ([0-9,]+) \| [0-9,]+\.[0-9]+ \| ([0-9,]+\.[0-9]+) \|$') {
            $totalOps = $Matches[1]
            $getOps = $Matches[2]
            $setOps = $Matches[3]
            $opsPerSec = $Matches[4]
            break
        }
    }
    if (-not $opsPerSec) {
        throw "benchmark-client metrics not found in summary: $($profileSummary.FullName)"
    }

    $summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} |' -f `
        $profile.Name, $totalOps, $getOps, $setOps, $opsPerSec,
        $profile.Clients, $profile.RequestsPerClient, $profile.PayloadSize, $profile.GetPercent))
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
