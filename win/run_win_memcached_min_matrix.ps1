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
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\matrix"
}
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot "out_win_memcached\memcached_win_min_main.exe"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

$Runner = Join-Path $RepoRoot "win\run_win_memcached_min_throughput.ps1"
$Profiles = @(
    @{
        Name = "smoke"
        Port = 11420
        Clients = 2
        RequestsPerClient = 1000
        PayloadSize = 16
    },
    @{
        Name = "balanced"
        Port = 11421
        Clients = 4
        RequestsPerClient = 2000
        PayloadSize = 32
    },
    @{
        Name = "higher_clients"
        Port = 11422
        Clients = 8
        RequestsPerClient = 2000
        PayloadSize = 32
    },
    @{
        Name = "larger_payload"
        Port = 11423
        Clients = 4
        RequestsPerClient = 1500
        PayloadSize = 256
    }
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_min_matrix.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_matrix.log")
$summary = New-Object System.Collections.Generic.List[string]
$raw = New-Object System.Collections.Generic.List[string]

$summary.Add("# Windows Memcached Min Matrix")
$summary.Add("")
$summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$summary.Add("")
$summary.Add("References:")
$summary.Add("- [win/run_win_memcached_min_throughput.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_throughput.ps1)")
$summary.Add("- [win/run_win_memcached_min_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_matrix.ps1)")
$summary.Add("- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)")
$summary.Add("")
$summary.Add("| profile | set ops/sec | get ops/sec | clients | req/client | payload |")
$summary.Add("| --- | ---: | ---: | ---: | ---: | ---: |")

foreach ($profile in $Profiles) {
    $profileOutputDir = Join-Path $OutputDir ("matrix_" + $profile.Name)
    $profileRawDir = Join-Path $RawOutputDir ("matrix_" + $profile.Name)

    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $Runner,
        "-OutputDir", $profileOutputDir,
        "-RawOutputDir", $profileRawDir,
        "-ExePath", $ExePath,
        "-Port", [string]$profile.Port,
        "-ServerThreads", [string]$ServerThreads,
        "-MemMb", [string]$MemMb,
        "-Clients", [string]$profile.Clients,
        "-RequestsPerClient", [string]$profile.RequestsPerClient,
        "-PayloadSize", [string]$profile.PayloadSize
    )
    if ($SkipBuild) {
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
        throw "memcached throughput profile failed: $($profile.Name)"
    }

    $profileSummary = Get-ChildItem $profileOutputDir -Filter "*_memcached_min_throughput.md" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $profileSummary) {
        throw "profile summary not found: $($profile.Name)"
    }

    $setOps = ""
    $getOps = ""
    foreach ($line in (Get-Content $profileSummary.FullName)) {
        if ($line -match '^\| set \| .* \| ([0-9,]+\.[0-9]+) \|$') {
            $setOps = $Matches[1]
        }
        if ($line -match '^\| get \| .* \| ([0-9,]+\.[0-9]+) \|$') {
            $getOps = $Matches[1]
        }
    }

    $summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} |' -f `
        $profile.Name, $setOps, $getOps, $profile.Clients, $profile.RequestsPerClient, $profile.PayloadSize))
}

$summary.Add("")
$summary.Add(('Raw log: `{0}`' -f $RawLogPath))

Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8

Write-Host "Wrote summary: $SummaryPath"
Write-Host "Wrote raw log: $RawLogPath"
