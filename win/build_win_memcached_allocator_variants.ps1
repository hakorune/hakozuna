[CmdletBinding()]
param(
    [string]$SourceRoot,
    [string]$VcpkgRoot,
    [string]$Triplet = "x64-windows",
    [string]$OutDir = "",
    [switch]$SkipPrereqBuilds
)

$ErrorActionPreference = "Stop"

if (-not $SourceRoot) {
    $SourceRoot = ".\private\bench-assets\windows\memcached\source\memcached-1.6.40"
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}
if (-not $OutDir) {
    $OutDir = ".\out_win_memcached_allocators"
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ResolvedSourceRoot = (Resolve-Path (Join-Path $RepoRoot $SourceRoot)).Path
$ResolvedOutDir = (Resolve-Path (New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot $OutDir))).Path
$CompatIncludeRoot = Join-Path $RepoRoot "win\memcached\include"
$SharedCompatIncludeRoot = Join-Path $RepoRoot "win\include"
$WrapperPath = Join-Path $RepoRoot "win\memcached\memcached_win_min_main.c"
$MiShimSrc = Join-Path $RepoRoot "win\memcached\memcached_mimalloc_shim.c"
$TcShimSrc = Join-Path $RepoRoot "win\memcached\memcached_tcmalloc_shim.c"
$Hz3Root = Join-Path $RepoRoot "hakozuna"
$Hz4Root = Join-Path $RepoRoot "hakozuna-mt"
$Hz3Lib = Join-Path $Hz3Root "out_win\hz3_win.lib"
$Hz4Lib = Join-Path $Hz4Root "out_win_bench\hz4_win.lib"
$Hz3ShimSrc = Join-Path $Hz3Root "src\hz3_shim.c"
$Hz4ShimSrc = Join-Path $Hz4Root "src\hz4_shim.c"

$Compiler = Get-Command "clang-cl" -ErrorAction SilentlyContinue
if (-not $Compiler) {
    throw "clang-cl not found in PATH"
}

