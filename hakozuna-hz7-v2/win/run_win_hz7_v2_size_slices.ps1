param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 20000000,
    [int]$WorkingSet = 400,
    [int]$DirectRetainCap = 0
)

$ErrorActionPreference = "Stop"

$Hz7V2Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz7V2Root
$BuildScript = Join-Path $RepoRoot "win\build_win_random_mixed_suite.ps1"
$BenchDirName = if ($DirectRetainCap -gt 0) {
    "out_win_random_mixed_hz7v2_cap$DirectRetainCap"
} else {
    "out_win_random_mixed"
}
$BenchDir = Join-Path $RepoRoot $BenchDirName
$BenchExe = Join-Path $BenchDir "bench_random_mixed_hz7_v2.exe"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_hz7_v2_size_slices"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

if ($DirectRetainCap -gt 0 -or -not (Test-Path $BenchExe)) {
    & $BuildScript -OnlyHz7V2 -OutDirName $BenchDirName -Hz7V2DirectRetainCap $DirectRetainCap
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_random_mixed_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Length -eq 0) {
        return [double]::NaN
    }
    $sorted = $Values | Sort-Object
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Format-Ops {
    param([double]$Value)
    if ([double]::IsNaN($Value)) {
        return "NaN"
    }
    if ($Value -ge 1000000.0) {
        return ("{0:N3}M" -f ($Value / 1000000.0))
    }
    if ($Value -ge 1000.0) {
        return ("{0:N3}K" -f ($Value / 1000.0))
    }
    return ("{0:N2}" -f $Value)
}

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = ($Arguments | ForEach-Object {
        if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }) -join ' '

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($chunk in @($stdout, $stderr)) {
        if (-not [string]::IsNullOrEmpty($chunk)) {
            foreach ($line in ($chunk -split "`r?`n")) {
                if ($line -ne "") {
                    $lines.Add($line)
                }
            }
        }
    }

    return @{ ExitCode = $proc.ExitCode; Lines = $lines }
}

$Profiles = @(
    @{ Name = "small_16_2k"; MinSize = 16; MaxSize = 2048; Note = "small span-heavy control" },
    @{ Name = "span_4k_16k"; MinSize = 4096; MaxSize = 16384; Note = "medium slice still covered by spans" },
    @{ Name = "direct_16k_32k"; MinSize = 16385; MaxSize = 32768; Note = "medium slice crossing into direct retained regions" },
    @{ Name = "mixed_16_32k"; MinSize = 16; MaxSize = 32768; Note = "paper mixed range" }
)

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz7_v2_size_slices_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_hz7_v2_size_slices_windows.log")
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary = New-Object System.Collections.Generic.List[string]

$Summary.Add("# HZ7 v2 Windows Size Slices")
$Summary.Add("")
$Summary.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')")
$Summary.Add("")
$Summary.Add('- benchmark: `bench_random_mixed_compare`')
$Summary.Add('- allocator: `hz7-v2`')
$Summary.Add("- runs: $Runs")
$Summary.Add("- iters: $Iters")
$Summary.Add("- working_set: $WorkingSet")
$Summary.Add("- direct_retain_cap: $(if ($DirectRetainCap -gt 0) { $DirectRetainCap } else { 'default' })")
$Summary.Add("- purpose: split medium into span-covered and direct-retained slices")
$Summary.Add("")
$Summary.Add("| profile | range | note | median ops/s | median peak_kb | runs |")
$Summary.Add("| --- | --- | --- | ---: | ---: | --- |")

foreach ($profile in $Profiles) {
    $ops = New-Object System.Collections.Generic.List[double]
    $rss = New-Object System.Collections.Generic.List[double]
    $runCells = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $Runs; ++$i) {
        $seed = 305419896 + $i
        $args = @(
            [string]$Iters,
            [string]$WorkingSet,
            [string]$profile.MinSize,
            [string]$profile.MaxSize,
            [string]$seed
        )
        $result = Invoke-CapturedProcess -FilePath $BenchExe -Arguments $args
        foreach ($line in $result.Lines) {
            $RawLines.Add("[$($profile.Name)] $line")
        }
        if ($result.ExitCode -ne 0) {
            throw "bench failed for $($profile.Name) run=$i exit=$($result.ExitCode)"
        }
        $joined = ($result.Lines -join " ")
        if ($joined -match 'ops/s=([0-9.]+)') {
            $opsValue = [double]$Matches[1]
            $ops.Add($opsValue)
        } else {
            throw "ops/s not found for $($profile.Name) run=$i"
        }
        if ($joined -match '\[RSS\]\s+peak_kb=([0-9.]+)') {
            $rssValue = [double]$Matches[1]
            $rss.Add($rssValue)
        } else {
            throw "peak_kb not found for $($profile.Name) run=$i"
        }
        $runCells.Add(("{0} / {1:N0} KB" -f (Format-Ops $opsValue), $rssValue))
    }
    $medianOps = Get-Median $ops.ToArray()
    $medianRss = Get-Median $rss.ToArray()
    $range = "$($profile.MinSize)..$($profile.MaxSize)"
    $Summary.Add("| $($profile.Name) | $range | $($profile.Note) | $(Format-Ops $medianOps) | $([int]$medianRss) | $($runCells -join ', ') |")
}

$Summary.Add("")
$Summary.Add("Artifacts: $OutputDir")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
