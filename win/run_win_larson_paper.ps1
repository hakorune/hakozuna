param(
    [string]$OutputDir,
    [int]$Runs = 5,
    [int[]]$ThreadCounts,
    [int]$TimeoutSeconds = 120,
    [switch]$IncludeCompactControl,
    [switch]$IncludeWorkerWarmupControl,
    [int]$CompactChunksPerThread = 400,
    [switch]$IncludeHz6CapacityControls,
    [switch]$LarsonAppcapOnly,
    [switch]$ContinueOnFailure
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_larson"
$BuildScript = Join-Path $PSScriptRoot "build_win_larson_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_larson_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_larson_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_larson_hz4.exe") },
    @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_larson_hz5_policy.exe") },
    @{ Name = "hz11-span"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span.exe") },
    @{ Name = "hz11-span-cache256"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256.exe") },
    @{ Name = "hz11-span-cache512-classbatch16-coldskip"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache512_classbatch16_coldskip.exe") },
    @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_appcap.exe") },
    @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_appcap.exe") },
    @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_appcap.exe") },
    @{ Name = "hz6-ownerlocality-appcap-strict"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_ownerlocality_appcap.exe") },
    @{ Name = "hz6-ownerlocality-appcap-speed"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_ownerlocality_appcap.exe") },
    @{ Name = "hz6-ownerlocality-appcap-rss"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_ownerlocality_appcap.exe") },
    @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_route4k.exe") },
    @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_route4k.exe") },
    @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_route4k.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_larson_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_larson_tcmalloc.exe") }
)

if ($LarsonAppcapOnly) {
    $Executables = @(
        @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_appcap.exe") },
        @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_appcap.exe") },
        @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_appcap.exe") },
        @{ Name = "hz6-ownerlocality-appcap-speed"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_ownerlocality_appcap.exe") }
    )
} elseif ($IncludeHz6CapacityControls) {
    $Executables = @(
        @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_larson_crt.exe") },
        @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_larson_hz3.exe") },
        @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_larson_hz4.exe") },
        @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_larson_hz5_policy.exe") },
        @{ Name = "hz11-span"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span.exe") },
        @{ Name = "hz11-span-cache256"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256.exe") },
        @{ Name = "hz11-span-cache256-bumpbatch16"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256_bumpbatch16.exe") },
        @{ Name = "hz11-span-cache256-bumpbatch16-diag"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256_bumpbatch16_diag.exe") },
        @{ Name = "hz11-span-cache256-returnedrange"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256_returnedrange.exe") },
        @{ Name = "hz11-span-cache256-returnedrange-diag"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256_returnedrange_diag.exe") },
        @{ Name = "hz11-span-cache256-returnedrange32"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache256_returnedrange32.exe") },
        @{ Name = "hz11-span-cache512-classbatch16-coldskip"; Path = (Join-Path $SuiteDir "bench_larson_hz11_span_cache512_classbatch16_coldskip.exe") },
        @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict.exe") },
        @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed.exe") },
        @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss.exe") },
        @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_broad.exe") },
        @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_broad.exe") },
        @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_broad.exe") },
        @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_route4k.exe") },
        @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_route4k.exe") },
        @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_route4k.exe") },
        @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_appcap.exe") },
        @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_appcap.exe") },
        @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_appcap.exe") },
        @{ Name = "hz6-ownerlocality-appcap-strict"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_ownerlocality_appcap.exe") },
        @{ Name = "hz6-ownerlocality-appcap-speed"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_ownerlocality_appcap.exe") },
        @{ Name = "hz6-ownerlocality-appcap-rss"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_ownerlocality_appcap.exe") },
        @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_larson_mimalloc.exe") },
        @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_larson_tcmalloc.exe") }
    )
}

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_larson_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Length -eq 0) {
        return [double]::NaN
    }
    $sorted = $Values | Sort-Object
    $mid = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [int]$TimeoutSeconds = 120
    )

    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("larson-capture-{0}" -f ([Guid]::NewGuid().ToString('N')))
    New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
    $stdoutPath = Join-Path $tempRoot 'stdout.log'
    $stderrPath = Join-Path $tempRoot 'stderr.log'
    $proc = $null
    $peakWorkingSetBytes = [Int64]0
    $timedOut = $false
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)

    try {
        $proc = Start-Process `
            -FilePath $FilePath `
            -ArgumentList $Arguments `
            -RedirectStandardOutput $stdoutPath `
            -RedirectStandardError $stderrPath `
            -PassThru `
            -WindowStyle Hidden

        try {
            $proc.Refresh()
            if ($proc.WorkingSet64 -gt $peakWorkingSetBytes) {
                $peakWorkingSetBytes = $proc.WorkingSet64
            }
            if ($proc.PeakWorkingSet64 -gt $peakWorkingSetBytes) {
                $peakWorkingSetBytes = $proc.PeakWorkingSet64
            }
        } catch {
        }

        while (-not $proc.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            try {
                $proc.Refresh()
                if ($proc.WorkingSet64 -gt $peakWorkingSetBytes) {
                    $peakWorkingSetBytes = $proc.WorkingSet64
                }
                if ($proc.PeakWorkingSet64 -gt $peakWorkingSetBytes) {
                    $peakWorkingSetBytes = $proc.PeakWorkingSet64
                }
            } catch {
            }
            Start-Sleep -Milliseconds 5
        }

        if (-not $proc.HasExited) {
            $timedOut = $true
            try { $proc.Kill($true) } catch {
                try { $proc.Kill() } catch {}
            }
            try { $proc.WaitForExit() } catch {}
            return @{
                ExitCode = 124
                TimedOut = $true
                Lines = @("[TIMEOUT] exceeded ${TimeoutSeconds}s")
                PeakKb = "NA"
            }
        }

        try {
            $proc.Refresh()
            if ($proc.WorkingSet64 -gt $peakWorkingSetBytes) {
                $peakWorkingSetBytes = $proc.WorkingSet64
            }
            if ($proc.PeakWorkingSet64 -gt $peakWorkingSetBytes) {
                $peakWorkingSetBytes = $proc.PeakWorkingSet64
            }
        } catch {
        }

        try { $proc.WaitForExit() } catch {}

        $stdout = if (Test-Path -LiteralPath $stdoutPath) {
            Get-Content -LiteralPath $stdoutPath -Raw
        } else {
            ""
        }
        $stderr = if (Test-Path -LiteralPath $stderrPath) {
            Get-Content -LiteralPath $stderrPath -Raw
        } else {
            ""
        }
        $lines = New-Object System.Collections.Generic.List[string]
        foreach ($chunk in @($stdout, $stderr)) {
            if (-not [string]::IsNullOrEmpty($chunk)) {
                foreach ($line in ($chunk -split "`r?`n")) {
                    if ($line -ne "") { $lines.Add($line) }
                }
            }
        }

        $peakKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
        if ($stdout -match 'peak_kb=([0-9]+)') {
            $peakKb = $Matches[1]
        } elseif ($stderr -match 'peak_kb=([0-9]+)') {
            $peakKb = $Matches[1]
        }

        return @{ ExitCode = $proc.ExitCode; TimedOut = $timedOut; Lines = $lines; PeakKb = $peakKb }
    } finally {
        if ($proc) {
            try { $proc.Dispose() } catch {}
        }
        try { Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue } catch {}
    }
}

function Parse-Hz6Stats {
    param([string[]]$Lines)

    $result = @{
        RouteMiss = "NA"
        RouteVisibilityLookup = "NA"
        RouteVisibilityHit = "NA"
        RouteVisibilityHitLocalOwner = "NA"
        RouteVisibilityHitForeignOwner = "NA"
        RouteVisibilityMiss = "NA"
        RouteVisibilityProbeTotal = "NA"
        RouteVisibilityProbeMax = "NA"
        SourceAlloc = "NA"
        TransferPush = "NA"
        TransferPop = "NA"
        TransferCurrent = "NA"
        TransferCurrentMax = "NA"
        RemoteFreeAttempt = "NA"
        RemoteFreeStrictOwnerBlock = "NA"
        RemoteFreeTransferFail = "NA"
        RouteRehomeAttempt = "NA"
        RouteRehomeSuccess = "NA"
        RouteRehomeFail = "NA"
        LifecycleOwnerMismatch = "NA"
        LifecycleForeignFreeAttempt = "NA"
        LifecycleForeignFreeHandled = "NA"
        LifecycleForeignFreeInvalid = "NA"
        VisibleFirstAttempt = "NA"
        VisibleFirstHit = "NA"
        VisibleFirstMiss = "NA"
        VisibleFirstVisibleInvalid = "NA"
        VisibleFirstLocalFallback = "NA"
        VisibleFirstLocalFallbackInvalid = "NA"
        VisibleFirstLocalLookupSkipped = "NA"
        NegativeFilterAttempt = "NA"
        NegativeFilterNotArmed = "NA"
        NegativeFilterRehomeBlocked = "NA"
        NegativeFilterSkipLocal = "NA"
        NegativeFilterMaybeLocal = "NA"
        NegativeFilterShadowFalseSkip = "NA"
        NegativeFilterShadowLocalValid = "NA"
        NegativeFilterShadowLocalInvalid = "NA"
        NegativeFilterRangeProbeTotal = "NA"
        NegativeFilterRangeProbeMax = "NA"
        SharedDirLookup = "NA"
        SharedDirHit = "NA"
        SharedDirMiss = "NA"
        SharedDirStale = "NA"
        SharedDirHitLocalAllocator = "NA"
        SharedDirHitForeignAllocator = "NA"
        SharedDirWouldSkipLocal = "NA"
        SharedDirRegister = "NA"
        SharedDirUnregister = "NA"
        SharedDirProbeTotal = "NA"
        SharedDirProbeMax = "NA"
        SharedDirFirstAttempt = "NA"
        SharedDirFirstHit = "NA"
        SharedDirFirstFallback = "NA"
        SharedDirFirstInvalid = "NA"
        SourceOwnedPrepare = "NA"
        SourceOwnedRouteHitLocalOwner = "NA"
        SourceOwnedVisibilityHitLocalOwner = "NA"
        SourceOwnedVisibilityHitForeignOwner = "NA"
        SourceOwnedRemoteFreeAttempt = "NA"
        SourceOwnedRelease = "NA"
        FrontcacheReuseHit = "NA"
        FrontcacheReuseInvalid = "NA"
        TransferReuseHit = "NA"
        TransferReuseInvalid = "NA"
        SourceRefillStarvation = "NA"
        SourceRefillSaturation = "NA"
        SourceRefillBoost = "NA"
        SourceRefillClamp = "NA"
        SourceAdmissionOpen = "NA"
        SourceAdmissionBoosted = "NA"
        SourceAdmissionClamped = "NA"
        SourcePrefillAttempt = "NA"
        SourcePrefillFilled = "NA"
        SourcePrefillFallback = "NA"
        FrontSourceOpsAlloc = "NA"
        FrontSourceSlotAlloc = "NA"
        FrontSourcePrefillAlloc = "NA"
        ToySourcePrefillCall = "NA"
        Local2pSourceAlloc = "NA"
        MidpageSourceAlloc = "NA"
        LargeSourceAlloc = "NA"
        ToySourceAlloc = "NA"
        AllocFail = "NA"
        DescriptorProbeTotal = "NA"
        DescriptorProbeMax = "NA"
        RouteRegisterProbeTotal = "NA"
        RouteRegisterProbeMax = "NA"
        RouteUnregisterProbeTotal = "NA"
        RouteUnregisterProbeMax = "NA"
        SourceBlockProbeTotal = "NA"
        SourceBlockProbeMax = "NA"
        FrontPathLocal2p = "NA"
        FrontPathMidpage = "NA"
        FrontPathLarge = "NA"
        FrontPathToy = "NA"
        FrontPrefillAttemptLocal2p = "NA"
        FrontPrefillAttemptMidpage = "NA"
        FrontPrefillAttemptLarge = "NA"
        FrontPrefillAttemptToy = "NA"
        FrontPrefillFilledLocal2p = "NA"
        FrontPrefillFilledMidpage = "NA"
        FrontPrefillFilledLarge = "NA"
        FrontPrefillFilledToy = "NA"
        FrontPrefillFallbackLocal2p = "NA"
        FrontPrefillFallbackMidpage = "NA"
        FrontPrefillFallbackLarge = "NA"
        FrontPrefillFallbackToy = "NA"
        FrontcacheClass = "NA"
    }

    foreach ($line in $Lines) {
        if ($line.StartsWith("[HZ6_STATS]")) {
            foreach ($part in $line.Split(" ")) {
                switch -Regex ($part) {
                    '^route_miss=(.*)$' { $result.RouteMiss = $Matches[1]; continue }
                    '^route_visibility_lookup=(.*)$' { $result.RouteVisibilityLookup = $Matches[1]; continue }
                    '^route_visibility_hit=(.*)$' { $result.RouteVisibilityHit = $Matches[1]; continue }
                    '^route_visibility_hit_local_owner=(.*)$' { $result.RouteVisibilityHitLocalOwner = $Matches[1]; continue }
                    '^route_visibility_hit_foreign_owner=(.*)$' { $result.RouteVisibilityHitForeignOwner = $Matches[1]; continue }
                    '^route_visibility_miss=(.*)$' { $result.RouteVisibilityMiss = $Matches[1]; continue }
                    '^route_visibility_probe_total=(.*)$' { $result.RouteVisibilityProbeTotal = $Matches[1]; continue }
                    '^route_visibility_probe_max=(.*)$' { $result.RouteVisibilityProbeMax = $Matches[1]; continue }
                    '^source_alloc=(.*)$' { $result.SourceAlloc = $Matches[1]; continue }
                    '^transfer_push=(.*)$' { $result.TransferPush = $Matches[1]; continue }
                    '^transfer_pop=(.*)$' { $result.TransferPop = $Matches[1]; continue }
                    '^transfer_current=(.*)$' { $result.TransferCurrent = $Matches[1]; continue }
                    '^transfer_current_max=(.*)$' { $result.TransferCurrentMax = $Matches[1]; continue }
                    '^remote_free_attempt=(.*)$' { $result.RemoteFreeAttempt = $Matches[1]; continue }
                    '^remote_free_strict_owner_block=(.*)$' { $result.RemoteFreeStrictOwnerBlock = $Matches[1]; continue }
                    '^remote_free_transfer_fail=(.*)$' { $result.RemoteFreeTransferFail = $Matches[1]; continue }
                    '^route_rehome_attempt=(.*)$' { $result.RouteRehomeAttempt = $Matches[1]; continue }
                    '^route_rehome_success=(.*)$' { $result.RouteRehomeSuccess = $Matches[1]; continue }
                    '^route_rehome_fail=(.*)$' { $result.RouteRehomeFail = $Matches[1]; continue }
                    '^lifecycle_owner_mismatch=(.*)$' { $result.LifecycleOwnerMismatch = $Matches[1]; continue }
                    '^lifecycle_foreign_free_attempt=(.*)$' { $result.LifecycleForeignFreeAttempt = $Matches[1]; continue }
                    '^lifecycle_foreign_free_handled=(.*)$' { $result.LifecycleForeignFreeHandled = $Matches[1]; continue }
                    '^lifecycle_foreign_free_invalid=(.*)$' { $result.LifecycleForeignFreeInvalid = $Matches[1]; continue }
                    '^visible_first_attempt=(.*)$' { $result.VisibleFirstAttempt = $Matches[1]; continue }
                    '^visible_first_hit=(.*)$' { $result.VisibleFirstHit = $Matches[1]; continue }
                    '^visible_first_miss=(.*)$' { $result.VisibleFirstMiss = $Matches[1]; continue }
                    '^visible_first_visible_invalid=(.*)$' { $result.VisibleFirstVisibleInvalid = $Matches[1]; continue }
                    '^visible_first_local_fallback=(.*)$' { $result.VisibleFirstLocalFallback = $Matches[1]; continue }
                    '^visible_first_local_fallback_invalid=(.*)$' { $result.VisibleFirstLocalFallbackInvalid = $Matches[1]; continue }
                    '^visible_first_local_lookup_skipped=(.*)$' { $result.VisibleFirstLocalLookupSkipped = $Matches[1]; continue }
                    '^negative_filter_attempt=(.*)$' { $result.NegativeFilterAttempt = $Matches[1]; continue }
                    '^negative_filter_not_armed=(.*)$' { $result.NegativeFilterNotArmed = $Matches[1]; continue }
                    '^negative_filter_rehome_blocked=(.*)$' { $result.NegativeFilterRehomeBlocked = $Matches[1]; continue }
                    '^negative_filter_skip_local=(.*)$' { $result.NegativeFilterSkipLocal = $Matches[1]; continue }
                    '^negative_filter_maybe_local=(.*)$' { $result.NegativeFilterMaybeLocal = $Matches[1]; continue }
                    '^negative_filter_shadow_false_skip=(.*)$' { $result.NegativeFilterShadowFalseSkip = $Matches[1]; continue }
                    '^negative_filter_shadow_local_valid=(.*)$' { $result.NegativeFilterShadowLocalValid = $Matches[1]; continue }
                    '^negative_filter_shadow_local_invalid=(.*)$' { $result.NegativeFilterShadowLocalInvalid = $Matches[1]; continue }
                    '^negative_filter_range_probe_total=(.*)$' { $result.NegativeFilterRangeProbeTotal = $Matches[1]; continue }
                    '^negative_filter_range_probe_max=(.*)$' { $result.NegativeFilterRangeProbeMax = $Matches[1]; continue }
                    '^shared_dir_lookup=(.*)$' { $result.SharedDirLookup = $Matches[1]; continue }
                    '^shared_dir_hit=(.*)$' { $result.SharedDirHit = $Matches[1]; continue }
                    '^shared_dir_miss=(.*)$' { $result.SharedDirMiss = $Matches[1]; continue }
                    '^shared_dir_stale=(.*)$' { $result.SharedDirStale = $Matches[1]; continue }
                    '^shared_dir_hit_local_allocator=(.*)$' { $result.SharedDirHitLocalAllocator = $Matches[1]; continue }
                    '^shared_dir_hit_foreign_allocator=(.*)$' { $result.SharedDirHitForeignAllocator = $Matches[1]; continue }
                    '^shared_dir_would_skip_local=(.*)$' { $result.SharedDirWouldSkipLocal = $Matches[1]; continue }
                    '^shared_dir_register=(.*)$' { $result.SharedDirRegister = $Matches[1]; continue }
                    '^shared_dir_unregister=(.*)$' { $result.SharedDirUnregister = $Matches[1]; continue }
                    '^shared_dir_probe_total=(.*)$' { $result.SharedDirProbeTotal = $Matches[1]; continue }
                    '^shared_dir_probe_max=(.*)$' { $result.SharedDirProbeMax = $Matches[1]; continue }
                    '^shared_dir_first_attempt=(.*)$' { $result.SharedDirFirstAttempt = $Matches[1]; continue }
                    '^shared_dir_first_hit=(.*)$' { $result.SharedDirFirstHit = $Matches[1]; continue }
                    '^shared_dir_first_fallback=(.*)$' { $result.SharedDirFirstFallback = $Matches[1]; continue }
                    '^shared_dir_first_invalid=(.*)$' { $result.SharedDirFirstInvalid = $Matches[1]; continue }
                    '^source_owned_prepare=(.*)$' { $result.SourceOwnedPrepare = $Matches[1]; continue }
                    '^source_owned_route_hit_local_owner=(.*)$' { $result.SourceOwnedRouteHitLocalOwner = $Matches[1]; continue }
                    '^source_owned_visibility_hit_local_owner=(.*)$' { $result.SourceOwnedVisibilityHitLocalOwner = $Matches[1]; continue }
                    '^source_owned_visibility_hit_foreign_owner=(.*)$' { $result.SourceOwnedVisibilityHitForeignOwner = $Matches[1]; continue }
                    '^source_owned_remote_free_attempt=(.*)$' { $result.SourceOwnedRemoteFreeAttempt = $Matches[1]; continue }
                    '^source_owned_release=(.*)$' { $result.SourceOwnedRelease = $Matches[1]; continue }
                    '^frontcache_reuse_hit=(.*)$' { $result.FrontcacheReuseHit = $Matches[1]; continue }
                    '^frontcache_reuse_invalid=(.*)$' { $result.FrontcacheReuseInvalid = $Matches[1]; continue }
                    '^transfer_reuse_hit=(.*)$' { $result.TransferReuseHit = $Matches[1]; continue }
                    '^transfer_reuse_invalid=(.*)$' { $result.TransferReuseInvalid = $Matches[1]; continue }
                    '^source_refill_starvation=(.*)$' { $result.SourceRefillStarvation = $Matches[1]; continue }
                    '^source_refill_saturation=(.*)$' { $result.SourceRefillSaturation = $Matches[1]; continue }
                    '^source_refill_boost=(.*)$' { $result.SourceRefillBoost = $Matches[1]; continue }
                    '^source_refill_clamp=(.*)$' { $result.SourceRefillClamp = $Matches[1]; continue }
                    '^source_admission_open=(.*)$' { $result.SourceAdmissionOpen = $Matches[1]; continue }
                    '^source_admission_boosted=(.*)$' { $result.SourceAdmissionBoosted = $Matches[1]; continue }
                    '^source_admission_clamped=(.*)$' { $result.SourceAdmissionClamped = $Matches[1]; continue }
                    '^source_prefill_attempt=(.*)$' { $result.SourcePrefillAttempt = $Matches[1]; continue }
                    '^source_prefill_filled=(.*)$' { $result.SourcePrefillFilled = $Matches[1]; continue }
                    '^source_prefill_fallback=(.*)$' { $result.SourcePrefillFallback = $Matches[1]; continue }
                    '^front_source_ops_alloc=(.*)$' { $result.FrontSourceOpsAlloc = $Matches[1]; continue }
                    '^front_source_slot_alloc=(.*)$' { $result.FrontSourceSlotAlloc = $Matches[1]; continue }
                    '^front_source_prefill_alloc=(.*)$' { $result.FrontSourcePrefillAlloc = $Matches[1]; continue }
                    '^toy_source_prefill_call=(.*)$' { $result.ToySourcePrefillCall = $Matches[1]; continue }
                    '^local2p_source_alloc=(.*)$' { $result.Local2pSourceAlloc = $Matches[1]; continue }
                    '^midpage_source_alloc=(.*)$' { $result.MidpageSourceAlloc = $Matches[1]; continue }
                    '^large_source_alloc=(.*)$' { $result.LargeSourceAlloc = $Matches[1]; continue }
                    '^toy_source_alloc=(.*)$' { $result.ToySourceAlloc = $Matches[1]; continue }
                    '^alloc_fail=(.*)$' { $result.AllocFail = $Matches[1]; continue }
                    '^descriptor_probe_total=(.*)$' { $result.DescriptorProbeTotal = $Matches[1]; continue }
                    '^descriptor_probe_max=(.*)$' { $result.DescriptorProbeMax = $Matches[1]; continue }
                    '^route_register_probe_total=(.*)$' { $result.RouteRegisterProbeTotal = $Matches[1]; continue }
                    '^route_register_probe_max=(.*)$' { $result.RouteRegisterProbeMax = $Matches[1]; continue }
                    '^route_unregister_probe_total=(.*)$' { $result.RouteUnregisterProbeTotal = $Matches[1]; continue }
                    '^route_unregister_probe_max=(.*)$' { $result.RouteUnregisterProbeMax = $Matches[1]; continue }
                    '^source_block_probe_total=(.*)$' { $result.SourceBlockProbeTotal = $Matches[1]; continue }
                    '^source_block_probe_max=(.*)$' { $result.SourceBlockProbeMax = $Matches[1]; continue }
                }
            }
            continue
        }

        if ($line.StartsWith("[HZ6_PATH]")) {
            switch -Regex ($line) {
                '^\[HZ6_PATH\] front=local2p (.*)$' { $result.FrontPathLocal2p = $Matches[1]; continue }
                '^\[HZ6_PATH\] front=midpage (.*)$' { $result.FrontPathMidpage = $Matches[1]; continue }
                '^\[HZ6_PATH\] front=large (.*)$' { $result.FrontPathLarge = $Matches[1]; continue }
                '^\[HZ6_PATH\] front=toy (.*)$' { $result.FrontPathToy = $Matches[1]; continue }
            }
            continue
        }

        if ($line.StartsWith("[HZ6_PREFILL]")) {
            switch -Regex ($line) {
                '^\[HZ6_PREFILL\] front=local2p attempt=(\d+) filled=(\d+) fallback=(\d+)$' {
                    $result.FrontPrefillAttemptLocal2p = $Matches[1]
                    $result.FrontPrefillFilledLocal2p = $Matches[2]
                    $result.FrontPrefillFallbackLocal2p = $Matches[3]
                    continue
                }
                '^\[HZ6_PREFILL\] front=midpage attempt=(\d+) filled=(\d+) fallback=(\d+)$' {
                    $result.FrontPrefillAttemptMidpage = $Matches[1]
                    $result.FrontPrefillFilledMidpage = $Matches[2]
                    $result.FrontPrefillFallbackMidpage = $Matches[3]
                    continue
                }
                '^\[HZ6_PREFILL\] front=large attempt=(\d+) filled=(\d+) fallback=(\d+)$' {
                    $result.FrontPrefillAttemptLarge = $Matches[1]
                    $result.FrontPrefillFilledLarge = $Matches[2]
                    $result.FrontPrefillFallbackLarge = $Matches[3]
                    continue
                }
                '^\[HZ6_PREFILL\] front=toy attempt=(\d+) filled=(\d+) fallback=(\d+)$' {
                    $result.FrontPrefillAttemptToy = $Matches[1]
                    $result.FrontPrefillFilledToy = $Matches[2]
                    $result.FrontPrefillFallbackToy = $Matches[3]
                    continue
                }
            }
        }

        if ($line.StartsWith("[HZ6_FRONTCACHE_CLASS]")) {
            if ($line -match '^\[HZ6_FRONTCACHE_CLASS\] class=(\d+) push=(\d+) pop_empty=(\d+)$') {
                $entry = ('c{0}:push={1},empty={2}' -f $Matches[1], $Matches[2], $Matches[3])
                if ($result.FrontcacheClass -eq "NA") {
                    $result.FrontcacheClass = $entry
                } else {
                    $result.FrontcacheClass = $result.FrontcacheClass + "; " + $entry
                }
            }
            continue
        }
    }

    return $result
}

if (-not $ThreadCounts -or $ThreadCounts.Count -eq 0) {
    $ThreadCounts = @(1, 4, 8, 16)
}

$RuntimeSec = 10
$MinSize = 8
$MaxSize = 1024
$ChunksPerThread = 10000
$Rounds = 1
$Seed = 12345

$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_paper_larson_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_paper_larson_windows.log")

$Summary = New-Object System.Collections.Generic.List[string]
$RawLines = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Paper-Aligned Windows Larson")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("References:")
$Summary.Add("- [private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/paper/RESULTS_20260118.md#L259)")
$Summary.Add("- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/larson_summary.csv)")
$Summary.Add("")
$Summary.Add("Windows native note:")
$Summary.Add('- benchmark: `bench_larson_compare`')
$Summary.Add('- params: `runtime=10s min=8 max=1024 chunks=10000 rounds=1 seed=12345`')
$Summary.Add(('- compact control (optional): `chunks={0}`' -f $CompactChunksPerThread))
$Summary.Add('- worker-warmup control (optional): same chunks, but each worker owns its warmup allocations before the timer starts')
$Summary.Add('- shared route visibility diagnostics: `route_visibility_lookup / hit / miss / probe_total / probe_max`')
$Summary.Add('- shared route owner diagnostics: `route_visibility_hit_local_owner / route_visibility_hit_foreign_owner`')
$Summary.Add('- transfer backlog diagnostics: `transfer_current / transfer_current_max`')
$Summary.Add('- remote free diagnostics: `remote_free_attempt / strict_owner_block / transfer_fail / route_rehome_attempt / route_rehome_success / route_rehome_fail`')
$Summary.Add('- lifecycle diagnostics: `lifecycle_owner_mismatch / foreign_free_attempt / foreign_free_handled / foreign_free_invalid`')
$Summary.Add('- visible-first diagnostics: `visible_first_attempt / hit / miss / visible_invalid / local_fallback / fallback_invalid / local_lookup_skipped`')
$Summary.Add('- diagnostic-only frontcache class lines: `[HZ6_FRONTCACHE_CLASS] class=<id> push=<n> pop_empty=<n>`')
$Summary.Add('- thread sweep: `1, 4, 8, 16`')
$Summary.Add(('- runs: `{0}`' -f $Runs))
$Summary.Add(('- timeout: `{0}s` per allocator row' -f $TimeoutSeconds))
$Summary.Add('- statistic: `median alloc/s` from the benchmark''s `Throughput = ...` line')
$Summary.Add('- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`')
$Summary.Add("")

. (Join-Path $PSScriptRoot "run_win_larson_paper_tail.ps1")
