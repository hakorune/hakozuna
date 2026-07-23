#!/usr/bin/env pwsh
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$PackagePath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not (Test-Path -LiteralPath $PackagePath -PathType Leaf)) {
    throw "Provider package was not found: $PackagePath"
}

$verifyRoot = Join-Path $env:TEMP ("hbl-provider-verify-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $verifyRoot | Out-Null
try {
    Expand-Archive -LiteralPath $PackagePath -DestinationPath $verifyRoot -Force
    $manifestPath = Join-Path $verifyRoot 'provider.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw 'Provider package has no root provider.json.'
    }

    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    if ($manifest.schemaVersion -ne 1) { throw 'Unsupported provider schema.' }
    if ($manifest.platform -ne 'windows' -or $manifest.architecture -ne 'x64') {
        throw "Unexpected provider target: $($manifest.platform)-$($manifest.architecture)"
    }

    foreach ($artifact in $manifest.artifacts) {
        $artifactPath = Join-Path $verifyRoot $artifact.relativePath
        if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
            throw "Missing declared artifact: $($artifact.relativePath)"
        }
        $actual = (Get-FileHash -LiteralPath $artifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actual -ne $artifact.sha256) {
            throw "SHA-256 mismatch: $($artifact.relativePath)"
        }
    }

    foreach ($profile in $manifest.environmentProfiles) {
        foreach ($value in $profile.variables.PSObject.Properties.Value) {
            if ([System.IO.Path]::IsPathRooted([string]$value)) {
                throw "Environment profile contains an absolute path: $value"
            }
        }
    }

    foreach ($required in @('SHA256SUMS', 'LICENSES\PACK_NOTICES.txt')) {
        if (-not (Test-Path -LiteralPath (Join-Path $verifyRoot $required) -PathType Leaf)) {
            throw "Required package file is missing: $required"
        }
    }

    Write-Output "Verified provider pack: $PackagePath"
    Write-Output "  id=$($manifest.id) version=$($manifest.version) artifacts=$($manifest.artifacts.Count) profiles=$($manifest.environmentProfiles.Count)"
}
finally {
    if (Test-Path -LiteralPath $verifyRoot) {
        Remove-Item -LiteralPath $verifyRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
