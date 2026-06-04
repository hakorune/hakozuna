param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$TimeoutSeconds = 180,
    [switch]$SkipBuild,
    [switch]$ForceBuild,
    [switch]$DiagnosticHz6Probes,
    [switch]$ContinueOnFailure,
    [switch]$SelectedFamily,
    [switch]$SelectedFamilyGuard,
    [switch]$LarsonCrossOwnerSelected,
    [switch]$LarsonCrossOwnerLowestRss,
    [switch]$LarsonMetadataSlim,
    [switch]$LarsonSourceRunMetaSlim,
    [switch]$LarsonRun512RouteSlim,
    [switch]$LarsonRun512DescSlim,
    [switch]$LarsonRun512DescriptorLayout,
    [switch]$LarsonThinDescSourceCap,
    [switch]$ListPresets
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$MatrixScript = Join-Path $PSScriptRoot "run_win_hz6_capacity_matrix.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper\hz6_selected_family"
}

function New-Preset {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$Families,
        [Parameter(Mandatory = $true)][string[]]$BenchmarkProfiles,
        [Parameter(Mandatory = $true)][string[]]$Hz6Profiles,
        [Parameter(Mandatory = $true)][string[]]$CapacityLanes,
        [string]$Note = ""
    )

    @{
        Name = $Name
        Families = $Families
        BenchmarkProfiles = $BenchmarkProfiles
        Hz6Profiles = $Hz6Profiles
        CapacityLanes = $CapacityLanes
        Note = $Note
    }
}

