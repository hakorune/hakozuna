param(
    [string]$VcpkgRoot,
    [string[]]$ExtraDefines
)

$ErrorActionPreference = "Stop"

$Hz4Dir = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz4Dir
$OutDir = Join-Path $Hz4Dir "out_win_bench"
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

function Get-DefineName {
    param([string]$Define)

    if ([string]::IsNullOrWhiteSpace($Define)) {
        return ""
    }
    $eq = $Define.IndexOf("=")
    if ($eq -lt 0) {
        return $Define.Trim()
    }
    return $Define.Substring(0, $eq).Trim()
}

function Merge-Defines {
    param(
        [string[]]$BaseDefines,
        [string[]]$OverrideDefines
    )

    $overrideMap = @{}
    foreach ($define in @($OverrideDefines)) {
        if ([string]::IsNullOrWhiteSpace($define)) {
            continue
        }
        $overrideMap[(Get-DefineName -Define $define)] = $define.Trim()
    }

    $merged = New-Object System.Collections.Generic.List[string]
    foreach ($define in @($BaseDefines)) {
        if ([string]::IsNullOrWhiteSpace($define)) {
            continue
        }
        $name = Get-DefineName -Define $define
        if ($overrideMap.ContainsKey($name)) {
            continue
        }
        $merged.Add($define.Trim())
    }

    foreach ($define in $overrideMap.Values) {
        $merged.Add($define)
    }

    return $merged.ToArray()
}

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\\vcpkg"
}

$VcpkgInclude = Join-Path $VcpkgRoot "installed\\x64-windows\\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\\x64-windows\\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\\x64-windows\\bin"

$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/D__thread=__declspec(thread)",
    "/I$Hz4Dir\\core",
    "/I$Hz4Dir\\include",
    "/I$Hz4Dir\\win",
    "/I$RepoRoot\\hakozuna\\include",
    "/I$RepoRoot\\hakozuna\\win\\include"
)

$Hz4BaseDefines = @(
    "HZ4_TLS_DIRECT=0",
    "HZ4_PAGE_META_SEPARATE=0",
    "HZ4_RSSRETURN=0",
    "HZ4_MID_PAGE_SUPPLY_RESV_BOX=0",
    "HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1",
    "HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1"
)
$MergedDefines = Merge-Defines -BaseDefines $Hz4BaseDefines -OverrideDefines $ExtraDefines
$BaseFlags += ($MergedDefines | ForEach-Object { "/D$_" })

$SrcFiles = Get-ChildItem (Join-Path $Hz4Dir "src") -Filter "*.c" | Where-Object {
    $_.Name -ne "hz4_shim.c"
}

$Objs = @()
foreach ($src in $SrcFiles) {
    $obj = Join-Path $ObjDir ($src.BaseName + ".obj")
    Invoke-Checked $Cc ($BaseFlags + @("/c", $src.FullName, "/Fo$obj"))
    $Objs += $obj
}

$LibTool = $null
$LibCmd = Get-Command "llvm-lib" -ErrorAction SilentlyContinue
if ($LibCmd) {
    $LibTool = $LibCmd.Path
} else {
    $LibCmd = Get-Command "lib" -ErrorAction SilentlyContinue
    if ($LibCmd) {
        $LibTool = $LibCmd.Path
    }
}
if (-not $LibTool) {
    throw "llvm-lib or lib.exe not found in PATH."
}

$LibOut = Join-Path $OutDir "hz4_win.lib"
Invoke-Checked $LibTool (@("/nologo", "/out:$LibOut") + $Objs)

$Hz4ApiObj = Join-Path $ObjDir "hz4_win_api.obj"
Invoke-Checked $Cc ($BaseFlags + @("/c", (Join-Path $Hz4Dir "win\\hz4_win_api.c"), "/Fo$Hz4ApiObj"))

$BenchSrc = Join-Path $RepoRoot "win\\bench_allocator_compare.c"
$BenchOut = Join-Path $OutDir "bench_mixed_ws_hz4.exe"
Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ4=1", "/DHZ_BENCH_DISABLE_REALLOC=1", $BenchSrc, $Hz4ApiObj, $LibOut, "/link", "/out:$BenchOut"))

$MiDll = Join-Path $VcpkgBin "mimalloc.dll"
if (Test-Path $MiDll) {
    Copy-Item -Force $MiDll $OutDir | Out-Null
}
$MiRedirect = Join-Path $VcpkgBin "mimalloc-redirect.dll"
if (Test-Path $MiRedirect) {
    Copy-Item -Force $MiRedirect $OutDir | Out-Null
}
$TcDll = Join-Path $VcpkgBin "tcmalloc_minimal.dll"
if (Test-Path $TcDll) {
    Copy-Item -Force $TcDll $OutDir | Out-Null
}

Write-Host "Built: $BenchOut"
