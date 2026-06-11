param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$Iters = 10000000,
    [int]$DirectRetainCap = 0,
    [int]$SpanClassMax = 0,
    [int]$EmptySpanCap = 0
)

$ErrorActionPreference = "Stop"

$Hz7Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz7Root
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out_win_hz7_v3_hotpath"
}

$CommonScript = Join-Path $PSScriptRoot "bench_hz7_v3_common.ps1"
$BenchSource = Join-Path $PSScriptRoot "bench_hz7_v3_hotpath.c"
if (-not (Test-Path $CommonScript)) {
    throw "common bench helper not found: $CommonScript"
}
. $CommonScript
Invoke-H7BenchmarkProbe -CompilerPath $CompilerPath -Hz7Root $Hz7Root -OutputDir $OutputDir `
    -BenchSource $BenchSource -OutputExeName "bench_hz7_v3_hotpath.exe" `
    -RunnerTag "hz7-v3-hotpath" -SummaryTitle "HZ7 v3 Windows Hot Path Microbench" `
    -Benchmark "bench_hz7_v3_hotpath" -Allocator "hz7-v3" -Runs $Runs -Iters $Iters `
    -Note "diagnostic-only; no allocator counters or production hot-path instrumentation" `
    -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax -EmptySpanCap $EmptySpanCap
