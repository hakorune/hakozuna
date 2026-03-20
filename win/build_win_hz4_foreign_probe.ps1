param(
    [string]$OutDir,
    [string]$VcpkgRoot,
    [string[]]$ExtraDefines
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$SmokeSrc = Join-Path $RepoRoot "win\hz4_foreign_probe.c"
$BuildBenchScript = Join-Path $Hz4Dir "win\build_win_bench_compare.ps1"

if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "out_win_hz4_foreign_probe"
}

$ObjDir = Join-Path $OutDir "obj"
New-Item -ItemType Directory -Force $ObjDir | Out-Null

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

if (-not (Test-Path $SmokeSrc)) {
    throw "smoke source not found: $SmokeSrc"
}

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

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$Hz4Dir\core",
    "/I$Hz4Dir\include",
    "/I$Hz4Dir\win"
)

$HelperDll = Join-Path $OutDir "hz4_foreign_probe_helper.dll"
Invoke-Checked $Cc ($BaseFlags + @("/DHZ4_FOREIGN_HELPER_DLL=1", "/LD", $SmokeSrc, "/link", "/out:$HelperDll"))

$Hz4ApiObj = Join-Path $Hz4Dir "out_win_bench\obj\hz4_win_api.obj"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\hz4_win.lib"
if (-not (Test-Path $Hz4ApiObj)) {
    throw "hz4_win_api.obj not found: $Hz4ApiObj"
}
if (-not (Test-Path $Hz4Lib)) {
    throw "hz4_win.lib not found: $Hz4Lib"
}

$HostExe = Join-Path $OutDir "hz4_foreign_probe.exe"
Invoke-Checked $Cc ($BaseFlags + @($SmokeSrc, $Hz4ApiObj, $Hz4Lib, "/link", "/out:$HostExe"))

Write-Host "Built foreign probe artifacts in: $OutDir"
