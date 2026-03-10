[CmdletBinding()]
param(
    [string]$SourceRoot,
    [string]$VcpkgRoot,
    [string]$Triplet = "x64-windows",
    [string]$OutDir = "",
    [string]$ExeName = "memcached_win_min_main.exe"
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
    $OutDir = ".\out_win_memcached"
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ResolvedSourceRoot = (Resolve-Path (Join-Path $RepoRoot $SourceRoot)).Path
$ResolvedOutDir = (Resolve-Path (New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot $OutDir))).Path
$CompatIncludeRoot = Join-Path $RepoRoot "win\memcached\include"
$SharedCompatIncludeRoot = Join-Path $RepoRoot "win\include"
$WrapperPath = Join-Path $RepoRoot "win\memcached\memcached_win_min_main.c"

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

$ExePath = Join-Path $ResolvedOutDir $ExeName
$RspPath = Join-Path $ResolvedOutDir "memcached_win_min_main_link.rsp"
$LogPath = Join-Path $ResolvedOutDir "memcached_win_min_main_link.log"
$StdoutPath = Join-Path $ResolvedOutDir "memcached_win_min_main_link.stdout.log"
$StderrPath = Join-Path $ResolvedOutDir "memcached_win_min_main_link.stderr.log"

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

$CompileArgs = @(
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

$RspLines = foreach ($Arg in ($CompileArgs + $Sources)) {
    if ($Arg -match '\s') {
        '"' + $Arg.Replace('"', '\"') + '"'
    } else {
        $Arg
    }
}
$RspLines | Set-Content -Path $RspPath -Encoding ascii

$Args = @(
    ("@" + $RspPath),
    "/link",
    ("/LIBPATH:" + $LibRoot),
    ("/OUT:" + $ExePath),
    "event.lib",
    "event_core.lib",
    "event_extra.lib",
    "Ws2_32.lib",
    "Advapi32.lib",
    "Iphlpapi.lib",
    "User32.lib",
    "Shell32.lib"
)

Remove-Item $ExePath,$StdoutPath,$StderrPath,$LogPath -ErrorAction SilentlyContinue
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
    Write-Host ("compiler: {0}" -f $Compiler.Source)
    Write-Host ("log: {0}" -f $LogPath)
    Write-Host ("exit_code: {0}" -f $ExitCode)
    if ($Merged) {
        $Merged | Select-Object -First 80 | ForEach-Object { Write-Host $_ }
    }
    exit $ExitCode
}

foreach ($dll in @("event.dll", "event_core.dll", "event_extra.dll")) {
    $src = Join-Path $BinRoot $dll
    if (Test-Path $src) {
        Copy-Item -Force $src $ResolvedOutDir
    }
}

Write-Host ("compiler: {0}" -f $Compiler.Source)
Write-Host ("output: {0}" -f $ExePath)
Write-Host ("log: {0}" -f $LogPath)
Write-Host "copied_dlls: event.dll event_core.dll event_extra.dll"

exit 0
