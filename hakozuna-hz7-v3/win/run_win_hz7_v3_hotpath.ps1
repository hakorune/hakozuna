param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 10000000,
    [int]$DirectRetainCap = 0,
    [int]$SpanClassMax = 0
)

$ErrorActionPreference = "Stop"

$Hz7Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz7Root
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_hz7_v3_hotpath"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Compiler = Get-Command $CompilerPath -ErrorAction Stop
$CommonScript = Join-Path $PSScriptRoot "bench_hz7_v3_common.ps1"
$BenchSource = Join-Path $PSScriptRoot "bench_hz7_v3_hotpath.c"
$Hz7Source = Join-Path $Hz7Root "hz7.c"
$OutputPath = Join-Path $OutputDir "bench_hz7_v3_hotpath.exe"

if (-not (Test-Path $BenchSource)) {
    throw "bench source not found: $BenchSource"
}
if (-not (Test-Path $CommonScript)) {
    throw "common bench helper not found: $CommonScript"
}
if (-not (Test-Path $Hz7Source)) {
    throw "HZ7 source not found: $Hz7Source"
}

$Defines = @("/D_CRT_SECURE_NO_WARNINGS")
if ($DirectRetainCap -gt 0) {
    $Defines += "/DH7_DIRECT_RETAIN_CAP=$DirectRetainCap"
}
if ($SpanClassMax -gt 0) {
    $Defines += "/DH7_SPAN_CLASS_MAX=$SpanClassMax"
}

$BuildArgs = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/W4",
    "/WX"
) + $Defines + @(
    $Hz7Source,
    $BenchSource,
    "psapi.lib",
    "/Fe:$OutputPath"
)

Write-Host "[hz7-v3-hotpath] building bench_hz7_v3_hotpath.exe"
& $Compiler.Source @BuildArgs
if ($LASTEXITCODE -ne 0) {
    throw "clang-cl hotpath bench failed with exit code $LASTEXITCODE"
}

. $CommonScript

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz7_v3_hotpath_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_hz7_v3_hotpath_windows.log")
$RawLines = New-Object System.Collections.Generic.List[string]
$Rows = @{}

for ($run = 1; $run -le $Runs; ++$run) {
    $result = Invoke-H7CapturedProcess -FilePath $OutputPath -Arguments @([string]$Iters)
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
 $Summary = New-H7BenchmarkSummaryLines -Title "HZ7 v3 Windows Hot Path Microbench" `
    -Benchmark "bench_hz7_v3_hotpath" -Allocator "hz7-v3" -Runs $Runs -Iters $Iters `
    -Note "diagnostic-only; no allocator counters or production hot-path instrumentation" `
    -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax

Add-H7BenchmarkSummaryTable -Lines $Summary -Rows $Rows

$Summary.Add("")
$Summary.Add("Artifacts: $OutputDir")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
