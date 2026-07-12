param(
    [string]$VcpkgRoot,
    [switch]$DiagnosticHz6Probes,
    [switch]$OnlyHz11,
    [switch]$OnlyHz8
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_suite"

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Hz3Script = Join-Path $RepoRoot "hakozuna\\win\\build_win_bench_compare.ps1"
$Hz4Script = Join-Path $RepoRoot "hakozuna-mt\\win\\build_win_bench_compare.ps1"
$Hz5Root = Join-Path $RepoRoot "hakozuna-hz5"
$Hz6Root = Join-Path $RepoRoot "hakozuna-hz6"
$Hz8Root = Join-Path $RepoRoot "hakozuna-hz8"
$Hz11Root = Join-Path $RepoRoot "hakozuna-hz11"
$Hz6Common = Join-Path $Hz6Root "win\\hz6_win_build_common.ps1"
$AppLikeBuildCommon = Join-Path $PSScriptRoot "bench_app_like_allocator_build_common.ps1"
$Compiler = Get-Command "clang-cl" -ErrorAction Stop
$CompareSource = Join-Path $PSScriptRoot "bench_allocator_compare.c"

function Invoke-Hz8AllocatorMatrixBuild {
    if (-not (Test-Path $Hz8Root)) {
        throw "HZ8 root not found; cannot build HZ8 matrix executable: $Hz8Root"
    }
    $Hz8Sources = Get-ChildItem (Join-Path $Hz8Root "src") -Filter "*.c" |
        ForEach-Object { $_.FullName }
    $Hz10Root = Join-Path $RepoRoot "hakozuna-hz10"
    $Hz10PageSources = @(
        "hz10_public_entry.c", "hz10_public_entry_owner.c", "hz10_class_pages.c",
        "hz10_retired_ready.c", "hz10_size_class.c", "hz10_large_alloc.c",
        "hz10_pooled_page.c", "hz10_page_pool.c", "hz10_remote_stack.c",
        "hz10_freelist_page.c", "hz10_pagemap.c", "hz10_platform.c"
    ) | ForEach-Object { Join-Path $Hz10Root "src\$_" }
    $Hz8CommonFlags = @(
        "/DHZ_BENCH_USE_HZ8=1",
        "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
        "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
        "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
        "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
        "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
        "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
        "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1"
    )
    foreach ($variant in @(
        @{ Name = "hz8-v2"; Output = "bench_mixed_ws_hz8_v2.exe"; ExtraFlags = @() },
        @{
            Name = "hz8-v2-nomag"
            Output = "bench_mixed_ws_hz8_v2_nomag.exe"
            ExtraFlags = @("/DH8_REUSABLE_SPAN_MAGAZINE_L1=0")
        },
        @{
            Name = "hz8-v2-mag32"
            Output = "bench_mixed_ws_hz8_v2_mag32.exe"
            ExtraFlags = @("/DH8_REUSABLE_SPAN_MAG_CAP=32")
        },
        @{
            Name = "hz8-v2-mediumlocalfast"
            Output = "bench_mixed_ws_hz8_v2_mediumlocalfast.exe"
            ExtraFlags = @("/DH8_MEDIUM_ENABLE_LOCAL_FAST_TIER=1")
        },
        @{
            Name = "hz8-medium-pageshadow"
            Output = "bench_mixed_ws_hz8_medium_pageshadow.exe"
            ExtraFlags = @("/DH8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0=1")
        },
        @{
            Name = "hz8-medium-page8k-local"
            Output = "bench_mixed_ws_hz8_medium_page8k_local.exe"
            ExtraFlags = @(
                "/DH8_MEDIUM_PAGE_SUBSTRATE_FIXED8K_L1=1",
                "/DHZ_BENCH_DISABLE_REALLOC=1",
                "/I$Hz10Root\src",
                "/I$RepoRoot\win\include"
            )
            ExtraSources = $Hz10PageSources
        },
        @{
            Name = "hz8-r3-page8k-integrated"
            Output = "bench_mixed_ws_hz8_medium_page8k_remote.exe"
            ExtraFlags = @(
                "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
                "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
                "/DHZ_BENCH_DISABLE_REALLOC=1"
            )
        },
        @{
            Name = "hz8-v3-adaptive-shadow"
            Output = "bench_mixed_ws_hz8_v3_adaptive_shadow.exe"
            ExtraFlags = @("/DH8_ADAPTIVE_TRANSFER_SHADOW_L0=1")
        },
        @{
            Name = "hz8-reclaim-shadow"
            Output = "bench_mixed_ws_hz8_reclaim_shadow.exe"
            ExtraFlags = @("/DH8_RECLAIM_ADAPTER_SHADOW_L0=1")
        },
        @{
            Name = "hz8-magazine-tail-shadow"
            Output = "bench_mixed_ws_hz8_magazine_tail_shadow.exe"
            ExtraFlags = @("/DH8_MAGAZINE_TAIL_RECLAIM_SHADOW_L0=1")
        },
        @{
            Name = "hz8-speed-attribution"
            Output = "bench_mixed_ws_hz8_speed_attribution.exe"
            ExtraFlags = @(
                "/DH8_SPEED_ATTRIBUTION_L0=1",
                "/DH8_ENABLE_DEBUG_STATS=1"
            )
        },
        @{
            Name = "hz8-small-reuse-visibility"
            Output = "bench_mixed_ws_hz8_small_reuse_visibility.exe"
            ExtraFlags = @(
                "/DH8_SMALL_REUSE_VISIBILITY_SHADOW_L0=1",
                "/DH8_SPEED_ATTRIBUTION_L0=1",
                "/DH8_ENABLE_DEBUG_STATS=1"
            )
        }
    )) {
        $output = Join-Path $OutDir $variant.Output
        $args = @(
            "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
            "/I$Hz8Root\include", "/I$Hz8Root\src"
        )
        $args += $Hz8CommonFlags
        $args += $variant.ExtraFlags
        $args += $Hz8Sources
        if ($variant.ExtraSources) {
            $args += $variant.ExtraSources
        }
        $args += $CompareSource
        $args += "psapi.lib"
        $args += "/Fe:$output"
        Write-Host "[hz8-win] building $($variant.Output)"
        & $Compiler.Source @args
        if ($LASTEXITCODE -ne 0) {
            throw "HZ8 mixed_ws build failed for $($variant.Name) with exit code $LASTEXITCODE"
        }
    }

    $shadowSmoke = Join-Path $Hz8Root "tests\h8_medium_page_shadow_smoke.c"
    $shadowSmokeOut = Join-Path $OutDir "h8_medium_page_shadow_smoke.exe"
    $shadowSmokeArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src"
    )
    $shadowSmokeArgs += $Hz8CommonFlags
    $shadowSmokeArgs += "/DH8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0=1"
    $shadowSmokeArgs += $Hz8Sources
    $shadowSmokeArgs += $shadowSmoke
    $shadowSmokeArgs += "/Fe:$shadowSmokeOut"
    Write-Host "[hz8-win] building h8_medium_page_shadow_smoke.exe"
    & $Compiler.Source @shadowSmokeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page shadow smoke build failed with exit code $LASTEXITCODE"
    }

    $pageBackendSmoke = Join-Path $Hz8Root "tests\h8_medium_page_backend_smoke.c"
    $pageBackendSmokeOut = Join-Path $OutDir "h8_medium_page_backend_smoke.exe"
    $pageBackendSmokeArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src", "/I$Hz10Root\src",
        "/I$RepoRoot\win\include",
        "/DH8_MEDIUM_PAGE_SUBSTRATE_FIXED8K_L1=1"
    )
    $pageBackendSmokeArgs += $Hz8CommonFlags
    $pageBackendSmokeArgs += $Hz8Sources
    $pageBackendSmokeArgs += $Hz10PageSources
    $pageBackendSmokeArgs += $pageBackendSmoke
    $pageBackendSmokeArgs += "/Fe:$pageBackendSmokeOut"
    Write-Host "[hz8-win] building h8_medium_page_backend_smoke.exe"
    & $Compiler.Source @pageBackendSmokeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page backend smoke build failed with exit code $LASTEXITCODE"
    }

    $pageRemoteSmoke = Join-Path $Hz8Root "tests\h8_medium_page8k_remote_smoke.c"
    $pageRemoteSmokeOut = Join-Path $OutDir "h8_medium_page8k_remote_smoke.exe"
    $pageRemoteSmokeArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src",
        "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
        "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
        (Join-Path $Hz8Root "src\h8_platform.c"),
        (Join-Path $Hz8Root "src\h8_medium_page8k_remote.c"),
        $pageRemoteSmoke,
        "/Fe:$pageRemoteSmokeOut"
    )
    Write-Host "[hz8-win] building h8_medium_page8k_remote_smoke.exe"
    & $Compiler.Source @pageRemoteSmokeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page8k remote smoke build failed with exit code $LASTEXITCODE"
    }

    $pageRemoteStress = Join-Path $Hz8Root "tests\h8_medium_page8k_remote_stress.c"
    $pageRemoteStressOut = Join-Path $OutDir "h8_medium_page8k_remote_stress.exe"
    $pageRemoteStressArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src",
        "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
        "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
        (Join-Path $Hz8Root "src\h8_platform.c"),
        (Join-Path $Hz8Root "src\h8_medium_page8k_remote.c"),
        $pageRemoteStress,
        "/Fe:$pageRemoteStressOut"
    )
    Write-Host "[hz8-win] building h8_medium_page8k_remote_stress.exe"
    & $Compiler.Source @pageRemoteStressArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page8k remote stress build failed with exit code $LASTEXITCODE"
    }

    $pageLifecycleSmoke = Join-Path $Hz8Root "tests\h8_medium_page8k_lifecycle_smoke.c"
    $pageLifecycleSmokeOut = Join-Path $OutDir "h8_medium_page8k_lifecycle_smoke.exe"
    $pageLifecycleSmokeArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src",
        "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
        "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
        (Join-Path $Hz8Root "src\h8_platform.c"),
        (Join-Path $Hz8Root "src\h8_medium_page8k_remote.c"),
        $pageLifecycleSmoke,
        "/Fe:$pageLifecycleSmokeOut"
    )
    Write-Host "[hz8-win] building h8_medium_page8k_lifecycle_smoke.exe"
    & $Compiler.Source @pageLifecycleSmokeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page8k lifecycle smoke build failed with exit code $LASTEXITCODE"
    }

    $pageLifecycleStress = Join-Path $Hz8Root "tests\h8_medium_page8k_lifecycle_stress.c"
    $pageLifecycleStressOut = Join-Path $OutDir "h8_medium_page8k_lifecycle_stress.exe"
    $pageLifecycleStressArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src",
        "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
        "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
        (Join-Path $Hz8Root "src\h8_platform.c"),
        (Join-Path $Hz8Root "src\h8_medium_page8k_remote.c"),
        $pageLifecycleStress,
        "/Fe:$pageLifecycleStressOut"
    )
    Write-Host "[hz8-win] building h8_medium_page8k_lifecycle_stress.exe"
    & $Compiler.Source @pageLifecycleStressArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page8k lifecycle stress build failed with exit code $LASTEXITCODE"
    }

    $pageResidencySmoke = Join-Path $Hz8Root "tests\h8_medium_page8k_residency_smoke.c"
    $pageResidencySmokeOut = Join-Path $OutDir "h8_medium_page8k_residency_smoke.exe"
    $pageResidencySmokeArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src",
        "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
        "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
        (Join-Path $Hz8Root "src\h8_platform.c"),
        (Join-Path $Hz8Root "src\h8_medium_page8k_remote.c"),
        $pageResidencySmoke,
        "/Fe:$pageResidencySmokeOut"
    )
    Write-Host "[hz8-win] building h8_medium_page8k_residency_smoke.exe"
    & $Compiler.Source @pageResidencySmokeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page8k residency smoke build failed with exit code $LASTEXITCODE"
    }

    $pageTurnoverSmoke = Join-Path $Hz8Root "tests\h8_medium_page8k_thread_turnover_smoke.c"
    $pageTurnoverSmokeOut = Join-Path $OutDir "h8_medium_page8k_thread_turnover_smoke.exe"
    $pageTurnoverSmokeArgs = @(
        "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
        "/I$Hz8Root\include", "/I$Hz8Root\src",
        "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
        "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
        (Join-Path $Hz8Root "src\h8_platform.c"),
        (Join-Path $Hz8Root "src\h8_medium_page8k_remote.c"),
        $pageTurnoverSmoke,
        "/Fe:$pageTurnoverSmokeOut"
    )
    Write-Host "[hz8-win] building h8_medium_page8k_thread_turnover_smoke.exe"
    & $Compiler.Source @pageTurnoverSmokeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 medium page8k thread turnover smoke build failed with exit code $LASTEXITCODE"
    }
}

