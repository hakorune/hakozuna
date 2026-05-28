$ErrorActionPreference = "Stop"

function Get-Hz6WinIncludeFlags {
    param(
        [Parameter(Mandatory = $true)][string]$Hz6Root,
        [string[]]$ExtraIncludeRoots = @()
    )

    $includeRoots = @(
        "include",
        "route",
        "scavenge",
        "transfer",
        "owner",
        "source",
        "frontcache",
        "fronts",
        "fronts\large",
        "fronts\local2p",
        "fronts\midpage",
        "fronts\toy",
        "policy",
        "api"
    ) + $ExtraIncludeRoots

    foreach ($includeRoot in $includeRoots) {
        "/I" + (Join-Path $Hz6Root $includeRoot)
    }
}

function Get-Hz6WinLibSources {
    param([Parameter(Mandatory = $true)][string]$Hz6Root)

    $libRoots = @(
        "api",
        "frontcache",
        "fronts",
        "owner",
        "policy",
        "route",
        "scavenge",
        "source",
        "transfer"
    ) | ForEach-Object { Join-Path $Hz6Root $_ }

    Get-ChildItem -File -Recurse -Path $libRoots |
        Where-Object {
            $_.Extension -eq ".c" -and
            $_.Name -notlike "linux_source_mmap*"
        } |
        Sort-Object FullName |
        ForEach-Object { $_.FullName }
}

function Get-Hz6WinClangCommonFlags {
    @(
        "/nologo",
        "/O2",
        "/DNDEBUG",
        "/std:c11",
        "/W3",
        "/WX",
        "/MD",
        "/DWIN32_LEAN_AND_MEAN",
        "/DNOMINMAX",
        "/D_CRT_SECURE_NO_WARNINGS"
    )
}
