param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 100000,
    [int]$Threads = 4,
    [int]$RemotePct = 90,
    [int]$WorkingSet = 256,
    [int]$RingSlots = 65536,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Hz3Root = Join-Path $RepoRoot "hakozuna"
$OutDir = Join-Path $RepoRoot "out_win_hz3_aligned_policy"
$ObjDir = Join-Path $OutDir "obj"
$ExePath = Join-Path $OutDir "bench_hz3_aligned_policy_probe.exe"
$Hz3Lib = Join-Path $Hz3Root "out_win_page_medium_research\hz3_win.lib"
$ProbeSrc = Join-Path $PSScriptRoot "bench_hz3_aligned_policy_probe.c"
$ShimSrc = Join-Path $Hz3Root "src\hz3_shim.c"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "private\raw-results\windows\hz3_aligned_policy"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null
New-Item -ItemType Directory -Force $ObjDir | Out-Null

$Compiler = Get-Command "clang-cl" -ErrorAction SilentlyContinue
if (-not $Compiler) {
    throw "clang-cl not found in PATH"
}

function Invoke-CheckedBuild {
    param([string[]]$ArgList)

    & $Compiler.Source @ArgList | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "clang-cl failed with exit code $LASTEXITCODE"
    }
}

if (-not $SkipBuild) {
    if (-not (Test-Path $Hz3Lib)) {
        & (Join-Path $Hz3Root "win\build_win_min.ps1") -OutDirName "out_win_page_medium_research" | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "build_win_min.ps1 failed with exit code $LASTEXITCODE"
        }
    }

    $CommonFlags = @(
        "/nologo",
        "/O2",
        "/DNDEBUG",
        "/std:c11",
        "/W3",
        "/MD",
        ("/I" + (Join-Path $Hz3Root "include")),
        ("/I" + (Join-Path $Hz3Root "win\include"))
    )

    Invoke-CheckedBuild ($CommonFlags + @($ProbeSrc, $ShimSrc, $Hz3Lib, "/link", "Psapi.lib", ("/out:" + $ExePath)))
}

