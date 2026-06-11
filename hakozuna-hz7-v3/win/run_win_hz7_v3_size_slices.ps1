param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 2000000,
    [int]$DirectRetainCap = 0,
    [int]$SpanClassMax = 0,
    [int]$EmptySpanCap = 0
)

$ErrorActionPreference = "Stop"

$Hz7Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz7Root
$CommonScript = Join-Path $PSScriptRoot "bench_hz7_v3_common.ps1"
$HotpathScript = Join-Path $PSScriptRoot "run_win_hz7_v3_hotpath.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_hz7_v3_size_slices"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

. $CommonScript

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$RawDir = Join-Path $OutputDir "raw"
New-Item -ItemType Directory -Force $RawDir | Out-Null

$hotpathArgs = @(
    "-Runs", [string]$Runs,
    "-Iters", [string]$Iters,
    "-OutputDir", $RawDir
)
if ($DirectRetainCap -gt 0) {
    $hotpathArgs += @("-DirectRetainCap", [string]$DirectRetainCap)
}
if ($SpanClassMax -gt 0) {
    $hotpathArgs += @("-SpanClassMax", [string]$SpanClassMax)
}
if ($EmptySpanCap -gt 0) {
    $hotpathArgs += @("-EmptySpanCap", [string]$EmptySpanCap)
}

$result = Invoke-H7CapturedProcess -FilePath "powershell" -Arguments @(
    "-ExecutionPolicy", "Bypass",
    "-File", $HotpathScript
) + $hotpathArgs
if ($result.ExitCode -ne 0) {
    throw "hotpath runner failed with exit code $($result.ExitCode)"
}

$summaryPath = $null
foreach ($line in $result.Lines) {
    if ($line -match '^Wrote summary:\s+(.*)$') {
        $summaryPath = $Matches[1].Trim()
    }
}
if (-not $summaryPath -or -not (Test-Path $summaryPath)) {
    throw "hotpath summary not found"
}

$summaryLines = Get-Content $summaryPath
$rows = @{}
Add-H7SummaryRowsFromMarkdown -Rows $rows -Lines $summaryLines

$wanted = @(
    'malloc_free:span4k',
    'malloc_free:span8k',
    'malloc_free:span16k',
    'route_invariant:span4k',
    'route_invariant:span8k',
    'route_invariant:span16k',
    'route_valid:span4k',
    'route_valid:span8k',
    'route_valid:span16k',
    'route_invalid:span4k',
    'route_invalid:span8k',
    'route_invalid:span16k',
    'malloc_batch:span4k',
    'malloc_batch:span8k',
    'malloc_batch:span16k',
    'free_batch:span4k',
    'free_batch:span8k',
    'free_batch:span16k',
    'free_retained_loop:span4k',
    'free_retained_loop:span8k',
    'free_retained_loop:span16k'
)

$Report = New-H7BenchmarkSummaryLines -Title "HZ7 v3 Windows Size Slices" `
    -Benchmark "bench_hz7_v3_size_slices" -Allocator "hz7-v3" -Runs $Runs -Iters $Iters `
    -Note "filtered from the v3 hotpath probe to emphasize 4K/8K/16K span-audit rows" `
    -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax

Add-H7BenchmarkSummaryTable -Lines $Report -Rows $rows -OrderedKeys $wanted

$Report.Add("")
$Report.Add("Artifacts: $OutputDir")

$summaryOut = Join-Path $OutputDir ($Stamp + "_hz7_v3_size_slices_windows.md")
$rawOut = Join-Path $OutputDir ($Stamp + "_hz7_v3_size_slices_windows.log")
Set-Content -Path $summaryOut -Value $Report -Encoding UTF8
Set-Content -Path $rawOut -Value ($result.Lines -join "`r`n") -Encoding UTF8
Write-Host "Wrote summary: $summaryOut"
