param(
    [ValidateSet('win-x64', 'win-arm64')]
    [string]$Runtime = 'win-x64',
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$labRoot = Split-Path -Parent $PSScriptRoot
$artifactRoot = Join-Path $labRoot '.artifacts'
$stage = Join-Path $artifactRoot "HakozunaBenchLab-$Runtime"
$archive = "$stage.zip"
$project = Join-Path $labRoot 'src/HakozunaBenchLab.App/HakozunaBenchLab.App.csproj'

if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Force
}

New-Item -ItemType Directory -Path $stage | Out-Null
dotnet publish $project `
    --configuration $Configuration `
    --runtime $Runtime `
    --self-contained true `
    --output $stage `
    -p:PublishSingleFile=true `
    -p:DebugType=None `
    -p:DebugSymbols=false

Copy-Item -LiteralPath (Join-Path $labRoot 'README.md') -Destination $stage
Copy-Item -LiteralPath (Join-Path $labRoot 'README.ja.md') -Destination $stage
Copy-Item -LiteralPath (Join-Path $labRoot 'docs/DISTRIBUTION_AND_PROVIDERS.md') -Destination $stage

Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -CompressionLevel Optimal
Write-Output "Portable archive: $archive"
