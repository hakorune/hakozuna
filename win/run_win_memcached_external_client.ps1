[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExePath,
    [string]$ClientExePath,
    [int]$Port = 11460,
    [string]$Listen = "127.0.0.1",
    [int]$ServerThreads = 4,
    [int]$MemMb = 64,
    [int]$ServerVerbose = 1,
    [int]$ClientThreads = 4,
    [int]$ClientConnections = 16,
    [int]$KeyMaximum = 10000,
    [int]$DataSize = 64,
    [int]$TestTime = 10,
    [string]$Ratio = "1:1",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\external_client"
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

if (-not $SkipBuild) {
    $BuildScript = Join-Path $RepoRoot "win\build_win_memcached_min_main.ps1"
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_memcached_min_main.ps1 failed with exit code $LASTEXITCODE"
    }

    $ClientBuildScript = Join-Path $RepoRoot "win\build_win_memtier_benchmark.ps1"
    & $ClientBuildScript | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_memtier_benchmark.ps1 failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path $ExePath)) {
    throw "memcached minimal main exe not found: $ExePath"
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss_fff"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_external_client.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client.log")
$StdoutPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client_stdout.log")
$StderrPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client_stderr.log")
$ClientStdoutPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client_client_stdout.log")
$ClientStderrPath = Join-Path $RawOutputDir ($Stamp + "_memcached_external_client_client_stderr.log")

$summary = New-Object System.Collections.Generic.List[string]
$raw = New-Object System.Collections.Generic.List[string]
$server = $null

