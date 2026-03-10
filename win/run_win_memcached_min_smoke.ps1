[CmdletBinding()]
param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$ExePath,
    [int]$Port = 11411,
    [string]$Listen = "127.0.0.1",
    [int]$Threads = 2,
    [int]$MemMb = 64,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\memcached\smoke"
}
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot "out_win_memcached\memcached_win_min_main.exe"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force $path | Out-Null
    }
}

if (-not $SkipBuild) {
    $BuildScript = Join-Path $RepoRoot "win\build_win_memcached_min_main.ps1"
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_memcached_min_main.ps1 failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path $ExePath)) {
    throw "memcached minimal main exe not found: $ExePath"
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_memcached_min_smoke.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_smoke.log")
$StdoutPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_stdout.log")
$StderrPath = Join-Path $RawOutputDir ($Stamp + "_memcached_min_stderr.log")

function Send-MemcachedCommand {
    param(
        [string]$TargetHost,
        [int]$TargetPort,
        [string]$Payload,
        [int]$ReceiveBytes = 1024
    )

    $client = [System.Net.Sockets.TcpClient]::new()
    $client.ReceiveTimeout = 2000
    $client.SendTimeout = 2000
    $client.Connect($TargetHost, $TargetPort)

    try {
        $stream = $client.GetStream()
        $stream.ReadTimeout = 2000
        $stream.WriteTimeout = 2000
        $bytes = [System.Text.Encoding]::ASCII.GetBytes($Payload)
        $stream.Write($bytes, 0, $bytes.Length)
        Start-Sleep -Milliseconds 200
        $buffer = New-Object byte[] $ReceiveBytes
        $read = $stream.Read($buffer, 0, $buffer.Length)
        if ($read -le 0) {
            return ""
        }
        return [System.Text.Encoding]::ASCII.GetString($buffer, 0, $read)
    } finally {
        $client.Close()
    }
}

$server = $null
$raw = New-Object System.Collections.Generic.List[string]
$summary = New-Object System.Collections.Generic.List[string]

try {
    Remove-Item $StdoutPath, $StderrPath -ErrorAction SilentlyContinue
    $serverArgs = @(
        "-p", [string]$Port,
        "-l", $Listen,
        "-t", [string]$Threads,
        "-m", [string]$MemMb
    )
    $server = Start-Process -FilePath $ExePath -ArgumentList $serverArgs `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru
    Start-Sleep -Seconds 1

    $versionResponse = Send-MemcachedCommand -TargetHost $Listen -TargetPort $Port -Payload "version`r`n"
    $setGetResponse = Send-MemcachedCommand -TargetHost $Listen -TargetPort $Port -Payload "set k 0 0 5`r`nvalue`r`nget k`r`n" -ReceiveBytes 2048

    $versionOk = $versionResponse -match "^VERSION 1\.6\.40"
    $setGetOk = $setGetResponse -match "STORED" -and $setGetResponse -match "VALUE k 0 5" -and $setGetResponse -match "value" -and $setGetResponse -match "END"

    $raw.Add("exe: $ExePath")
    $raw.Add("args: " + ($serverArgs -join " "))
    $raw.Add("version_response:")
    $raw.Add($versionResponse.TrimEnd())
    $raw.Add("")
    $raw.Add("set_get_response:")
    $raw.Add($setGetResponse.TrimEnd())
    $raw.Add("")

    $summary.Add("# Windows Memcached Min Smoke")
    $summary.Add("")
    $summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
    $summary.Add("")
    $summary.Add("References:")
    $summary.Add("- [win/build_win_memcached_min_main.ps1](/C:/git/hakozuna-win/win/build_win_memcached_min_main.ps1)")
    $summary.Add("- [win/run_win_memcached_min_smoke.ps1](/C:/git/hakozuna-win/win/run_win_memcached_min_smoke.ps1)")
    $summary.Add("- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)")
    $summary.Add("")
    $summary.Add("| check | result |")
    $summary.Add("| --- | --- |")
    $summary.Add(('| version | {0} |' -f ($(if ($versionOk) { "PASS" } else { "FAIL" }))))
    $summary.Add(('| set/get | {0} |' -f ($(if ($setGetOk) { "PASS" } else { "FAIL" }))))
    $summary.Add("")
    $summary.Add("Config:")
    $summary.Add(('- exe: `{0}`' -f $ExePath))
    $summary.Add(('- listen: `{0}:{1}`' -f $Listen, $Port))
    $summary.Add(('- threads: `{0}`' -f $Threads))
    $summary.Add(('- mem_mb: `{0}`' -f $MemMb))
    $summary.Add("")
    $summary.Add("Version response:")
    $summary.Add('```text')
    $summary.Add($versionResponse.TrimEnd())
    $summary.Add('```')
    $summary.Add("")
    $summary.Add("Set/Get response:")
    $summary.Add('```text')
    $summary.Add($setGetResponse.TrimEnd())
    $summary.Add('```')
    $summary.Add("")
    $summary.Add(('Raw log: `{0}`' -f $RawLogPath))

    if (-not ($versionOk -and $setGetOk)) {
        throw "memcached minimal smoke failed"
    }
} finally {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 200
    }
    if (Test-Path $StdoutPath) {
        $raw.Add("stdout:")
        foreach ($line in (Get-Content $StdoutPath)) {
            $raw.Add($line)
        }
    }
    if (Test-Path $StderrPath) {
        $raw.Add("stderr:")
        foreach ($line in (Get-Content $StderrPath)) {
            $raw.Add($line)
        }
    }
    Set-Content -Path $RawLogPath -Value $raw -Encoding UTF8
    if ($summary.Count -gt 0) {
        Set-Content -Path $SummaryPath -Value $summary -Encoding UTF8
        Write-Host "Wrote summary: $SummaryPath"
    }
    Write-Host "Wrote raw log: $RawLogPath"
}
