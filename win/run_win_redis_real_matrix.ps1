param(
    [string]$OutputDir,
    [string]$RawOutputDir,
    [string]$RedisRoot,
    [string]$Allocators = "Jemalloc,Tcmalloc,Mimalloc,Hz3,Hz4",
    [int]$Runs = 3,
    [int]$PortBase = 6389,
    [int]$Requests = 100000,
    [int]$Clients = 50,
    [int]$Pipeline = 16,
    [string]$Tests = "set,get,lpush,lpop",
    [string]$Config = "Release",
    [string]$Platform = "x64",
    [string]$Toolset = "v143",
    [string]$MSBuildPath = "",
    [string]$VcpkgRoot = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildScript = Join-Path $RepoRoot "win\build_win_redis_source_variant.ps1"

if (-not $RedisRoot) {
    foreach ($candidate in @(
        (Join-Path $RepoRoot "private\bench-assets\windows\redis\source"),
        (Join-Path $RepoRoot "private\hakmem\hakozuna\bench\windows\redis-src")
    )) {
        if (Test-Path $candidate) {
            $RedisRoot = $candidate
            break
        }
    }
}
if (-not $RedisRoot) {
    throw "redis source tree not found. Run .\win\prepare_win_redis_private_layout.ps1 first or keep the legacy private\hakmem tree."
}

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\apps"
}
if (-not $RawOutputDir) {
    $RawOutputDir = Join-Path $RepoRoot "private\raw-results\windows\redis\real_matrix"
}

foreach ($path in @($OutputDir, $RawOutputDir)) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Force $path | Out-Null
    }
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
    param(
        [string]$ServerPath,
        [int]$ListenPort,
        [string]$ServerDir
    )

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

function Stop-RedisServerProcess {
    param($Process)

    if (-not $Process) {
        return
    }
    try {
        if (-not $Process.HasExited) {
            Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
        }
    } catch {
    }
    try {
        $Process.WaitForExit(5000) | Out-Null
    } catch {
    }
    Start-Sleep -Milliseconds 700
}

$AllocatorList = @($Allocators.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
$TestList = @($Tests.Split(",") | ForEach-Object { $_.Trim().ToUpperInvariant() } | Where-Object { $_ -ne "" })
$Matrix = @{}
foreach ($allocator in $AllocatorList) {
    $Matrix[$allocator] = @{}
    foreach ($testName in $TestList) {
        $Matrix[$allocator][$testName] = New-Object System.Collections.Generic.List[double]
    }
}

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_redis_real_windows_matrix.md")
$RawLogPath = Join-Path $RawOutputDir ($Stamp + "_redis_real_windows_matrix.log")
$WorkRoot = Join-Path $RepoRoot "out_win_redis_real"
$Summary = New-Object System.Collections.Generic.List[string]
$Raw = New-Object System.Collections.Generic.List[string]

$Summary.Add("# Windows Real Redis Source Matrix")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [win/prepare_win_redis_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_redis_private_layout.ps1)")
$Summary.Add("- [win/build_win_redis_source_variant.ps1](/C:/git/hakozuna-win/win/build_win_redis_source_variant.ps1)")
$Summary.Add("- [docs/WINDOWS_REDIS_MATRIX.md](/C:/git/hakozuna-win/docs/WINDOWS_REDIS_MATRIX.md)")
$Summary.Add("")
$Summary.Add(('Source root used: `{0}`' -f $RedisRoot))
$Summary.Add("Params:")
$Summary.Add(('- allocators: `{0}`' -f ($AllocatorList -join ",")))
$Summary.Add(('- tests: `{0}` n={1} clients={2} pipeline={3} runs={4}' -f $Tests, $Requests, $Clients, $Pipeline, $Runs))
$Summary.Add("- statistic: median requests/sec")
$Summary.Add("- raw logs: private only")
$Summary.Add("")

for ($allocatorIndex = 0; $allocatorIndex -lt $AllocatorList.Count; $allocatorIndex++) {
    $allocator = $AllocatorList[$allocatorIndex]
    if (-not $SkipBuild) {
        Get-Process -Name "redis-server" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 700
        $buildArgs = @(
            "-ExecutionPolicy", "Bypass",
            "-File", $BuildScript,
            "-RedisRoot", $RedisRoot,
            "-Allocator", $allocator,
            "-Config", $Config,
            "-Platform", $Platform,
            "-Toolset", $Toolset
        )
        if ($MSBuildPath) {
            $buildArgs += @("-MSBuildPath", $MSBuildPath)
        }
        if ($VcpkgRoot) {
            $buildArgs += @("-VcpkgRoot", $VcpkgRoot)
        }
        & powershell @buildArgs
        if ($LASTEXITCODE -ne 0) {
            throw "build failed for allocator $allocator"
        }
    }

    $ExeDir = Join-Path $RedisRoot ("msvs\{0}\{1}" -f $Platform, $Config)
    $RedisServer = Join-Path $ExeDir "redis-server.exe"
    $RedisBench = Join-Path $ExeDir "redis-benchmark.exe"
    if (-not (Test-Path $RedisServer)) {
        throw "redis-server.exe not found: $RedisServer"
    }
    if (-not (Test-Path $RedisBench)) {
        throw "redis-benchmark.exe not found: $RedisBench"
    }

    $WorkDir = Join-Path $WorkRoot ($allocator.ToLowerInvariant())
    if (-not (Test-Path $WorkDir)) {
        New-Item -ItemType Directory -Force $WorkDir | Out-Null
    }

    $Port = $PortBase + $allocatorIndex
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

            $Raw.Add("=== allocator $allocator run $run ===")
            $Raw.Add("root: $RedisRoot")
            $Raw.Add("server: $RedisServer")
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
                    if ($Matrix[$allocator].ContainsKey($name)) {
                        $Matrix[$allocator][$name].Add($value)
                    }
                }
            }
        } finally {
            Stop-RedisServerProcess -Process $proc
        }
    }
}

$Summary.Add("| allocator | test | median requests/sec | runs |")
$Summary.Add("| --- | --- | ---: | --- |")
foreach ($allocator in $AllocatorList) {
    foreach ($testName in $TestList) {
        $runsList = $Matrix[$allocator][$testName]
        $median = Get-Median -Values $runsList.ToArray()
        $runText = ($runsList | ForEach-Object { ('{0:N2}' -f $_) }) -join ", "
        $Summary.Add(('| {0} | {1} | {2:N2} | `{3}` |' -f $allocator.ToLowerInvariant(), $testName.ToLowerInvariant(), $median, $runText))
    }
}
$Summary.Add("")
$Summary.Add("Artifacts: [out_win_redis_real](/C:/git/hakozuna-win/out_win_redis_real)")
$Summary.Add(('Raw log: `{0}`' -f $RawLogPath))

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $Raw -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
Write-Host "Wrote raw log: $RawLogPath"
