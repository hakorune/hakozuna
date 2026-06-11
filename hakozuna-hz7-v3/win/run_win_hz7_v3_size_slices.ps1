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

$summaryPath = Find-H7SummaryPathFromLines -Lines $result.Lines
if (-not $summaryPath -or -not (Test-Path $summaryPath)) {
    throw "hotpath summary not found"
}

$rows = Get-H7BenchmarkRowsFromMarkdownPath -Path $summaryPath

$Report = New-H7BenchmarkSummaryLines -Title "HZ7 v3 Windows Size Slices" `
    -Benchmark "bench_hz7_v3_size_slices" -Allocator "hz7-v3" -Runs $Runs -Iters $Iters `
    -Note "filtered from the v3 hotpath probe to emphasize 4K/8K/16K span-audit rows" `
    -DirectRetainCap $DirectRetainCap -SpanClassMax $SpanClassMax

Add-H7BenchmarkSummaryTable -Lines $Report -Rows $rows -OrderedKeys (Get-H7SpanAuditOrderedKeys)

$Report.Add("")
$Report.Add("Artifacts: $OutputDir")

$summaryOut = Join-Path $OutputDir ($Stamp + "_hz7_v3_size_slices_windows.md")
$rawOut = Join-Path $OutputDir ($Stamp + "_hz7_v3_size_slices_windows.log")
Set-Content -Path $summaryOut -Value $Report -Encoding UTF8
Set-Content -Path $rawOut -Value ($result.Lines -join "`r`n") -Encoding UTF8
Write-Host "Wrote summary: $summaryOut"
