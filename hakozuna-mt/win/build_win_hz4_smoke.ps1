param(
    [string]$VcpkgRoot,
    [string[]]$ExtraDefines
)

$ErrorActionPreference = "Stop"

$Hz4Dir = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz4Dir
$OutDir = Join-Path $Hz4Dir "out_win_smoke"

New-Item -ItemType Directory -Force $OutDir | Out-Null

$BuildBenchScript = Join-Path $PSScriptRoot "build_win_bench_compare.ps1"
$BuildArgs = @()
if ($VcpkgRoot) {
    $BuildArgs += @("-VcpkgRoot", $VcpkgRoot)
}
if ($ExtraDefines) {
    $BuildArgs += @("-ExtraDefines", $ExtraDefines)
}

& $BuildBenchScript @BuildArgs
if ($LASTEXITCODE -ne 0) {
    throw "build_win_bench_compare.ps1 failed with exit code $LASTEXITCODE"
}

$Cc = "clang-cl"
if (-not (Get-Command $Cc -ErrorAction SilentlyContinue)) {
    throw "clang-cl not found in PATH."
}

function Invoke-Checked {
    param(
        [string]$Exe,
        [string[]]$ArgList
    )
    & $Exe @ArgList | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "$Exe failed with exit code $LASTEXITCODE"
    }
}

$SmokeSrc = Join-Path $PSScriptRoot "hz4_win_smoke.c"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\\hz4_win.lib"
$Hz4ApiObj = Join-Path $Hz4Dir "out_win_bench\\obj\\hz4_win_api.obj"
$SmokeExe = Join-Path $OutDir "hz4_win_smoke.exe"

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$Hz4Dir\\core",
    "/I$Hz4Dir\\include",
    "/I$Hz4Dir\\win",
    "/I$RepoRoot\\hakozuna\\include",
    "/I$RepoRoot\\hakozuna\\win\\include"
)

Invoke-Checked $Cc ($BaseFlags + @($SmokeSrc, $Hz4ApiObj, $Hz4Lib, "/link", "/out:$SmokeExe"))

Write-Host "Built: $SmokeExe"
