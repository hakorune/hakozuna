param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutDir
)

$ErrorActionPreference = "Stop"

$Hz6Root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $OutDir = Join-Path $Hz6Root "out\win\hz6_benchmark"
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

. (Join-Path $PSScriptRoot "hz6_win_build_common.ps1")

$Compiler = Get-Command $CompilerPath -ErrorAction Stop
$IncludeFlags = Get-Hz6WinIncludeFlags -Hz6Root $Hz6Root -ExtraIncludeRoots @("bench")
$LibSources = Get-Hz6WinLibSources -Hz6Root $Hz6Root
$CommonFlags = Get-Hz6WinClangCommonFlags

$BenchSource = Join-Path $Hz6Root "bench\hz6_allocator_bench.c"
$OutputPath = Join-Path $OutDir "hz6_allocator_bench.exe"
if (-not (Test-Path $BenchSource)) {
    throw "Benchmark source not found: $BenchSource"
}

$Args = @()
$Args += $CommonFlags
$Args += $IncludeFlags
$Args += $LibSources
$Args += $BenchSource
$Args += "/Fe:$OutputPath"

Write-Host "[hz6-win] building hz6_allocator_bench.exe"
& $Compiler.Source @Args
if ($LASTEXITCODE -ne 0) {
    throw "clang-cl failed with exit code $LASTEXITCODE"
}

Write-Host "HZ6 Windows benchmark build output: $OutputPath"
