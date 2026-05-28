param(
    [string]$CompilerPath = "clang-cl",
    [string]$OutDir,
    [switch]$SkipRun
)

$ErrorActionPreference = "Stop"

$Hz6Root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $OutDir = Join-Path $Hz6Root "out\win\r1_smokes"
}

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Compiler = Get-Command $CompilerPath -ErrorAction Stop
. (Join-Path $PSScriptRoot "hz6_win_build_common.ps1")

$IncludeFlags = Get-Hz6WinIncludeFlags -Hz6Root $Hz6Root
$LibSources = Get-Hz6WinLibSources -Hz6Root $Hz6Root

$Smokes = @(
    @{ Source = "tests\hz6_r1_core_contract_smoke.c"; Name = "hz6_r1_core_contract_smoke.exe" },
    @{ Source = "tests\hz6_r1_route_smoke.c"; Name = "hz6_r1_route_smoke.exe" },
    @{ Source = "tests\hz6_r1_transfer_contract_smoke.c"; Name = "hz6_r1_transfer_contract_smoke.exe" },
    @{ Source = "tests\hz6_r1_source_contract_smoke.c"; Name = "hz6_r1_source_contract_smoke.exe" },
    @{ Source = "tests\hz6_r1_allocator_smoke.c"; Name = "hz6_r1_allocator_smoke.exe" },
    @{ Source = "tests\hz6_r1_prefill_smoke.c"; Name = "hz6_r1_prefill_smoke.exe" },
    @{ Source = "tests\hz6_r1_sourceblock_smoke.c"; Name = "hz6_r1_sourceblock_smoke.exe" },
    @{ Source = "tests\hz6_r1_transfer_smoke.c"; Name = "hz6_r1_transfer_smoke.exe" },
    @{ Source = "tests\hz6_r1_reclaim_smoke.c"; Name = "hz6_r1_reclaim_smoke.exe" },
    @{ Source = "tests\hz6_r1_safety_smoke.c"; Name = "hz6_r1_safety_smoke.exe" }
)

function Invoke-CheckedCompile {
    param([string[]]$ArgList)

    & $Compiler.Source @ArgList
    if ($LASTEXITCODE -ne 0) {
        throw "clang-cl failed with exit code $LASTEXITCODE"
    }
}

function Invoke-CheckedRun {
    param([string]$Path)

    & $Path
    if ($LASTEXITCODE -ne 0) {
        throw "smoke failed: $Path rc=$LASTEXITCODE"
    }
}

$CommonFlags = Get-Hz6WinClangCommonFlags

foreach ($smoke in $Smokes) {
    $outputPath = Join-Path $OutDir $smoke.Name
    $smokeSource = Join-Path $Hz6Root $smoke.Source
    if (-not (Test-Path $smokeSource)) {
        throw "Smoke source not found: $smokeSource"
    }

    $Args = @()
    $Args += $CommonFlags
    $Args += $IncludeFlags
    $Args += $LibSources
    $Args += $smokeSource
    $Args += "/Fe:$outputPath"

    Write-Host "[hz6-win] building $($smoke.Name)"
    Invoke-CheckedCompile -ArgList $Args

    if (-not $SkipRun) {
        Write-Host "[hz6-win] running $($smoke.Name)"
        Invoke-CheckedRun -Path $outputPath
    }
}

Write-Host "HZ6 Windows R1 smoke build output: $OutDir"
