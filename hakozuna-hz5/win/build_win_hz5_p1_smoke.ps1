param()

$ErrorActionPreference = "Stop"

$Hz5Dir = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $Hz5Dir "out_win_p1_smoke"
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
    & $Exe @ArgList
    if ($LASTEXITCODE -ne 0) {
        throw "$Exe failed with exit code $LASTEXITCODE"
    }
}

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$Hz5Dir\include"
)

$SrcFiles = @(
    (Join-Path $Hz5Dir "core\hz5_segment.c"),
    (Join-Path $Hz5Dir "core\hz5_run.c"),
    (Join-Path $Hz5Dir "core\hz5_owner.c"),
    (Join-Path $Hz5Dir "core\hz5_remote.c"),
    (Join-Path $Hz5Dir "core\hz5_tcache.c"),
    (Join-Path $Hz5Dir "core\hz5_stats.c"),
    (Join-Path $Hz5Dir "win\hz5_p1_smoke.c")
)

$Exe = Join-Path $OutDir "hz5_p1_smoke.exe"
Invoke-Checked $Cc ($BaseFlags + $SrcFiles + @("/Fo$ObjDir\", "/link", "/out:$Exe"))

Write-Host "Built: $Exe"
& $Exe
if ($LASTEXITCODE -ne 0) {
    throw "HZ5-P1 smoke failed with exit code $LASTEXITCODE"
}
