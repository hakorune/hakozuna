#!/usr/bin/env pwsh
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string[]]$PackagePath,

    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'HakozunaBenchLab\providers')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$coreAssembly = Join-Path $repoRoot 'src\HakozunaBenchLab.Core\bin\Debug\net8.0\HakozunaBenchLab.Core.dll'
$agentAssembly = Join-Path $repoRoot 'src\HakozunaBenchLab.Agent\bin\Debug\net8.0\HakozunaBenchLab.Agent.dll'
foreach ($assembly in @($coreAssembly, $agentAssembly)) {
    if (-not (Test-Path -LiteralPath $assembly -PathType Leaf)) {
        throw "Build the GUI solution before running the installer smoke: $assembly"
    }
}
foreach ($package in $PackagePath) {
    if (-not (Test-Path -LiteralPath $package -PathType Leaf)) {
        throw "Provider package was not found: $package"
    }
}

$localAssemblyRoot = Join-Path $env:TEMP ("hbl-installer-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $localAssemblyRoot | Out-Null
try {
    $localCore = Join-Path $localAssemblyRoot 'HakozunaBenchLab.Core.dll'
    $localAgent = Join-Path $localAssemblyRoot 'HakozunaBenchLab.Agent.dll'
    Copy-Item -LiteralPath $coreAssembly -Destination $localCore
    Copy-Item -LiteralPath $agentAssembly -Destination $localAgent
    Add-Type -Path $localCore
    Add-Type -Path $localAgent

    $options = [HakozunaBenchLab.Agent.ProviderInstallOptions]::new('windows', 'x64')
    $installer = [HakozunaBenchLab.Agent.ProviderPackageInstaller]::new()
    foreach ($package in $PackagePath) {
        $installed = $installer.InstallAsync($package, $InstallRoot, $options).GetAwaiter().GetResult()
        Write-Output "Installed $($installed.Manifest.Id)/$($installed.Manifest.Version)"
        Write-Output "  directory=$($installed.InstallDirectory)"
        Write-Output "  artifacts=$($installed.Manifest.Artifacts.Count) profiles=$($installed.Manifest.EnvironmentProfiles.Count)"
    }
}
finally {
    if (Test-Path -LiteralPath $localAssemblyRoot) {
        Remove-Item -LiteralPath $localAssemblyRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