$presetMap = [ordered]@{
    "selected-mixed-lowrss" = New-Preset `
        -Name "selected-mixed-lowrss" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("balanced", "wide_ws") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap") `
        -Note "selected mixed_ws clean low-RSS row: desc17/route17 with linear wrap route probing"

    "selected-mixed-pressure" = New-Preset `
        -Name "selected-mixed-pressure" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("balanced", "wide_ws") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("descavail-noboost-route4k") `
        -Note "balanced / wide_ws low-RSS pressure row; fast but not safety-clean"

    "selected-random-sameowner" = New-Preset `
        -Name "selected-random-sameowner" `
        -Families @("random_mixed") `
        -BenchmarkProfiles @("small", "medium", "mixed") `
        -Hz6Profiles @("strict") `
        -CapacityLanes @("sameownerfast-descavail-noboost-route4k") `
        -Note "selected random_mixed same-owner speed lane"

    "selected-larger-lowrss" = New-Preset `
        -Name "selected-larger-lowrss" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("larger_sizes") `
        -Hz6Profiles @("speed", "rss") `
        -CapacityLanes @("largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "selected larger_sizes RSS/speed lane"

    "larson-cross-owner-selected" = New-Preset `
        -Name "larson-cross-owner-selected" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k", "ownerlocalityfast-rsscap-2-desc160k-front4k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512") `
        -Note "Larson full 10k selected lane plus current dir192k/no-backptr lowest-RSS sibling"

    "larson-cross-owner-lowest-rss" = New-Preset `
        -Name "larson-cross-owner-lowest-rss" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512") `
        -Note "front4k versus route192k, no-backptr, and selected dir192k lowest-RSS Larson siblings"

    "larson-thindesc-sourcecap" = New-Preset `
        -Name "larson-thindesc-sourcecap" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k") `
        -Note "thindesc source-block capacity recovery check after full-10k warmup failure"

    "larson-metadata-slim" = New-Preset `
        -Name "larson-metadata-slim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k") `
        -Note "MetadataSlim-L1 route-table capacity ladder for the selected thindesc-source16k Larson lane"

    "larson-sourcerun-metaslim" = New-Preset `
        -Name "larson-sourcerun-metaslim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512") `
        -Note "SourceBlockMetaSlim-L1 run-slot bitmap ladder on top of the selected route192k Larson lane"

    "larson-run512-routeslim" = New-Preset `
        -Name "larson-run512-routeslim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512") `
        -Note "Route-table capacity re-check after the selected run512 source-run metadata slim lane"

    "larson-run512-descslim" = New-Preset `
        -Name "larson-run512-descslim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512") `
        -Note "Descriptor-table capacity re-check after route192k-run512 closed the static route trim path"

    "larson-run512-descriptorlayout" = New-Preset `
        -Name "larson-run512-descriptorlayout" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512") `
        -Note "Descriptor layout: baseline, no-backptr, and side-owner16 under the selected run512 lane"

    "selected-family-guard" = New-Preset `
        -Name "selected-family-guard" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("smoke", "balanced", "wide_ws", "larger_sizes") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("route4k", "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap", "descavail-noboost-route4k", "largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "short guard/control slice before a broader selected-family run"
}

function Write-PresetList {
    foreach ($preset in $presetMap.Values) {
        Write-Host ("{0}: {1}" -f $preset.Name, $preset.Note)
        Write-Host ("  Families: {0}" -f ($preset.Families -join ","))
        Write-Host ("  BenchmarkProfiles: {0}" -f ($preset.BenchmarkProfiles -join ","))
        Write-Host ("  Hz6Profiles: {0}" -f ($preset.Hz6Profiles -join ","))
        Write-Host ("  CapacityLanes: {0}" -f ($preset.CapacityLanes -join ","))
    }
}

if ($ListPresets) {
    Write-PresetList
    return
}

$selectedPresetNames = New-Object System.Collections.Generic.List[string]

if ($LarsonCrossOwnerLowestRss) {
    [void]$selectedPresetNames.Add("larson-cross-owner-lowest-rss")
}

if ($LarsonThinDescSourceCap) {
    [void]$selectedPresetNames.Add("larson-thindesc-sourcecap")
}

if ($LarsonMetadataSlim) {
    [void]$selectedPresetNames.Add("larson-metadata-slim")
}

if ($LarsonSourceRunMetaSlim) {
    [void]$selectedPresetNames.Add("larson-sourcerun-metaslim")
}

if ($LarsonRun512RouteSlim) {
    [void]$selectedPresetNames.Add("larson-run512-routeslim")
}

if ($LarsonRun512DescSlim) {
    [void]$selectedPresetNames.Add("larson-run512-descslim")
}

if ($LarsonRun512DescriptorLayout) {
    [void]$selectedPresetNames.Add("larson-run512-descriptorlayout")
}

if ($LarsonCrossOwnerSelected) {
    [void]$selectedPresetNames.Add("larson-cross-owner-selected")
}

if ($SelectedFamilyGuard) {
    [void]$selectedPresetNames.Add("selected-family-guard")
}

if ($SelectedFamily) {
    foreach ($name in @(
        "selected-mixed-lowrss",
        "selected-random-sameowner",
        "selected-larger-lowrss",
        "larson-cross-owner-selected"
    )) {
        [void]$selectedPresetNames.Add($name)
    }
}

if ($selectedPresetNames.Count -eq 0) {
    [void]$selectedPresetNames.Add("selected-family-guard")
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$seen = @{}
foreach ($presetName in $selectedPresetNames) {
    if ($seen.ContainsKey($presetName)) {
        continue
    }
    $seen[$presetName] = $true
    if (-not $presetMap.Contains($presetName)) {
        throw "Unknown selected-family preset: $presetName"
    }

    $preset = $presetMap[$presetName]
    $presetOutputDir = Join-Path $OutputDir $preset.Name
    $matrixArgs = @{
        OutputDir = $presetOutputDir
        Runs = $Runs
        Families = $preset.Families
        BenchmarkProfiles = $preset.BenchmarkProfiles
        Hz6Profiles = $preset.Hz6Profiles
        CapacityLanes = $preset.CapacityLanes
        TimeoutSeconds = $TimeoutSeconds
        SkipBuild = [bool]$SkipBuild
        ForceBuild = [bool]$ForceBuild
        DiagnosticHz6Probes = [bool]$DiagnosticHz6Probes
        ContinueOnFailure = [bool]$ContinueOnFailure
    }

    Write-Host ("[selected-family] {0}: {1}" -f $preset.Name, $preset.Note)
    & $MatrixScript @matrixArgs
    if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        throw "run_win_hz6_capacity_matrix.ps1 failed for preset $($preset.Name) with exit code $LASTEXITCODE"
    }
}
