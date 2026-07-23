param(
    [string]$OutputDir = (Join-Path $env:LOCALAPPDATA "Temp\hakozuna-hz8-release-smoke")
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$clang = (Get-Command clang-cl -ErrorAction Stop).Source
$sources = Get-ChildItem (Join-Path $root "src") -Filter "*.c" |
    ForEach-Object { $_.FullName }
$smokeSource = Join-Path $root "tests\h8_smoke.c"
$smokeExe = Join-Path $OutputDir "h8_release_smoke.exe"

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$defaultFlags = @(
    "/nologo",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/DH8_ENABLE_DEBUG_STATS=1",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1",
    "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
    "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
    "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
    "/DH8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1=1",
    "/DH8_MEDIUM_PAGE_ENTRY_BOUNDARY_L1=1",
    "/DH8_SMALL_TRANSITION_INVENTORY_L1=1",
    "/I$(Join-Path $root 'include')",
    "/I$(Join-Path $root 'src')"
)

& $clang @defaultFlags "/Fe$smokeExe" @sources $smokeSource
if ($LASTEXITCODE -ne 0) {
    throw "HZ8 release smoke build failed with exit code $LASTEXITCODE"
}

try {
    $env:H8_SMOKE_REGULAR_ADOPTION = "0"
    & $smokeExe
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 release smoke failed with adoption disabled: $LASTEXITCODE"
    }

    $env:H8_SMOKE_REGULAR_ADOPTION = "1"
    & $smokeExe
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 release smoke failed with adoption enabled: $LASTEXITCODE"
    }
} finally {
    Remove-Item Env:H8_SMOKE_REGULAR_ADOPTION -ErrorAction SilentlyContinue
}

Write-Host "HZ8 release smoke passed: $smokeExe"
