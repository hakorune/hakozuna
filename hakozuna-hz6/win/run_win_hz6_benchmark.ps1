param(
    [int]$Runs = 5,
    [string]$OutDir,
    [switch]$SkipBuild,
    [UInt64]$LocalIters = 1000000,
    [UInt64]$RemoteIters = 200000,
    [UInt64]$ReuseIters = 100000,
    [string]$LocalSizes = "8192,65536",
    [string]$RemoteSizes = "131072",
    [string]$ReuseSizes = "131072",
    [string]$LocalProfiles = "strict,speed,rss,remote",
    [string]$RemoteProfiles = "speed,rss,remote",
    [string]$ReuseProfiles = "speed,rss,remote",
    [string]$CompilerPath = "clang-cl"
)

$ErrorActionPreference = "Stop"

$Hz6Root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutDir = Join-Path $Hz6Root "private\raw-results\windows\hz6_benchmark_$stamp"
}

function Split-CsvList {
    param([Parameter(Mandatory = $true)][string]$Value)
    $Value.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_.Length -gt 0 }
}

function Get-GitSha {
    param([Parameter(Mandatory = $true)][string]$RepoRoot)
    try {
        (& git -C $RepoRoot rev-parse --short HEAD 2>$null).Trim()
    } catch {
        "nogit"
    }
}

function Convert-BenchLine {
    param([string]$Line)

    $result = @{
        Ops = "NA"
        OpsS = "NA"
        ReuseHits = "NA"
    }
    if (-not $Line) {
        return $result
    }
    foreach ($part in $Line.Split(" ")) {
        if ($part -like "ops=*") {
            $result.Ops = $part.Substring(4)
        } elseif ($part -like "ops/s=*") {
            $result.OpsS = $part.Substring(6)
        } elseif ($part -like "reuse_hits=*") {
            $result.ReuseHits = $part.Substring(11)
        }
    }
    $result
}

function Update-PeakWorkingSet {
    param(
        [Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)][ref]$PeakBytes
    )

    try {
        $Process.Refresh()
        if ($Process.WorkingSet64 -gt $PeakBytes.Value) {
            $PeakBytes.Value = $Process.WorkingSet64
        }
        if ($Process.PeakWorkingSet64 -gt $PeakBytes.Value) {
            $PeakBytes.Value = $Process.PeakWorkingSet64
        }
    } catch {
        # Very short benchmark processes may exit between HasExited and Refresh.
    }
}

function Invoke-BenchCase {
    param(
        [Parameter(Mandatory = $true)][string]$BenchBin,
        [Parameter(Mandatory = $true)][string]$Mode,
        [Parameter(Mandatory = $true)][string]$Profile,
        [Parameter(Mandatory = $true)][int]$Run,
        [Parameter(Mandatory = $true)][UInt64]$Iters,
        [Parameter(Mandatory = $true)][UInt64]$Size,
        [Parameter(Mandatory = $true)][string]$ResultPath,
        [Parameter(Mandatory = $true)][string]$CaseOutDir
    )

    $logPath = Join-Path $CaseOutDir ("{0}_{1}_s{2}_r{3}.log" -f $Mode, $Profile, $Size, $Run)
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $BenchBin
    $psi.Arguments = "{0} {1} {2} {3}" -f $Mode, $Profile, $Iters, $Size
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true

    $proc = [System.Diagnostics.Process]::new()
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $peakWorkingSetBytes = [Int64]0
    Update-PeakWorkingSet -Process $proc -PeakBytes ([ref]$peakWorkingSetBytes)
    while (-not $proc.HasExited) {
        Update-PeakWorkingSet -Process $proc -PeakBytes ([ref]$peakWorkingSetBytes)
        Start-Sleep -Milliseconds 1
    }
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    Update-PeakWorkingSet -Process $proc -PeakBytes ([ref]$peakWorkingSetBytes)
    $peakWorkingSetKb = "NA"
    if ($peakWorkingSetBytes -gt 0) {
        $peakWorkingSetKb = [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0))
    }
    $status = $proc.ExitCode
    ($stdout + $stderr) | Set-Content -Encoding UTF8 -Path $logPath

    $summaryLine = ($stdout -split "`r?`n" | Where-Object { $_ -like "allocator=*" } | Select-Object -Last 1)
    $parsed = Convert-BenchLine -Line $summaryLine

    $row = @(
        $Mode,
        $Profile,
        [string]$Run,
        [string]$status,
        [string]$Iters,
        [string]$Size,
        $parsed.Ops,
        $parsed.OpsS,
        $peakWorkingSetKb,
        $parsed.ReuseHits,
        $logPath
    ) -join "`t"
    Add-Content -Encoding UTF8 -Path $ResultPath -Value $row
}

