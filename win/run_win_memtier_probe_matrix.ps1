[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\client_probe_matrix"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force -Path $path | Out-Null
    }
}

$Runner = Join-Path $RepoRoot "win\probe_win_memtier_msvc.ps1"
$Units = @(
    "memtier_benchmark.cpp",
    "client.cpp",
    "protocol.cpp",
    "config_types.cpp",
    "shard_connection.cpp"
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memtier_probe_matrix.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memtier_probe_matrix.log")
$summary = New-Object System.Collections.Generic.List[string]
$raw = New-Object System.Collections.Generic.List[string]

$summary.Add("# Windows Memtier Probe Matrix")
$summary.Add("")
$summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$summary.Add("")
$summary.Add("References:")
$summary.Add("- [win/probe_win_memtier_msvc.ps1](/C:/git/hakozuna-win/win/probe_win_memtier_msvc.ps1)")
$summary.Add("- [win/run_win_memtier_probe_matrix.ps1](/C:/git/hakozuna-win/win/run_win_memtier_probe_matrix.ps1)")
$summary.Add("- [docs/WINDOWS_MEMTIER_PROBE.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMTIER_PROBE.md)")
$summary.Add("")
$summary.Add("| translation unit | status | latest log |")
$summary.Add("| --- | --- | --- |")

foreach ($unit in $Units) {
    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $Runner,
        "-TranslationUnit", $unit
    )
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    $output = & powershell @args 2>&1
    $rc = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference

    $raw.Add(("=== {0} ===" -f $unit))
    $raw.Add("command: powershell " + ($args -join " "))
    $raw.Add("rc: $rc")
    foreach ($line in $output) {
        $raw.Add($line.ToString())
    }
    $raw.Add("")

    $logLine = $output | Where-Object { $_.ToString().StartsWith("Wrote log: ") } | Select-Object -Last 1
    $logPath = ""
    if ($logLine) {
        $logPath = $logLine.ToString().Substring("Wrote log: ".Length)
    }

    $status = if ($rc -eq 0) { "OK" } else { "BLOCKED" }
    $summary.Add(('| {0} | {1} | `{2}` |' -f $unit, $status, $logPath))
}

$summary.Add("")
$summary.Add(('Raw log: `{0}`' -f $RawLogPath))

Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8

Write-Host "Wrote summary: $SummaryPath"
Write-Host "Wrote raw log: $RawLogPath"