if (-not (Test-Path $ExePath)) {
    throw "probe executable not found: $ExePath"
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Count -eq 0) {
        return [double]::NaN
    }
    $sorted = $Values | Sort-Object
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Invoke-Probe {
    param(
        [string]$Lane,
        [bool]$PageMedium,
        [int]$Size,
        [int]$Alignment,
        [int]$Iterations,
        [int]$ThreadCount,
        [int]$RemotePercent,
        [int]$WorkingSetSize,
        [int]$RingSlotCount
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ExePath
    $psi.Arguments = ("{0} {1} {2} {3} {4} {5} {6}" -f `
        $Size, $Alignment, $Iterations, $ThreadCount, $RemotePercent, $WorkingSetSize, $RingSlotCount)
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    if ($PageMedium) {
        $psi.Environment["HZ3_PAGE_MEDIUM_ALIGNED"] = "1"
    } else {
        [void]$psi.Environment.Remove("HZ3_PAGE_MEDIUM_ALIGNED")
    }

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    $chunks = @($stdout, $stderr) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $text = ($chunks -join "`n").Trim()
    if ($proc.ExitCode -ne 0) {
        throw "probe failed lane=$Lane size=$Size align=$Alignment rc=$($proc.ExitCode): $text"
    }
    if ($text -notmatch "ops/s=([0-9.]+)") {
        throw "could not parse ops/s lane=$Lane size=$Size align=$Alignment`: $text"
    }
    $opsPerSec = [double]$Matches[1]
    $peakRssKB = 0
    if ($text -match "peak_rss_kb=([0-9]+)") {
        $peakRssKB = [int64]$Matches[1]
    }

    [pscustomobject]@{
        Lane = $Lane
        Size = $Size
        Alignment = $Alignment
        OpsPerSec = $opsPerSec
        PeakRssKB = $peakRssKB
        Output = $text
    }
}

$Cases = @(
    @{ Name = "medium_4k_a4096"; Size = 4096; Alignment = 4096 },
    @{ Name = "medium_8k_a4096"; Size = 8192; Alignment = 4096 },
    @{ Name = "medium_64k_a4096"; Size = 65536; Alignment = 4096 },
    @{ Name = "guard_2k_a4096"; Size = 2048; Alignment = 4096 },
    @{ Name = "guard_4k_a8192"; Size = 4096; Alignment = 8192 },
    @{ Name = "guard_65537_a4096"; Size = 65537; Alignment = 4096 }
)

$Lanes = @(
    @{ Name = "hz3-default"; PageMedium = $false },
    @{ Name = "hz3-page-medium"; PageMedium = $true }
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz3_aligned_policy_probe.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_hz3_aligned_policy_probe.log")
$RawLines = New-Object System.Collections.Generic.List[string]
$Rows = @()

foreach ($case in $Cases) {
    foreach ($lane in $Lanes) {
        $ops = New-Object System.Collections.Generic.List[double]
        $rss = New-Object System.Collections.Generic.List[double]
        $outputs = New-Object System.Collections.Generic.List[string]
        for ($run = 1; $run -le $Runs; $run++) {
            $result = Invoke-Probe `
                -Lane $lane.Name `
                -PageMedium ([bool]$lane.PageMedium) `
                -Size ([int]$case.Size) `
                -Alignment ([int]$case.Alignment) `
                -Iterations $Iters `
                -ThreadCount $Threads `
                -RemotePercent $RemotePct `
                -WorkingSetSize $WorkingSet `
                -RingSlotCount $RingSlots
            $ops.Add($result.OpsPerSec)
            $rss.Add([double]$result.PeakRssKB)
            $outputs.Add($result.Output)

            $RawLines.Add(("=== {0} / {1} / run {2} ===" -f $case.Name, $lane.Name, $run))
            $RawLines.Add($result.Output)
            $RawLines.Add("")
        }

        $Rows += [pscustomobject]@{
            Case = $case.Name
            Lane = $lane.Name
            Size = [int]$case.Size
            Alignment = [int]$case.Alignment
            MedianOps = Get-Median -Values $ops.ToArray()
            MedianPeakRssKB = Get-Median -Values $rss.ToArray()
            Runs = ($ops | ForEach-Object { ("{0:N2}" -f $_) }) -join ", "
        }
    }
}

$Summary = New-Object System.Collections.Generic.List[string]
$Summary.Add("# HZ3 Windows Aligned Policy Probe")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("Command:")
$Summary.Add('- `win/run_win_hz3_aligned_policy_probe.ps1`')
$Summary.Add("")
$Summary.Add("Settings:")
$Summary.Add(('- runs: `{0}`' -f $Runs))
$Summary.Add(('- iters: `{0}`' -f $Iters))
$Summary.Add(('- threads: `{0}`' -f $Threads))
$Summary.Add(('- remote pct: `{0}`' -f $RemotePct))
$Summary.Add(('- working set: `{0}`' -f $WorkingSet))
$Summary.Add(('- ring slots: `{0}`' -f $RingSlots))
$Summary.Add('- page-medium lane env: `HZ3_PAGE_MEDIUM_ALIGNED=1`')
$Summary.Add("")
$Summary.Add('| case | lane | size | align | median ops/s | median peak RSS KB | runs ops/s |')
$Summary.Add('|---|---|---:|---:|---:|---:|---|')
foreach ($row in $Rows) {
    $Summary.Add(('| {0} | {1} | {2} | {3} | {4:N2} | {5:N0} | `{6}` |' -f `
        $row.Case, $row.Lane, $row.Size, $row.Alignment, $row.MedianOps, $row.MedianPeakRssKB, $row.Runs))
}
$Summary.Add("")
$Summary.Add(('Raw log: `{0}`' -f ($RawLogPath.Replace($RepoRoot + "\", "").Replace("\", "/"))))

Set-Content -Path $SummaryPath -Value $Summary -Encoding utf8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding utf8
Write-Host "Wrote summary: $SummaryPath"
