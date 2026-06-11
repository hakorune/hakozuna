param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutDir,
    [switch]$SkipRun
)

$ErrorActionPreference = "Stop"

$Hz7Root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $OutDir = Join-Path $Hz7Root "out\win\smokes"
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Compiler = Get-Command $CompilerPath -ErrorAction Stop
$SmokeSource = Join-Path $Hz7Root "tests\hz7_smoke.c"
$RemoteSmokeSource = Join-Path $Hz7Root "tests\hz7_remote_smoke.c"
$MtSmokeSource = Join-Path $Hz7Root "tests\hz7_mt_smoke.c"
$Hz7Source = Join-Path $Hz7Root "hz7.c"
$OutputPath = Join-Path $OutDir "hz7_smoke.exe"
$RemoteOutputPath = Join-Path $OutDir "hz7_remote_smoke.exe"
$MtOutputPath = Join-Path $OutDir "hz7_mt_smoke.exe"

if (-not (Test-Path $SmokeSource)) {
    throw "Smoke source not found: $SmokeSource"
}
if (-not (Test-Path $MtSmokeSource)) {
    throw "MT smoke source not found: $MtSmokeSource"
}
if (-not (Test-Path $RemoteSmokeSource)) {
    throw "Remote smoke source not found: $RemoteSmokeSource"
}
if (-not (Test-Path $Hz7Source)) {
    throw "HZ7 source not found: $Hz7Source"
}

$Args = @(
    "/nologo",
    "/O2",
    "/W4",
    "/WX",
    "/D_CRT_SECURE_NO_WARNINGS",
    $Hz7Source,
    $SmokeSource,
    "/Fe:$OutputPath"
)

Write-Host "[hz7-win] building hz7_smoke.exe"
& $Compiler.Source @Args
if ($LASTEXITCODE -ne 0) {
    throw "clang-cl failed with exit code $LASTEXITCODE"
}

$RemoteArgs = @(
    "/nologo",
    "/O2",
    "/W4",
    "/WX",
    "/D_CRT_SECURE_NO_WARNINGS",
    $Hz7Source,
    $RemoteSmokeSource,
    "/Fe:$RemoteOutputPath"
)

Write-Host "[hz7-win] building hz7_remote_smoke.exe"
& $Compiler.Source @RemoteArgs
if ($LASTEXITCODE -ne 0) {
    throw "clang-cl remote smoke failed with exit code $LASTEXITCODE"
}

$MtArgs = @(
    "/nologo",
    "/O2",
    "/W4",
    "/WX",
    "/D_CRT_SECURE_NO_WARNINGS",
    $Hz7Source,
    $MtSmokeSource,
    "/Fe:$MtOutputPath"
)

Write-Host "[hz7-win] building hz7_mt_smoke.exe"
& $Compiler.Source @MtArgs
if ($LASTEXITCODE -ne 0) {
    throw "clang-cl mt smoke failed with exit code $LASTEXITCODE"
}

if (-not $SkipRun) {
    Write-Host "[hz7-win] running hz7_smoke.exe"
    & $OutputPath
    if ($LASTEXITCODE -ne 0) {
        throw "hz7 smoke failed with exit code $LASTEXITCODE"
    }
    Write-Host "[hz7-win] running hz7_remote_smoke.exe"
    & $RemoteOutputPath
    if ($LASTEXITCODE -ne 0) {
        throw "hz7 remote smoke failed with exit code $LASTEXITCODE"
    }
    Write-Host "[hz7-win] running hz7_mt_smoke.exe"
    & $MtOutputPath
    if ($LASTEXITCODE -ne 0) {
        throw "hz7 mt smoke failed with exit code $LASTEXITCODE"
    }
}

Write-Host "HZ7 Windows smoke output: $OutDir"
