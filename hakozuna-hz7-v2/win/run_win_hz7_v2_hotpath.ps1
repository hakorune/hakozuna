param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 10000000
)

$ErrorActionPreference = "Stop"

$Hz7Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz7Root
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_hz7_v2_hotpath"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Compiler = Get-Command $CompilerPath -ErrorAction Stop
$BenchSource = Join-Path $PSScriptRoot "bench_hz7_v2_hotpath.c"
$Hz7Source = Join-Path $Hz7Root "hz7.c"
$OutputPath = Join-Path $OutputDir "bench_hz7_v2_hotpath.exe"

if (-not (Test-Path $BenchSource)) {
    throw "bench source not found: $BenchSource"
}
if (-not (Test-Path $Hz7Source)) {
    throw "HZ7 source not found: $Hz7Source"
}

$BuildArgs = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/W4",
    "/WX",
    "/D_CRT_SECURE_NO_WARNINGS",
    $Hz7Source,
    $BenchSource,
    "psapi.lib",
    "/Fe:$OutputPath"
)

Write-Host "[hz7-hotpath] building bench_hz7_v2_hotpath.exe"
& $Compiler.Source @BuildArgs
if ($LASTEXITCODE -ne 0) {
    throw "clang-cl hotpath bench failed with exit code $LASTEXITCODE"
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

function Format-Rate {
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

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz7_v2_hotpath_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_hz7_v2_hotpath_windows.log")
$RawLines = New-Object System.Collections.Generic.List[string]
$Rows = @{}

for ($run = 1; $run -le $Runs; ++$run) {
    $result = Invoke-CapturedProcess -FilePath $OutputPath -Arguments @([string]$Iters)
    $RawLines.Add("=== run $run ===")
    foreach ($line in $result.Lines) {
        $RawLines.Add($line)
    }
    $RawLines.Add("")
    if ($result.ExitCode -ne 0) {
        throw "hotpath bench failed run=$run exit=$($result.ExitCode)"
    }
    foreach ($line in $result.Lines) {
        if ($line -notmatch '^hz7_hotpath:') {
            continue
        }
        $fields = @{}
        foreach ($part in ($line -split '\s+')) {
            if ($part -match '^([^=]+)=(.*)$') {
                $fields[$Matches[1]] = $Matches[2]
            }
        }
        if (-not $fields.ContainsKey("op") -or -not $fields.ContainsKey("label")) {
            continue
        }
        $key = "$($fields["op"]):$($fields["label"])"
        if (-not $Rows.ContainsKey($key)) {
            $Rows[$key] = @{
                Op = $fields["op"];
                Label = $fields["label"];
                Size = $fields["size"];
                RateName = if ($fields.ContainsKey("pairs/s")) { "pairs/s" } else { "ops/s" };
                Rates = New-Object System.Collections.Generic.List[double];
                Rss = New-Object System.Collections.Generic.List[double]
            }
        }
        $rateKey = $Rows[$key].RateName
        if ($fields.ContainsKey($rateKey)) {
            $Rows[$key].Rates.Add([double]$fields[$rateKey])
        }
        if ($fields.ContainsKey("peak_kb")) {
            $Rows[$key].Rss.Add([double]$fields["peak_kb"])
        }
    }
}

$Summary = New-Object System.Collections.Generic.List[string]
$Summary.Add("# HZ7 v2 Windows Hot Path Microbench")
$Summary.Add("")
$Summary.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')")
$Summary.Add("")
$Summary.Add('- benchmark: `bench_hz7_v2_hotpath`')
$Summary.Add('- allocator: `hz7-v2`')
$Summary.Add("- runs: $Runs")
$Summary.Add("- iters_per_run: $Iters")
$Summary.Add("- note: diagnostic-only; no allocator counters or production hot-path instrumentation")
$Summary.Add("")
$Summary.Add("| op | label | size | median rate | rate unit | median peak_kb |")
$Summary.Add("| --- | --- | ---: | ---: | --- | ---: |")

foreach ($key in ($Rows.Keys | Sort-Object)) {
    $row = $Rows[$key]
    $medianRate = Get-Median $row.Rates.ToArray()
    $medianRss = Get-Median $row.Rss.ToArray()
    $Summary.Add("| $($row.Op) | $($row.Label) | $($row.Size) | $(Format-Rate $medianRate) | $($row.RateName) | $([int]$medianRss) |")
}

$Summary.Add("")
$Summary.Add("Artifacts: $OutputDir")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
