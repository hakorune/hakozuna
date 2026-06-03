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
        -CapacityLanes @("descavail-noboost-route4k") `
        -Note "selected balanced / wide_ws low-RSS speed lane"

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
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k", "ownerlocalityfast-rsscap-2-desc160k-front4k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc") `
        -Note "Larson full 10k selected lane plus lower-RSS siblings"

    "larson-cross-owner-lowest-rss" = New-Preset `
        -Name "larson-cross-owner-lowest-rss" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc") `
        -Note "front4k versus thindesc lowest-RSS Larson sibling check"

    "selected-family-guard" = New-Preset `
        -Name "selected-family-guard" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("smoke", "balanced", "wide_ws", "larger_sizes") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("route4k", "descavail-noboost-route4k", "largerlowrss-front8k-sourcerun-desc8k-route8k") `
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
    $args = @(
        "-OutputDir", $presetOutputDir,
        "-Runs", [string]$Runs,
        "-Families", ($preset.Families -join ","),
        "-BenchmarkProfiles", ($preset.BenchmarkProfiles -join ","),
        "-Hz6Profiles", ($preset.Hz6Profiles -join ","),
        "-CapacityLanes", ($preset.CapacityLanes -join ","),
        "-TimeoutSeconds", [string]$TimeoutSeconds
    )

    if ($SkipBuild) { $args += "-SkipBuild" }
    if ($ForceBuild) { $args += "-ForceBuild" }
    if ($DiagnosticHz6Probes) { $args += "-DiagnosticHz6Probes" }
    if ($ContinueOnFailure) { $args += "-ContinueOnFailure" }

    Write-Host ("[selected-family] {0}: {1}" -f $preset.Name, $preset.Note)
    & $MatrixScript @args
    if ($LASTEXITCODE -ne 0) {
        throw "run_win_hz6_capacity_matrix.ps1 failed for preset $($preset.Name) with exit code $LASTEXITCODE"
    }
}