if ($Runs -le 0) {
    throw "Runs must be positive"
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build_win_hz6_benchmark.ps1") -CompilerPath $CompilerPath
    if ($LASTEXITCODE -ne 0) {
        throw "benchmark build failed with exit code $LASTEXITCODE"
    }
}

$BenchBin = Join-Path $Hz6Root "out\win\hz6_benchmark\hz6_allocator_bench.exe"
if (-not (Test-Path $BenchBin)) {
    throw "missing benchmark binary: $BenchBin"
}

$ReadmePath = Join-Path $OutDir "README.log"
$ResultPath = Join-Path $OutDir "results.tsv"

$localSizeList = Split-CsvList -Value $LocalSizes
$remoteSizeList = Split-CsvList -Value $RemoteSizes
$reuseSizeList = Split-CsvList -Value $ReuseSizes
$localProfileList = Split-CsvList -Value $LocalProfiles
$remoteProfileList = Split-CsvList -Value $RemoteProfiles
$reuseProfileList = Split-CsvList -Value $ReuseProfiles

@(
    "[HZ6_WINDOWS_BENCHMARK]",
    "git_sha=$(Get-GitSha -RepoRoot $Hz6Root)",
    "os=$([System.Environment]::OSVersion.VersionString)",
    "runs=$Runs",
    "local_iters=$LocalIters",
    "remote_iters=$RemoteIters",
    "reuse_iters=$ReuseIters",
    "local_sizes=$LocalSizes",
    "remote_sizes=$RemoteSizes",
    "reuse_sizes=$ReuseSizes",
    "local_profiles=$LocalProfiles",
    "remote_profiles=$RemoteProfiles",
    "reuse_profiles=$ReuseProfiles",
    "bench_bin=$BenchBin",
    "rss_column=PeakWorkingSet64/1024 on Windows",
    ""
) | Set-Content -Encoding UTF8 -Path $ReadmePath

"mode`tprofile`trun`tstatus`titers`tsize`tops`tops_s`tru_maxrss_kb`treuse_hits`tlog" |
    Set-Content -Encoding UTF8 -Path $ResultPath

for ($run = 1; $run -le $Runs; ++$run) {
    foreach ($size in $localSizeList) {
        foreach ($profile in $localProfileList) {
            Invoke-BenchCase -BenchBin $BenchBin -Mode "local" -Profile $profile -Run $run `
                -Iters $LocalIters -Size ([UInt64]$size) -ResultPath $ResultPath -CaseOutDir $OutDir
        }
    }
    foreach ($size in $remoteSizeList) {
        foreach ($profile in $remoteProfileList) {
            Invoke-BenchCase -BenchBin $BenchBin -Mode "remote" -Profile $profile -Run $run `
                -Iters $RemoteIters -Size ([UInt64]$size) -ResultPath $ResultPath -CaseOutDir $OutDir
        }
    }
    foreach ($size in $reuseSizeList) {
        foreach ($profile in $reuseProfileList) {
            Invoke-BenchCase -BenchBin $BenchBin -Mode "reuse" -Profile $profile -Run $run `
                -Iters $ReuseIters -Size ([UInt64]$size) -ResultPath $ResultPath -CaseOutDir $OutDir
        }
    }
}

Write-Host "[DONE] $OutDir"