function Invoke-Hz11AllocatorMatrixBuild {
    if (-not (Test-Path $Hz11Root)) {
        throw "HZ11 root not found; cannot build HZ11 matrix executable: $Hz11Root"
    }
    $Hz11Sources = @(
        (Join-Path $Hz11Root "src\hz11_size_class.c"),
        (Join-Path $Hz11Root "src\hz11_sys_alloc.c"),
        (Join-Path $Hz11Root "src\hz11_thread_cache.c"),
        (Join-Path $Hz11Root "src\hz11_public_entry.c"),
        (Join-Path $Hz11Root "src\hz11_span.c"),
        (Join-Path $Hz11Root "src\hz11_live_footprint.c")
    )
    $Hz11TransferSources = $Hz11Sources + @(
        (Join-Path $Hz11Root "src\hz11_transfer_cache.c"),
        (Join-Path $Hz11Root "src\hz11_alloc_diag.c")
    )
    $missing = $Hz11TransferSources | Where-Object { -not (Test-Path $_) }
    if ($missing.Count -gt 0) {
        throw "HZ11 source missing: $($missing -join ', ')"
    }

    foreach ($variant in @(
        @{
            Name = "hz11_span"
            Output = "bench_mixed_ws_hz11_span.exe"
            Sources = $Hz11Sources
            ExtraFlags = @()
        },
        @{
            Name = "hz11_span_transfer"
            Output = "bench_mixed_ws_hz11_span_transfer.exe"
            Sources = $Hz11TransferSources
            ExtraFlags = @("/DHZ11_TRANSFER_CENTRAL_SPAN=1")
        },
        @{
            Name = "hz11_span_transfer_fine128_win"
            Output = "bench_mixed_ws_hz11_span_transfer_fine128_win.exe"
            Sources = $Hz11TransferSources
            ExtraFlags = @(
                "/DHZ11_TLS_FASTPATH=1",
                "/DHZ11_CACHE_BYTE_ACCOUNTING=0",
                "/DHZ11_CACHE_SOA=1",
                "/DHZ11_TRANSFER_STATS=0",
                "/DHZ11_TRANSFER_CENTRAL_SPAN=1",
                "/DHZ11_CURRENT_SPAN_THREAD_EXIT=1",
                "/DHZ11_CENTRAL_CAP=65536",
                "/DHZ11_TRANSFER_BATCH=32",
                "/DHZ11_FINE_SIZE_CLASSES=1",
                "/DHZ11_FINE_LINEAR_MAX=128u"
            )
        },
        @{
            Name = "hz11_span_diag"
            Output = "bench_mixed_ws_hz11_span_diag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_tlsfast"
            Output = "bench_mixed_ws_hz11_span_tlsfast.exe"
            Sources = $Hz11Sources
            ExtraFlags = @("/DHZ11_TLS_FASTPATH=1")
        },
        @{
            Name = "hz11_span_cache256"
            Output = "bench_mixed_ws_hz11_span_cache256.exe"
            Sources = $Hz11Sources
            ExtraFlags = @("/DHZ11_CACHE_CAP=256")
        },
        @{
            Name = "hz11_span_cache256_bumpbatch16"
            Output = "bench_mixed_ws_hz11_span_cache256_bumpbatch16.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_SPAN_BUMP_BATCH=1",
                "/DHZ11_SPAN_BUMP_BATCH_COUNT=16"
            )
        },
        @{
            Name = "hz11_span_cache256_bumpbatch16_diag"
            Output = "bench_mixed_ws_hz11_span_cache256_bumpbatch16_diag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_SPAN_BUMP_BATCH=1",
                "/DHZ11_SPAN_BUMP_BATCH_COUNT=16",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_CLASS_DIAG=1",
                "/DHZ11_MATRIX_ATTRIB_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache256_returnedrange"
            Output = "bench_mixed_ws_hz11_span_cache256_returnedrange.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_RETURNED_PUSH_RANGE=1"
            )
        },
        @{
            Name = "hz11_span_cache256_returnedrange_diag"
            Output = "bench_mixed_ws_hz11_span_cache256_returnedrange_diag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_RETURNED_PUSH_RANGE=1",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_CLASS_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache256_returnedrange32"
            Output = "bench_mixed_ws_hz11_span_cache256_returnedrange32.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_RETURNED_PUSH_RANGE=1",
                "/DHZ11_RETURNED_PUSH_RANGE_CHUNK=32"
            )
        },
        @{
            Name = "hz11_span_tlsfast_cache256"
            Output = "bench_mixed_ws_hz11_span_tlsfast_cache256.exe"
            Sources = $Hz11Sources
            ExtraFlags = @("/DHZ11_TLS_FASTPATH=1", "/DHZ11_CACHE_CAP=256")
        },
        @{
            Name = "hz11_span_cache256_diag"
            Output = "bench_mixed_ws_hz11_span_cache256_diag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache256_matrixattrib"
            Output = "bench_mixed_ws_hz11_span_cache256_matrixattrib.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=256",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_MATRIX_ATTRIB_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache512"
            Output = "bench_mixed_ws_hz11_span_cache512.exe"
            Sources = $Hz11Sources
            ExtraFlags = @("/DHZ11_CACHE_CAP=512")
        },
        @{
            Name = "hz11_span_cache512_diag"
            Output = "bench_mixed_ws_hz11_span_cache512_diag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache512_matrixattrib"
            Output = "bench_mixed_ws_hz11_span_cache512_matrixattrib.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_MATRIX_ATTRIB_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache512_classdiag"
            Output = "bench_mixed_ws_hz11_span_cache512_classdiag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_CLASS_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=32"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch16"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch16.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=16"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch16_4_7"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch16_4_7.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_MAX_CLASS=7",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=16"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch16_matrixattrib"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch16_matrixattrib.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=16",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_MATRIX_ATTRIB_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch16_coldskip"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch16_coldskip.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=16",
                "/DHZ11_RETURNED_REFILL_COLD_SKIP=1",
                "/DHZ11_RETURNED_REFILL_COLD_SKIP_BUDGET=8"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch16_coldskip_matrixattrib"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch16_coldskip_matrixattrib.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=16",
                "/DHZ11_RETURNED_REFILL_COLD_SKIP=1",
                "/DHZ11_RETURNED_REFILL_COLD_SKIP_BUDGET=8",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_MATRIX_ATTRIB_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        },
        @{
            Name = "hz11_span_cache512_classbatch_diag"
            Output = "bench_mixed_ws_hz11_span_cache512_classbatch_diag.exe"
            Sources = $Hz11Sources
            ExtraFlags = @(
                "/DHZ11_CACHE_CAP=512",
                "/DHZ11_RETURNED_REFILL_BATCH=1",
                "/DHZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4",
                "/DHZ11_RETURNED_REFILL_BATCH_COUNT=32",
                "/DHZ11_ENABLE_HOT_COUNTERS=1",
                "/DHZ11_SPAN_RETURNED_DIAG=1",
                "/DHZ11_CLASS_DIAG=1",
                "/DHZ_BENCH_HZ11_SUMMARY=1"
            )
        }
    )) {
        $Hz11Output = Join-Path $OutDir $variant.Output
        $Hz11Args = @(
            "/nologo",
            "/O2",
            "/DNDEBUG",
            "/std:c11",
            "/W3",
            "/MD",
            "/DHZ_BENCH_USE_HZ11=1",
            "/DHZ11_CLASSIFY_SPAN=1",
            "/DHZ_BENCH_DISABLE_REALLOC=1",
            "/I$Hz11Root\include",
            "/I$Hz11Root\src"
        )
        $Hz11Args += $variant.ExtraFlags
        $Hz11Args += $variant.Sources
        $Hz11Args += $CompareSource
        $Hz11Args += "psapi.lib"
        $Hz11Args += "/Fe:$Hz11Output"
        Write-Host "[hz11-win] building $($variant.Output)"
        & $Compiler.Source @Hz11Args
        if ($LASTEXITCODE -ne 0) {
            throw "HZ11 mixed_ws build failed for $($variant.Name) with exit code $LASTEXITCODE"
        }
    }
}

