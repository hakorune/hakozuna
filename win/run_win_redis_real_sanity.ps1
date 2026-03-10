param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$RedisRoot,
    [int]$Runs = 3,
    [int]$Port = 6389,
    [int]$Requests = 100000,
    [int]$Clients = 50,
    [int]$Pipeline = 16,
    [string]$Tests = "set,get,lpush,lpop"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$WorkDir = Join-Path $RepoRoot "out_win_redis_real"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\redis\real_sanity"
}
if (-not $RedisRoot) {
    foreach ($candidate in @(
        (Join-Path $RepoRoot "private\bench-assets\windows\redis\prebuilt"),
        (Join-Path $RepoRoot "private\hakmem\hakozuna\bench\windows\redis")
    )) {
        if (Test-Path $candidate) {
            $RedisRoot = $candidate
            break
        }
    }
}
if (-not $RedisRoot) {
    throw "redis prebuilt root not found. Run .\win\prepare_win_redis_private_layout.ps1 or keep the legacy private\hakmem tree."
}

$RedisServer = Join-Path $RedisRoot "redis-server.exe"
$RedisBench = Join-Path $RedisRoot "redis-benchmark.exe"

foreach ($path in @($RedisRoot, $OutputDir, $RawOutputDir, $WorkDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force $path | Out-Null
    }
}

if (-not (Test-Path $RedisServer)) {
    throw "redis-server.exe not found: $RedisServer"
}
if (-not (Test-Path $RedisBench)) {
    throw "redis-benchmark.exe not found: $RedisBench"
}

function Get-Median {
    param([double[]]$Values)
    if (-not $Values -or $Values.Count -eq 0) {
        return [double]::NaN
    }
    $sorted = $Values | Sort-Object
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Start-RedisServer {
    param([string]$ServerPath, [int]$ListenPort, [string]$ServerDir)

    $args = @(
        "--port", [string]$ListenPort,
        "--save", '""',
        "--appendonly", "no",
        "--dir", $ServerDir,
        "--dbfilename", "redis-bench.rdb"
    )
    $proc = Start-Process -FilePath $ServerPath -ArgumentList $args -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2
    return $proc
}

$TestList = $Tests.Split(",") | ForEach-Object { $_.Trim().ToUpperInvariant() } | Where-Object { $_ -ne "" }
$Results = @{}
foreach ($name in $TestList) {
    $Results[$name] = New-Object System.Collections.Generic.List[double]
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_redis_real_windows_sanity.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_redis_real_windows_sanity.log")
$Summary = New-Object System.Collections.Generic.List[string]
$Raw = New-Object System.Collections.Generic.List[string]

$Summary.Add("# Windows Real Redis Sanity")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [win/prepare_win_redis_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_redis_private_layout.ps1)")
$Summary.Add("- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)")
$Summary.Add("")
$Summary.Add("Sanity lane note:")
$Summary.Add(('- source root: `{0}`' -f $RedisRoot))
$Summary.Add("- lane: prebuilt Windows Redis baseline")
$Summary.Add(("- params: tests={0} n={1} clients={2} pipeline={3} port={4}" -f $Tests, $Requests, $Clients, $Pipeline, $Port))
$Summary.Add(("- runs: {0}" -f $Runs))
$Summary.Add("- statistic: median requests/sec")
$Summary.Add("- note: this is an app-level baseline lane; allocator matrix still splits into real Redis and synthetic Redis-style workload")
$Summary.Add("- raw log path is private-only")
$Summary.Add("")

for ($run = 1; $run -le $Runs; $run++) {
    $proc = $null
    try {
        $proc = Start-RedisServer -ServerPath $RedisServer -ListenPort $Port -ServerDir $WorkDir
        $args = @(
            "-q",
            "-h", "127.0.0.1",
            "-p", [string]$Port,
            "-t", ($TestList -join ",").ToLowerInvariant(),
            "-n", [string]$Requests,
            "-P", [string]$Pipeline,
            "-c", [string]$Clients
        )
        $output = & $RedisBench @args 2>&1
        $rc = $LASTEXITCODE

        $Raw.Add("=== run $run ===")
        $Raw.Add(('server: {0} --port {1} --save "" --appendonly no --dir {2} --dbfilename redis-bench.rdb' -f $RedisServer, $Port, $WorkDir))
        $Raw.Add("bench: $RedisBench " + ($args -join " "))
        $Raw.Add("rc: $rc")
        foreach ($line in $output) {
            $Raw.Add($line.ToString())
        }
        $Raw.Add("")

        if ($rc -ne 0) {
            throw "redis-benchmark failed with exit code $rc"
        }

        foreach ($line in $output) {
            $text = $line.ToString().Trim()
            if ($text -match "^(SET|GET|LPUSH|LPOP|INCR):\s+([0-9.]+)\s+requests per second$") {
                $name = $Matches[1]
                $value = [double]$Matches[2]
                if ($Results.ContainsKey($name)) {
                    $Results[$name].Add($value)
                }
            }
        }
    } finally {
        if ($proc -and -not $proc.HasExited) {
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        }
        Start-Sleep -Milliseconds 300
    }
}

$Summary.Add("| test | median requests/sec | runs |")
$Summary.Add("| --- | ---: | --- |")
foreach ($name in $TestList) {
    $runsList = $Results[$name]
    $median = Get-Median -Values $runsList.ToArray()
    $runText = ($runsList | ForEach-Object { ('{0:N2}' -f $_) }) -join ", "
    $Summary.Add(('| {0} | {1:N2} | `{2}` |' -f $name.ToLowerInvariant(), $median, $runText))
}
$Summary.Add("")
$Summary.Add("Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)")
$Summary.Add(('Raw log: `{0}`' -f $RawLogPath))

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $Raw -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
Write-Host "Wrote raw log: $RawLogPath"
