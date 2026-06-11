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
$RemoteNaturalSmokeSource = Join-Path $Hz7Root "tests\hz7_remote_natural_smoke.c"
$MtSmokeSource = Join-Path $Hz7Root "tests\hz7_mt_smoke.c"
$StatsSmokeSource = Join-Path $Hz7Root "tests\hz7_stats_smoke.c"
$CppSmokeSource = Join-Path $Hz7Root "tests\hz7_cpp_smoke.cpp"
$Hz7Source = Join-Path $Hz7Root "hz7.c"
$OutputPath = Join-Path $OutDir "hz7_smoke.exe"
$RemoteOutputPath = Join-Path $OutDir "hz7_remote_smoke.exe"
$RemoteNaturalOutputPath = Join-Path $OutDir "hz7_remote_natural_smoke.exe"
$MtOutputPath = Join-Path $OutDir "hz7_mt_smoke.exe"
$StatsOutputPath = Join-Path $OutDir "hz7_stats_smoke.exe"
$CppOutputPath = Join-Path $OutDir "hz7_cpp_smoke.exe"

if (-not (Test-Path $SmokeSource)) {
    throw "Smoke source not found: $SmokeSource"
}
if (-not (Test-Path $MtSmokeSource)) {
    throw "MT smoke source not found: $MtSmokeSource"
}
if (-not (Test-Path $RemoteSmokeSource)) {
    throw "Remote smoke source not found: $RemoteSmokeSource"
}
if (-not (Test-Path $RemoteNaturalSmokeSource)) {
    throw "Remote natural smoke source not found: $RemoteNaturalSmokeSource"
}
if (-not (Test-Path $StatsSmokeSource)) {
    throw "Stats smoke source not found: $StatsSmokeSource"
}
if (-not (Test-Path $CppSmokeSource)) {
    throw "C++ smoke source not found: $CppSmokeSource"
}
if (-not (Test-Path $Hz7Source)) {
    throw "HZ7 source not found: $Hz7Source"
}

function Build-Hz7Smoke {
    param(
        [string]$Name,
        [string]$Source,
        [string]$Output,
        [string[]]$ExtraFlags = @()
    )

    $BuildArgs = @(
        "/nologo",
        "/O2",
        "/W4",
        "/WX",
        "/D_CRT_SECURE_NO_WARNINGS"
    ) + $ExtraFlags + @(
        $Hz7Source,
        $Source,
        "/Fe:$Output"
    )

    Write-Host "[hz7-win] building $Name"
    & $Compiler.Source @BuildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "clang-cl $Name failed with exit code $LASTEXITCODE"
    }
}

function Run-Hz7Smoke {
    param(
        [string]$Name,
        [string]$Output
    )

    Write-Host "[hz7-win] running $Name"
    & $Output
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE"
    }
}

$SmokeTargets = @(
    @{ Name = "hz7_smoke.exe"; Source = $SmokeSource; Output = $OutputPath },
    @{ Name = "hz7_remote_smoke.exe"; Source = $RemoteSmokeSource; Output = $RemoteOutputPath },
    @{ Name = "hz7_remote_natural_smoke.exe"; Source = $RemoteNaturalSmokeSource; Output = $RemoteNaturalOutputPath; ExtraFlags = @("/DH7_REMOTE_NATURAL_PRESET=1") },
    @{ Name = "hz7_mt_smoke.exe"; Source = $MtSmokeSource; Output = $MtOutputPath },
    @{ Name = "hz7_stats_smoke.exe"; Source = $StatsSmokeSource; Output = $StatsOutputPath },
    @{ Name = "hz7_cpp_smoke.exe"; Source = $CppSmokeSource; Output = $CppOutputPath }
)

foreach ($Target in $SmokeTargets) {
    $ExtraFlags = @()
    if ($Target.ContainsKey("ExtraFlags")) {
        $ExtraFlags = $Target.ExtraFlags
    }
    Build-Hz7Smoke -Name $Target.Name -Source $Target.Source -Output $Target.Output -ExtraFlags $ExtraFlags
}

if (-not $SkipRun) {
    foreach ($Target in $SmokeTargets) {
        Run-Hz7Smoke -Name $Target.Name -Output $Target.Output
    }
}

Write-Host "HZ7 Windows smoke output: $OutDir"