if ($OnlyHz11) {
    Invoke-Hz11AllocatorMatrixBuild
    Write-Host "Built HZ11 suite artifact in: $OutDir"
    return
}
if ($OnlyHz8) {
    Invoke-Hz8AllocatorMatrixBuild
    Write-Host "Built HZ8 suite artifacts in: $OutDir"
    return
}

$Hz3Args = @()
$Hz4Args = @()
if ($VcpkgRoot) {
    $Hz3Args += @("-VcpkgRoot", $VcpkgRoot)
    $Hz4Args += @("-VcpkgRoot", $VcpkgRoot)
}

& $Hz3Script @Hz3Args
if ($LASTEXITCODE -ne 0) {
    throw "hz3 build script failed with exit code $LASTEXITCODE"
}

& $Hz4Script @Hz4Args
if ($LASTEXITCODE -ne 0) {
    throw "hz4 build script failed with exit code $LASTEXITCODE"
}

$Hz3OutDir = Join-Path $RepoRoot "hakozuna\out_win_bench"
$Hz4OutDir = Join-Path $RepoRoot "hakozuna-mt\out_win_bench"

if (Test-Path $Hz5Root) {
    $Hz5IncludeFlags = @(
        "/I$Hz5Root\include",
        "/I$Hz5Root\contract",
        "/I$Hz5Root\core",
        "/I$Hz5Root\fallback",
        "/I$Hz5Root\lowpage",
        "/I$Hz5Root\policy",
        "/I$Hz5Root\route",
        "/I$Hz5Root\smallfront",
        "/I$Hz5Root\wrapper"
    )
    $Hz5LibSources = @(
        "$Hz5Root\contract\hz5_contract.c",
        "$Hz5Root\core\hz5_segment.c",
        "$Hz5Root\core\hz5_run.c",
        "$Hz5Root\core\hz5_owner.c",
        "$Hz5Root\core\hz5_remote.c",
        "$Hz5Root\core\hz5_tcache.c",
        "$Hz5Root\core\hz5_stats.c",
        "$Hz5Root\route\hz5_route.c",
        "$Hz5Root\wrapper\hz5_wrapper.c",
        "$Hz5Root\policy\hz5_policy.c",
        "$Hz5Root\lowpage\hz5_lowpage64.c",
        "$Hz5Root\lowpage\hz5_lowpage64_os.c",
        "$Hz5Root\lowpage\hz5_lowpage64_p43_segment.c"
    )
    $Hz5Output = Join-Path $OutDir "bench_mixed_ws_hz5_policy.exe"
    $Hz5Args = @(
        "/nologo",
        "/O2",
        "/DNDEBUG",
        "/std:c11",
        "/W3",
        "/MD",
        "/DHZ_BENCH_USE_HZ5_POLICY",
        "/DHZ_BENCH_DISABLE_REALLOC=1",
        "/DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN=1"
    )
    $Hz5Args += $Hz5IncludeFlags
    $Hz5Args += $Hz5LibSources
    $Hz5Args += $CompareSource
    $Hz5Args += "psapi.lib"
    $Hz5Args += "/Fe:$Hz5Output"
    Write-Host "[hz5-win] building bench_mixed_ws_hz5_policy.exe"
    & $Compiler.Source @Hz5Args
    if ($LASTEXITCODE -ne 0) {
        throw "HZ5 mixed_ws build failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Warning "HZ5 root not found; skipping HZ5 matrix executable: $Hz5Root"
}

if (Test-Path $Hz6Common) {
    . $Hz6Common
    if (Test-Path $AppLikeBuildCommon) {
        . $AppLikeBuildCommon
    } else {
        throw "HZ6 app-like build helper not found: $AppLikeBuildCommon"
    }
    $Hz6IncludeFlags = Get-Hz6WinIncludeFlags -Hz6Root $Hz6Root -ExtraIncludeRoots @("win")
    $Hz6LibSources = Get-Hz6WinLibSources -Hz6Root $Hz6Root
    if ((Split-Path -Leaf $CompareSource) -eq "bench_allocator_compare.c") {
        $Hz6LibSources += (Join-Path $RepoRoot "win\bench_allocator_compare_hz6_summary.c")
    }
    if ((Split-Path -Leaf $CompareSource) -eq "bench_redis_workload_compare.c") {
        $Hz6LibSources += (Join-Path $RepoRoot "win\bench_redis_hz6_diag.c")
    }
    $Hz6CommonFlags = Get-Hz6WinClangCommonFlags
    $Hz6DiagnosticFlags = @()
    if ($DiagnosticHz6Probes) {
        $Hz6DiagnosticFlags += "/DHZ6_DIAGNOSTIC_PROBES=1"
    }
    $Hz6Profiles = @(
        @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" },
        @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" },
        @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" }
    )
    $Hz6BroadCapacityFlags = Get-Hz6WinBroadCapacityFlags
    $Hz6Route4kCapacityFlags = Get-Hz6WinRoute4kCapacityFlags
    $Hz6LargerLowRssCapacityFlags = Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6LargeDirectRetain16mLargerLowRssCapacityFlags = Get-Hz6WinLargeDirectRetain16mLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6LargeDirectRetain32mLargerLowRssCapacityFlags = Get-Hz6WinLargeDirectRetain32mLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6SameOwnerFastLargerLowRssCapacityFlags = Get-Hz6WinSameOwnerFastLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6DirectLocalSmall8kSameOwnerLargeLargerLowRssCapacityFlags = Get-Hz6WinDirectLocalSmall8kSameOwnerLargeLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6DirectLocalFreeReuseLargerLowRssCapacityFlags = Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6DirectLocalFreeReuseSmall8kLargerLowRssCapacityFlags = Get-Hz6WinDirectLocalFreeReuseSmall8kLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags

    foreach ($profile in $Hz6Profiles) {
        foreach ($variant in @(
            @{ Suffix = ""; ExtraFlags = @() },
            @{ Suffix = "_broad"; ExtraFlags = $Hz6BroadCapacityFlags },
            @{ Suffix = "_route4k"; ExtraFlags = $Hz6Route4kCapacityFlags },
            @{ Suffix = "_largerlowrss"; ExtraFlags = $Hz6LargerLowRssCapacityFlags },
            @{ Suffix = "_largedirectretain16m_largerlowrss"; ExtraFlags = $Hz6LargeDirectRetain16mLargerLowRssCapacityFlags },
            @{ Suffix = "_largedirectretain32m_largerlowrss"; ExtraFlags = $Hz6LargeDirectRetain32mLargerLowRssCapacityFlags },
            @{ Suffix = "_sameownerfast_largerlowrss"; ExtraFlags = $Hz6SameOwnerFastLargerLowRssCapacityFlags },
            @{ Suffix = "_directlocalsmall8k_sameownerlarge_largerlowrss"; ExtraFlags = $Hz6DirectLocalSmall8kSameOwnerLargeLargerLowRssCapacityFlags },
            @{ Suffix = "_directlocalfreereuse_largerlowrss"; ExtraFlags = $Hz6DirectLocalFreeReuseLargerLowRssCapacityFlags },
            @{ Suffix = "_directlocalfreereuse_small8k_largerlowrss"; ExtraFlags = $Hz6DirectLocalFreeReuseSmall8kLargerLowRssCapacityFlags }
        )) {
            $output = Join-Path $OutDir ("bench_mixed_ws_hz6_{0}{1}.exe" -f $profile.Name, $variant.Suffix)
            $args = @()
            $args += $Hz6CommonFlags
            $args += $Hz6DiagnosticFlags
            $args += "/DHZ_BENCH_USE_HZ6"
            $args += "/DHZ_BENCH_DISABLE_REALLOC=1"
            $args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
            $args += $variant.ExtraFlags
            $args += $Hz6IncludeFlags
            $args += $Hz6LibSources
            $args += $CompareSource
            $args += "psapi.lib"
            $args += "/Fe:$output"
            Write-Host ("[hz6-win] building {0}" -f (Split-Path -Leaf $output))
            & $Compiler.Source @args
            if ($LASTEXITCODE -ne 0) {
                throw "HZ6 mixed_ws build failed for $($profile.Name)$($variant.Suffix) with exit code $LASTEXITCODE"
            }
        }
    }
} else {
    Write-Warning "HZ6 Windows build helper not found; skipping HZ6 matrix executables: $Hz6Common"
}

Invoke-Hz11AllocatorMatrixBuild
Invoke-Hz8AllocatorMatrixBuild

$Artifacts = @(
    (Join-Path $Hz3OutDir "bench_mixed_ws_crt.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_hz3.exe"),
    (Join-Path $Hz4OutDir "bench_mixed_ws_hz4.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_mimalloc.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_tcmalloc.exe"),
    (Join-Path $Hz3OutDir "mimalloc.dll"),
    (Join-Path $Hz3OutDir "mimalloc-redirect.dll"),
    (Join-Path $Hz3OutDir "tcmalloc_minimal.dll")
)

foreach ($artifact in $Artifacts) {
    if (Test-Path $artifact) {
        Copy-Item -Force $artifact $OutDir | Out-Null
    }
}

Write-Host "Built suite artifacts in: $OutDir"
