#!/usr/bin/env pwsh
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('mimalloc', 'tcmalloc')]
    [string]$ProviderId,

    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,

    [string]$Version = "local-$(Get-Date -Format yyyyMMdd)",
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\.artifacts\providers'),
    [string]$LicensePath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Add-PackageArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [Parameter(Mandatory = $true)][string]$Kind,
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [string]$WorkloadId
    )

    if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf)) {
        throw "Provider artifact was not found: $SourcePath"
    }

    $destinationName = [System.IO.Path]::GetFileName($SourcePath)
    $destination = Join-Path $script:stagingArtifacts $destinationName
    Copy-Item -LiteralPath $SourcePath -Destination $destination -Force
    $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    $relativePath = "artifacts/$destinationName"
    $artifact = [ordered]@{
        id = $Id
        kind = $Kind
        relativePath = $relativePath
        sha256 = $hash
    }
    if ($WorkloadId) { $artifact.workloadId = $WorkloadId }
    $script:manifestArtifacts.Add([pscustomobject]$artifact)
}

function Add-PackageTextArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $true)][string]$Contents
    )

    $destination = Join-Path $script:stagingArtifacts $FileName
    [System.IO.File]::WriteAllText($destination, $Contents, [System.Text.UTF8Encoding]::new($false))
    $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    $script:manifestArtifacts.Add([pscustomobject][ordered]@{
        id = $Id
        kind = 'auxiliary'
        relativePath = "artifacts/$FileName"
        sha256 = $hash
    })
}

$sourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
$outputRoot = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    [System.IO.Path]::GetFullPath($OutputDirectory)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot $OutputDirectory))
}
$packageId = "hakozuna-$ProviderId"
$packagePath = Join-Path $outputRoot "$packageId-$Version.hbl-provider.zip"
$stagingRoot = Join-Path $outputRoot ".staging-$packageId-$Version-$([Guid]::NewGuid().ToString('N'))"
$script:stagingArtifacts = Join-Path $stagingRoot 'artifacts'
$licensesRoot = Join-Path $stagingRoot 'LICENSES'
$manifestArtifacts = [System.Collections.Generic.List[object]]::new()

try {
    New-Item -ItemType Directory -Force -Path $script:stagingArtifacts, $licensesRoot | Out-Null

    $environment = [ordered]@{}
    $description = ''
    if ($ProviderId -eq 'mimalloc') {
        $source = Join-Path $sourceRoot 'private\windows-allocators\mimalloc\bin'
        Add-PackageArtifact 'allocator' 'allocatorLibrary' (Join-Path $source 'mimalloc.dll')
        Add-PackageArtifact 'redirect' 'auxiliary' (Join-Path $source 'mimalloc-redirect.dll')
        Add-PackageArtifact 'minject' 'auxiliary' (Join-Path $source 'minject.exe')
        Add-PackageTextArtifact 'launcher' 'run_with_mimalloc.ps1' @'
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)][string]$TargetExe,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$TargetArgs
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$launcherRoot = Join-Path $root '.launcher'
New-Item -ItemType Directory -Force -Path $launcherRoot | Out-Null
$targetName = [System.IO.Path]::GetFileNameWithoutExtension($TargetExe)
$targetExt = [System.IO.Path]::GetExtension($TargetExe)
$patched = Join-Path $launcherRoot "$targetName-mimalloc$targetExt"
Copy-Item -LiteralPath $TargetExe -Destination $patched -Force
Copy-Item -LiteralPath (Join-Path $root 'mimalloc.dll') -Destination (Join-Path $launcherRoot 'mimalloc.dll') -Force
Copy-Item -LiteralPath (Join-Path $root 'mimalloc-redirect.dll') -Destination (Join-Path $launcherRoot 'mimalloc-redirect.dll') -Force
& (Join-Path $root 'minject.exe') --force --inplace $patched | Write-Verbose
if ($LASTEXITCODE -ne 0) { throw "minject failed with exit code $LASTEXITCODE" }
$env:MIMALLOC_DISABLE_REDIRECT = $null
$env:PATH = "$launcherRoot;$env:PATH"
& $patched @TargetArgs
exit $LASTEXITCODE
'@
        $environment = [ordered]@{
            BENCHLAB_ALLOCATOR_API = 'mimalloc'
            BENCHLAB_ALLOCATOR_DLL = 'artifacts/mimalloc.dll'
            MIMALLOC_VERBOSE = '0'
            BENCHLAB_PROVIDER_LAUNCHER = 'artifacts/run_with_mimalloc.ps1'
        }
        $description = 'Windows x64 mimalloc provider pack for isolated Bench Lab child processes. The launcher uses the packaged minject and redirect DLLs.'
    }
    else {
        $source = Join-Path $sourceRoot 'private\windows-allocators\tcmalloc\bin'
        Add-PackageArtifact 'allocator' 'allocatorLibrary' (Join-Path $source 'benchlab_tcmalloc.dll')
        $runtime = Join-Path $source 'tcmalloc_minimal.dll'
        if (Test-Path -LiteralPath $runtime -PathType Leaf) {
            Add-PackageArtifact 'runtime' 'auxiliary' $runtime
        }
        $environment = [ordered]@{
            BENCHLAB_ALLOCATOR_API = 'tcmalloc_adapter'
            BENCHLAB_ALLOCATOR_DLL = 'artifacts/benchlab_tcmalloc.dll'
        }
        $description = 'Windows x64 tcmalloc adapter provider pack for isolated Bench Lab child processes.'
    }

    $notice = @"
Hakozuna Bench Lab local provider pack

Provider: $packageId
Version: $Version
Source root: $sourceRoot

This pack was generated for local benchmark evaluation. Before redistributing
it, replace this notice with the applicable upstream license files and verify
that the packaged binaries may be redistributed under those licenses.
"@
    [System.IO.File]::WriteAllText((Join-Path $licensesRoot 'PACK_NOTICES.txt'), $notice, [System.Text.UTF8Encoding]::new($false))

    if ($LicensePath) {
        if (-not (Test-Path -LiteralPath $LicensePath -PathType Leaf)) { throw "License file was not found: $LicensePath" }
        Copy-Item -LiteralPath $LicensePath -Destination (Join-Path $licensesRoot ([System.IO.Path]::GetFileName($LicensePath))) -Force
    }

    $manifest = [ordered]@{
        schemaVersion = 1
        id = $packageId
        label = if ($ProviderId -eq 'mimalloc') { 'mimalloc' } else { 'tcmalloc' }
        version = $Version
        platform = 'windows'
        architecture = 'x64'
        laneKind = 'speed'
        artifacts = @($manifestArtifacts)
        environmentProfiles = @([ordered]@{
            id = 'windows-default'
            label = 'Windows default'
            variables = $environment
        })
        description = $description
    }

    $manifestPath = Join-Path $stagingRoot 'provider.json'
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding utf8NoBOM
    $sumLines = foreach ($artifact in $manifestArtifacts) {
        "$($artifact.sha256)  $($artifact.relativePath)"
    }
    Set-Content -LiteralPath (Join-Path $stagingRoot 'SHA256SUMS') -Value $sumLines -Encoding utf8NoBOM

    New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
    if (Test-Path -LiteralPath $packagePath) { Remove-Item -LiteralPath $packagePath -Force }
    Compress-Archive -Path (Join-Path $stagingRoot '*') -DestinationPath $packagePath -CompressionLevel Optimal
    Write-Output "Created provider pack: $packagePath"
}
finally {
    if (Test-Path -LiteralPath $stagingRoot) { Remove-Item -LiteralPath $stagingRoot -Recurse -Force }
}