try {
    Remove-Item $StdoutPath, $StderrPath, $ClientStdoutPath, $ClientStderrPath -ErrorAction SilentlyContinue

    $serverArgs = @(
        "-p", [string]$Port,
        "-l", $Listen,
        "-t", [string]$ServerThreads,
        "-m", [string]$MemMb
    )
    for ($i = 0; $i -lt $ServerVerbose; $i++) {
        $serverArgs += "-v"
    }

    $summary.Add("# Windows Memcached External Client")
    $summary.Add("")
    $summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
    $summary.Add("")
    $summary.Add("References:")
    $summary.Add("- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)")
    $summary.Add("- [win/build_win_memtier_benchmark.ps1](/C:/git/hakozuna-win/win/build_win_memtier_benchmark.ps1)")
    $summary.Add("- [win/run_win_memcached_external_client.ps1](/C:/git/hakozuna-win/win/run_win_memcached_external_client.ps1)")
    $summary.Add("- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)")
    $summary.Add("")
    $summary.Add("Config:")
    $summary.Add(('- server exe: `{0}`' -f $ExePath))
    $summary.Add(('- client exe: `{0}`' -f $ClientExePath))
    $summary.Add(('- listen: `{0}:{1}`' -f $Listen, $Port))
    $summary.Add(('- server_threads: `{0}`' -f $ServerThreads))
    $summary.Add(('- server_verbose: `{0}`' -f $ServerVerbose))
    $summary.Add(('- client_threads: `{0}`' -f $ClientThreads))
    $summary.Add(('- client_connections: `{0}`' -f $ClientConnections))
    $summary.Add(('- ratio: `{0}`' -f $Ratio))
    $summary.Add(('- data_size: `{0}`' -f $DataSize))
    $summary.Add(('- key_maximum: `{0}`' -f $KeyMaximum))
    $summary.Add(('- test_time: `{0}`' -f $TestTime))
    $summary.Add("")

    if (-not (Test-Path $ClientExePath)) {
        $summary.Add('Status: `BLOCKED`')
        $summary.Add("")
        $summary.Add("Reason:")
        $summary.Add("- external client executable was not found in the public output lane")
        $summary.Add('- run the native Windows memtier build lane first, or omit `-SkipBuild`')
        $summary.Add("")
        $summary.Add("Expected path:")
        $summary.Add(('- `{0}`' -f $ClientExePath))
        $summary.Add("")
        $summary.Add(('Raw log: `{0}`' -f $RawLogPath))

        $raw.Add("status: BLOCKED")
        $raw.Add("reason: external client executable not found")
        $raw.Add("expected_client_exe: $ClientExePath")
        return
    }

    $VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\vcpkg" }
    $VcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"
    if (Test-Path $VcpkgBin) {
        $env:PATH = $VcpkgBin + ";" + $env:PATH
    }

    $server = Start-Process -FilePath $ExePath -ArgumentList $serverArgs `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 1

    $clientArgs = @(
        "--protocol=memcache_text",
        "--server=$Listen",
        "--port=$Port",
        "--threads=$ClientThreads",
        "--clients=$ClientConnections",
        "--ratio=$Ratio",
        "--data-size=$DataSize",
        "--key-maximum=$KeyMaximum",
        "--hide-histogram",
        "--test-time=$TestTime",
        "--run-count=1"
    )

    $client = Start-Process -FilePath $ClientExePath -ArgumentList $clientArgs `
        -RedirectStandardOutput $ClientStdoutPath -RedirectStandardError $ClientStderrPath -PassThru -Wait -WindowStyle Hidden

    if ($client.ExitCode -eq 0) {
        $summary.Add('Status: `OK`')
    } else {
        $summary.Add('Status: `FAILED`')
    }
    $summary.Add("")
    $summary.Add("Client exit code:")
    $summary.Add(('- `{0}`' -f $client.ExitCode))
    $summary.Add("")
    if (Test-Path $ClientStdoutPath) {
        $opsLine = Get-Content $ClientStdoutPath | Where-Object { $_ -match '^\s*Totals\s+' } | Select-Object -Last 1
        if ($opsLine) {
            $summary.Add("Client totals:")
            $summary.Add(('- `{0}`' -f (($opsLine -replace '\s+', ' ').Trim())))
            $summary.Add("")
        }
    }
    $dropCount = 0
    if (Test-Path $ClientStderrPath) {
        $dropCount = @(Get-Content $ClientStderrPath | Where-Object { $_ -match '^connection dropped' }).Count
        $progressLine = Get-Content $ClientStderrPath | Where-Object { $_ -match 'avg:\s+[0-9]+.*ops/sec' } | Select-Object -Last 1
        if ($progressLine) {
            $summary.Add("Observed progress line:")
            $summary.Add(('- `{0}`' -f (($progressLine -replace '\s+', ' ').Trim())))
            $summary.Add("")
        }
    }
    $serverSlotCount = 0
    if (Test-Path $StderrPath) {
        $serverSlotCount = @(Get-Content $StderrPath | Where-Object { $_ -match '\[hz-win-min\] conn_close .*slot=\d+' } | ForEach-Object {
            if ($_ -match 'slot=(\d+)') { [int]$Matches[1] }
        } | Sort-Object -Unique).Count
    }
    $summary.Add("Runtime note:")
    $summary.Add(('- `connection dropped.` count: `{0}`' -f $dropCount))
    $summary.Add(('- `server slot count:` `{0}`' -f $serverSlotCount))
    $summary.Add("")
    $summary.Add("Raw client stdout and stderr are kept in the private raw-results lane.")
    $summary.Add("")
    $summary.Add(('Raw log: `{0}`' -f $RawLogPath))

    $raw.Add("status: OK")
    $raw.Add("server_args: " + ($serverArgs -join " "))
    $raw.Add("client_args: " + ($clientArgs -join " "))
    $raw.Add("client_exit_code: " + $client.ExitCode)
    $raw.Add("client_connection_dropped_count: " + $dropCount)
    $raw.Add("server_slot_count: " + $serverSlotCount)
} finally {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 200
    }
    if (Test-Path $StdoutPath) {
        $raw.Add("")
        $raw.Add("server stdout:")
        foreach ($line in (Get-Content $StdoutPath)) {
            $raw.Add($line)
        }
    }
    if (Test-Path $StderrPath) {
        $raw.Add("")
        $raw.Add("server stderr:")
        foreach ($line in (Get-Content $StderrPath)) {
            $raw.Add($line)
        }
    }
    if (Test-Path $ClientStdoutPath) {
        $raw.Add("")
        $raw.Add("client stdout:")
        foreach ($line in (Get-Content $ClientStdoutPath)) {
            $raw.Add($line)
        }
    }
    if (Test-Path $ClientStderrPath) {
        $raw.Add("")
        $raw.Add("client stderr:")
        foreach ($line in (Get-Content $ClientStderrPath)) {
            $raw.Add($line)
        }
    }
    Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8
    Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
    Write-Host "Wrote summary: $SummaryPath"
    Write-Host "Wrote raw log: $RawLogPath"
}

[pscustomobject]@{
    SummaryPath = $SummaryPath
    RawLogPath = $RawLogPath
    ClientExePath = $ClientExePath
}
