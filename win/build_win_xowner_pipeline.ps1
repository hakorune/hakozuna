param(
    [string]$OutDirName = "out_win_xowner",
    [switch]$OnlyHz11
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot $OutDirName
$Hz11Root = Join-Path $RepoRoot "hakozuna-hz11"
$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\vcpkg" }
$BaseFlags = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$PSScriptRoot", "/I$Hz11Root\include", "/I$Hz11Root\src"
)
$Hz11Sources = @(
    "$Hz11Root\src\hz11_size_class.c",
    "$Hz11Root\src\hz11_sys_alloc.c",
    "$Hz11Root\src\hz11_thread_cache.c",
    "$Hz11Root\src\hz11_public_entry.c",
    "$Hz11Root\src\hz11_span.c",
    "$Hz11Root\src\hz11_live_footprint.c"
)
$Bench = Join-Path $PSScriptRoot "bench_xowner_pipeline.c"
New-Item -ItemType Directory -Force $OutDir | Out-Null

function Invoke-Checked {
    param([string[]]$CompilerArgs)
    & clang-cl @CompilerArgs
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

$hz11Flags = $BaseFlags + @(
    "/DHZ_BENCH_USE_HZ11=1", "/DHZ11_CLASSIFY_SPAN=1"
) + @($Bench) + $Hz11Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_xowner_hz11.exe')"
)
Invoke-Checked $hz11Flags

if (-not $OnlyHz11) {
    $tcLib = Join-Path $VcpkgRoot "installed\x64-windows\lib\tcmalloc_minimal.lib"
    $tcHeader = Join-Path $VcpkgRoot "installed\x64-windows\include\gperftools\tcmalloc.h"
    if ((Test-Path $tcLib) -and (Test-Path $tcHeader)) {
        Invoke-Checked ($BaseFlags + @(
            "/I$(Join-Path $VcpkgRoot 'installed\x64-windows\include')",
            "/DHZ_BENCH_USE_TCMALLOC=1"
        ) + @($Bench, $tcLib) + @(
            "/link",
            "/out:$(Join-Path $OutDir 'bench_xowner_tcmalloc.exe')"
        ))
    } else {
        Write-Warning "tcmalloc not found; built HZ11 only."
    }
}
if (Test-Path (Join-Path $VcpkgRoot "installed\x64-windows\bin\tcmalloc_minimal.dll")) {
    Copy-Item -Force (Join-Path $VcpkgRoot "installed\x64-windows\bin\tcmalloc_minimal.dll") $OutDir
}
Write-Host "Built xowner pipeline artifacts in: $OutDir"