$IncludeRoot = Join-Path $VcpkgRoot ("installed\" + $Triplet + "\include")
$LibRoot = Join-Path $VcpkgRoot ("installed\" + $Triplet + "\lib")
$BinRoot = Join-Path $VcpkgRoot ("installed\" + $Triplet + "\bin")

if (-not (Test-Path (Join-Path $IncludeRoot "event.h"))) {
    throw "libevent headers not found. Run win/prepare_win_memcached_libevent.ps1 first."
}

if (-not $SkipPrereqBuilds) {
    if (-not (Test-Path $Hz3Lib)) {
        & (Join-Path $Hz3Root "win\build_win_min.ps1") | Out-Null
        if (-not (Test-Path $Hz3Lib)) {
            throw "hz3 library not found after build: $Hz3Lib"
        }
    }
    if (-not (Test-Path $Hz4Lib)) {
        & (Join-Path $Hz4Root "win\build_win_bench_compare.ps1") | Out-Null
        if (-not (Test-Path $Hz4Lib)) {
            throw "hz4 library not found after build: $Hz4Lib"
        }
    }
}

$Sources = @(
    $WrapperPath,
    (Join-Path $ResolvedSourceRoot "hash.c"),
    (Join-Path $ResolvedSourceRoot "jenkins_hash.c"),
    (Join-Path $ResolvedSourceRoot "murmur3_hash.c"),
    (Join-Path $ResolvedSourceRoot "slabs.c"),
    (Join-Path $ResolvedSourceRoot "items.c"),
    (Join-Path $ResolvedSourceRoot "assoc.c"),
    (Join-Path $ResolvedSourceRoot "thread.c"),
    (Join-Path $ResolvedSourceRoot "stats_prefix.c"),
    (Join-Path $ResolvedSourceRoot "util.c"),
    (Join-Path $ResolvedSourceRoot "cache.c"),
    (Join-Path $ResolvedSourceRoot "bipbuffer.c"),
    (Join-Path $ResolvedSourceRoot "base64.c"),
    (Join-Path $ResolvedSourceRoot "logger.c"),
    (Join-Path $ResolvedSourceRoot "crawler.c"),
    (Join-Path $ResolvedSourceRoot "itoa_ljust.c"),
    (Join-Path $ResolvedSourceRoot "slab_automove.c"),
    (Join-Path $ResolvedSourceRoot "slabs_mover.c"),
    (Join-Path $ResolvedSourceRoot "authfile.c"),
    (Join-Path $ResolvedSourceRoot "proto_text.c"),
    (Join-Path $ResolvedSourceRoot "proto_bin.c"),
    (Join-Path $ResolvedSourceRoot "proto_parser.c"),
    (Join-Path $ResolvedSourceRoot "vendor\mcmc\mcmc.c")
)

$BaseCompileArgs = @(
    "/nologo",
    "/W3",
    ("/I" + $CompatIncludeRoot),
    ("/I" + $SharedCompatIncludeRoot),
    ("/I" + $ResolvedSourceRoot),
    ("/I" + $IncludeRoot),
    "/DHAVE_CONFIG_H",
    "/D_WIN32",
    "/DWIN32",
    "/D_CRT_SECURE_NO_WARNINGS"
)

function Write-RspFile {
    param(
        [string]$Path,
        [string[]]$Lines
    )

    $rsp = foreach ($line in $Lines) {
        if ($line -match '\s') {
            '"' + $line.Replace('"', '\"') + '"'
        } else {
            $line
        }
    }
    $rsp | Set-Content -Path $Path -Encoding ascii
}

function Invoke-VariantBuild {
    param(
        [string]$Name,
        [string[]]$CompileArgs,
        [string[]]$VariantSources,
        [string[]]$LinkArgs,
        [string[]]$DllsToCopy
    )

    $ExePath = Join-Path $ResolvedOutDir ("memcached_win_min_main_{0}.exe" -f $Name)
    $RspPath = Join-Path $ResolvedOutDir ("memcached_win_min_main_{0}.rsp" -f $Name)
    $StdoutPath = Join-Path $ResolvedOutDir ("memcached_win_min_main_{0}.stdout.log" -f $Name)
    $StderrPath = Join-Path $ResolvedOutDir ("memcached_win_min_main_{0}.stderr.log" -f $Name)
    $LogPath = Join-Path $ResolvedOutDir ("memcached_win_min_main_{0}.log" -f $Name)

    Remove-Item $ExePath,$RspPath,$StdoutPath,$StderrPath,$LogPath -ErrorAction SilentlyContinue
    Write-RspFile -Path $RspPath -Lines ($CompileArgs + $VariantSources)

    $Args = @(
        ("@" + $RspPath),
        "/link",
        ("/LIBPATH:" + $LibRoot),
        ("/OUT:" + $ExePath)
    ) + $LinkArgs

    $Proc = Start-Process -FilePath $Compiler.Source -ArgumentList $Args -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath
    $ExitCode = $Proc.ExitCode

    $Merged = @()
    if (Test-Path $StdoutPath) {
        $Merged += Get-Content -Path $StdoutPath
    }
    if (Test-Path $StderrPath) {
        $Merged += Get-Content -Path $StderrPath
    }
    $Merged | Set-Content -Path $LogPath -Encoding utf8

    if ($ExitCode -ne 0) {
        throw "variant build failed: $Name (see $LogPath)"
    }

    foreach ($dll in $DllsToCopy) {
        if (Test-Path $dll) {
            Copy-Item -Force $dll $ResolvedOutDir
        }
    }

    [pscustomobject]@{
        Name = $Name
        ExePath = $ExePath
        LogPath = $LogPath
    }
}

$CommonLinkArgs = @(
    "event.lib",
    "event_core.lib",
    "event_extra.lib",
    "Ws2_32.lib",
    "Advapi32.lib",
    "Iphlpapi.lib",
    "User32.lib",
    "Shell32.lib"
)
$EventDlls = @(
    (Join-Path $BinRoot "event.dll"),
    (Join-Path $BinRoot "event_core.dll"),
    (Join-Path $BinRoot "event_extra.dll")
)

$Variants = @()

$Variants += Invoke-VariantBuild `
    -Name "crt" `
    -CompileArgs $BaseCompileArgs `
    -VariantSources $Sources `
    -LinkArgs $CommonLinkArgs `
    -DllsToCopy $EventDlls

$Hz3CompileArgs = $BaseCompileArgs + @(
    ("/I" + (Join-Path $Hz3Root "include")),
    ("/I" + (Join-Path $Hz3Root "win\include"))
)
$Variants += Invoke-VariantBuild `
    -Name "hz3" `
    -CompileArgs $Hz3CompileArgs `
    -VariantSources ($Sources + @($Hz3ShimSrc)) `
    -LinkArgs ($CommonLinkArgs + @($Hz3Lib)) `
    -DllsToCopy $EventDlls

$Hz4CompileArgs = $BaseCompileArgs + @(
    ("/I" + (Join-Path $Hz4Root "core")),
    ("/I" + (Join-Path $Hz4Root "include")),
    ("/I" + (Join-Path $Hz4Root "win")),
    "/DHZ4_TLS_DIRECT=0",
    "/DHZ4_PAGE_META_SEPARATE=0",
    "/DHZ4_RSSRETURN=0",
    "/DHZ4_MID_PAGE_SUPPLY_RESV_BOX=0"
)
$Variants += Invoke-VariantBuild `
    -Name "hz4" `
    -CompileArgs $Hz4CompileArgs `
    -VariantSources ($Sources + @($Hz4ShimSrc)) `
    -LinkArgs ($CommonLinkArgs + @($Hz4Lib)) `
    -DllsToCopy $EventDlls

$MiLib = Join-Path $LibRoot "mimalloc.dll.lib"
$MiDll = Join-Path $BinRoot "mimalloc.dll"
$MiRedirect = Join-Path $BinRoot "mimalloc-redirect.dll"
if (-not (Test-Path $MiLib)) {
    throw "mimalloc.dll.lib not found: $MiLib"
}
$MiCompileArgs = $BaseCompileArgs + @(
    ("/I" + $IncludeRoot)
)
$Variants += Invoke-VariantBuild `
    -Name "mimalloc" `
    -CompileArgs $MiCompileArgs `
    -VariantSources ($Sources + @($MiShimSrc)) `
    -LinkArgs ($CommonLinkArgs + @($MiLib)) `
    -DllsToCopy ($EventDlls + @($MiDll, $MiRedirect))

$TcLib = Join-Path $LibRoot "tcmalloc_minimal.lib"
$TcDll = Join-Path $BinRoot "tcmalloc_minimal.dll"
if (-not (Test-Path $TcLib)) {
    throw "tcmalloc_minimal.lib not found: $TcLib"
}
$TcCompileArgs = $BaseCompileArgs + @(
    ("/I" + $IncludeRoot)
)
$Variants += Invoke-VariantBuild `
    -Name "tcmalloc" `
    -CompileArgs $TcCompileArgs `
    -VariantSources ($Sources + @($TcShimSrc)) `
    -LinkArgs ($CommonLinkArgs + @($TcLib)) `
    -DllsToCopy ($EventDlls + @($TcDll))

foreach ($variant in $Variants) {
    Write-Host ("built_variant: {0} => {1}" -f $variant.Name, $variant.ExePath)
}
