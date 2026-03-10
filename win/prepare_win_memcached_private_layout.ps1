[CmdletBinding()]
param(
    [switch]$CopyPaperLibevent
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$PrivateRoot = Join-Path $RepoRoot "private"
$AssetRoot = Join-Path $PrivateRoot "bench-assets\windows\memcached"
$SourceRoot = Join-Path $AssetRoot "source"
$ClientRoot = Join-Path $AssetRoot "client"
$DepsRoot = Join-Path $AssetRoot "deps"
$NotesRoot = Join-Path $AssetRoot "notes"
$RawRoot = Join-Path $PrivateRoot "raw-results\windows\memcached"

$PaperDepsRoot = Join-Path $RepoRoot "private\hakmem\docs\paper\ACE-Alloc\paper_full_20260120\deps"
$PaperLibeventArchive = Join-Path $PaperDepsRoot "libevent-2.1.12-stable.tar.gz"
$PaperLibeventTree = Join-Path $PaperDepsRoot "libevent-2.1.12-stable"

$Dirs = @(
    $SourceRoot,
    $ClientRoot,
    $DepsRoot,
    $NotesRoot,
    $RawRoot
)

foreach ($Dir in $Dirs) {
    New-Item -ItemType Directory -Force -Path $Dir | Out-Null
}

$ReadmePath = Join-Path $AssetRoot "README.txt"
$Readme = @(
    "Windows memcached private asset layout",
    "",
    "source : memcached source tree or prepared Windows port",
    "client : memtier_benchmark source tree or client binaries",
    "deps   : libevent, OpenSSL, zlib, and other vendor inputs",
    "notes  : local migration notes",
    "raw    : benchmark logs and temporary outputs"
)
Set-Content -Path $ReadmePath -Value $Readme -Encoding ascii

if ($CopyPaperLibevent) {
    if (Test-Path $PaperLibeventArchive) {
        Copy-Item -Path $PaperLibeventArchive -Destination (Join-Path $DepsRoot "libevent-2.1.12-stable.tar.gz") -Force
    }
    if (Test-Path $PaperLibeventTree) {
        $DestTree = Join-Path $DepsRoot "libevent-2.1.12-stable"
        if (-not (Test-Path $DestTree)) {
            Copy-Item -Path $PaperLibeventTree -Destination $DestTree -Recurse
        }
    }
}

$Status = [pscustomobject]@{
    asset_root = $AssetRoot
    source_root = $SourceRoot
    client_root = $ClientRoot
    deps_root = $DepsRoot
    raw_root = $RawRoot
    copied_paper_libevent = [bool]$CopyPaperLibevent
    paper_libevent_archive_present = Test-Path $PaperLibeventArchive
    paper_libevent_tree_present = Test-Path $PaperLibeventTree
}

$Status | Format-List | Out-String | Write-Host
