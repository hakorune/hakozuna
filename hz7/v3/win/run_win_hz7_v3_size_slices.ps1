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
$hotpathArgs = @(
    "-Runs", [string]$Runs,
    "-Iters", [string]$Iters,
    "-OutputDir", (Join-Path $OutputDir "raw")
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

Invoke-H7FilteredBenchmarkProbe -RunnerScriptPath $HotpathScript -RunnerArguments $hotpathArgs `
    -OutputDir $OutputDir -RunnerTag "hz7-v3-size-slices" `
    -SummaryTitle "HZ7 v3 Windows Size Slices" -Benchmark "bench_hz7_v3_size_slices" `
    -Allocator "hz7-v3" -Runs $Runs -Iters $Iters `
    -Note "filtered from the v3 hotpath probe to emphasize 4K/8K/16K span-audit rows" `
    -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax `
    -OrderedKeys (Get-H7SpanAuditOrderedKeys)
