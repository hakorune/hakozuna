function Invoke-LarsonSweep {
    param(
        [string]$SectionTitle,
        [int]$ChunksPerThreadValue,
        [int]$WarmupMode = 0
    )

foreach ($threads in $ThreadCounts) {
        $Summary.Add("## " + $SectionTitle + " T=" + $threads)
        $Summary.Add("")
        $tableHeader = "| allocator | median ops/s | median peak_kb | route_miss | route_vis_lookup | route_vis_hit | route_vis_hit_local_owner | route_vis_hit_foreign_owner | route_vis_miss | route_vis_probe_total | route_vis_probe_max | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | transfer_current | transfer_current_max | remote_free_attempt | remote_free_strict_block | remote_free_transfer_fail | route_rehome_attempt | route_rehome_success | route_rehome_fail | lifecycle_owner_mismatch | foreign_free_attempt | foreign_free_handled | foreign_free_invalid | visible_first_attempt | visible_first_hit | visible_first_miss | visible_first_visible_invalid | visible_first_local_fallback | visible_first_fallback_invalid | visible_first_local_lookup_skipped | negative_filter_attempt | negative_filter_not_armed | negative_filter_rehome_blocked | negative_filter_skip_local | negative_filter_maybe_local | negative_filter_false_skip | negative_filter_shadow_local_valid | negative_filter_shadow_local_invalid | negative_filter_range_probe_total | negative_filter_range_probe_max | shared_dir_lookup | shared_dir_hit | shared_dir_miss | shared_dir_stale | shared_dir_hit_local_allocator | shared_dir_hit_foreign_allocator | shared_dir_would_skip_local | shared_dir_register | shared_dir_unregister | shared_dir_probe_total | shared_dir_probe_max | shared_dir_first_attempt | shared_dir_first_hit | shared_dir_first_fallback | shared_dir_first_invalid | source_owned_prepare | source_owned_route_hit_local_owner | source_owned_visibility_hit_local_owner | source_owned_visibility_hit_foreign_owner | source_owned_remote_free_attempt | source_owned_release | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |"
        $Summary.Add($tableHeader)
        $metricColumnCount = (($tableHeader -split '\|' | Where-Object { $_.Trim() -ne "" }).Count - 4)
        $tableSeparators = ($tableHeader -split '\|' | Where-Object { $_.Trim() -ne "" } | ForEach-Object {
            if ($_.Trim() -eq "allocator" -or $_.Trim() -eq "runs") { "---" } else { "---:" }
        })
        $Summary.Add("| " + ($tableSeparators -join " | ") + " |")

        foreach ($exe in $Executables) {
            $opsRuns = New-Object System.Collections.Generic.List[double]
            $peakRuns = New-Object System.Collections.Generic.List[double]
            $runTexts = New-Object System.Collections.Generic.List[string]
            $lastStats = @{
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
                RouteRegisterProbeTotal = "NA"
                RouteUnregisterProbeTotal = "NA"
                SourceBlockProbeTotal = "NA"
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

            for ($run = 1; $run -le $Runs; $run++) {
                $args = @(
                    [string]$RuntimeSec,
                    [string]$MinSize,
                    [string]$MaxSize,
                    [string]$ChunksPerThreadValue,
                    [string]$Rounds,
                    [string]$Seed,
                    [string]$threads,
                    [string]$WarmupMode
                )
                $result = Invoke-CapturedProcess -FilePath $exe.Path -Arguments $args -TimeoutSeconds $TimeoutSeconds
                $output = $result.Lines
                $rc = $result.ExitCode

                $RawLines.Add("=== " + $SectionTitle + " / T=" + $threads + " / " + $exe.Name + " / run " + $run + " ===")
                $RawLines.Add("cmd: " + $exe.Path + " " + ($args -join " "))
                $RawLines.Add("rc: " + $rc)
                foreach ($line in $output) {
                    $RawLines.Add($line.ToString())
                }
                $RawLines.Add("")

                if ($rc -ne 0) {
                    if ($ContinueOnFailure) {
                        $runTexts.Add(("failed:rc{0}" -f $rc))
                        continue
                    }
                    throw "Larson runner allocator $($exe.Name) failed at T=$threads with exit code $rc"
                }

                $raw = (($output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
                if ($raw -notmatch "Throughput = ([0-9.]+) operations per second") {
                    if ($ContinueOnFailure) {
                        $runTexts.Add("failed:parse")
                        continue
                    }
                    throw "Could not parse throughput for allocator $($exe.Name) at T=$threads"
                }
                $ops = [double]$Matches[1]
                $opsRuns.Add($ops)
                if ($result.PeakKb -match '^[0-9]+$') {
                    $peakRuns.Add([double]$result.PeakKb)
                }
                $runTexts.Add(("{0:N3}M / {1} KB" -f ($ops / 1000000.0), $result.PeakKb))
                $parsedStats = Parse-Hz6Stats -Lines $output
                if ($exe.Name -like "hz6-*") {
                    $lastStats = $parsedStats
                }
            }

            if ($opsRuns.Count -eq 0) {
                $failedMetrics = ((@('NA') * $metricColumnCount) -join ' | ')
                $Summary.Add(('| {0} | failed | n/a | {1} | `{2}` |' -f $exe.Name, $failedMetrics, ($runTexts -join ", ")))
                continue
            }

            $medianOps = Get-Median -Values $opsRuns.ToArray()
            $medianPeak = if ($peakRuns.Count -gt 0) { Get-Median -Values $peakRuns.ToArray() } else { [double]::NaN }
            $metricCells = @(
                $lastStats.RouteMiss,
                $lastStats.RouteVisibilityLookup,
                $lastStats.RouteVisibilityHit,
                $lastStats.RouteVisibilityHitLocalOwner,
                $lastStats.RouteVisibilityHitForeignOwner,
                $lastStats.RouteVisibilityMiss,
                $lastStats.RouteVisibilityProbeTotal,
                $lastStats.RouteVisibilityProbeMax,
                $lastStats.SourceAlloc,
                $lastStats.Local2pSourceAlloc,
                $lastStats.MidpageSourceAlloc,
                $lastStats.LargeSourceAlloc,
                $lastStats.ToySourceAlloc,
                $lastStats.FrontSourceOpsAlloc,
                $lastStats.FrontSourceSlotAlloc,
                $lastStats.FrontSourcePrefillAlloc,
                $lastStats.ToySourcePrefillCall,
                $lastStats.FrontPathLocal2p,
                $lastStats.FrontPathMidpage,
                $lastStats.FrontPathLarge,
                $lastStats.FrontPathToy,
                $lastStats.TransferPush,
                $lastStats.TransferPop,
                $lastStats.TransferCurrent,
                $lastStats.TransferCurrentMax,
                $lastStats.RemoteFreeAttempt,
                $lastStats.RemoteFreeStrictOwnerBlock,
                $lastStats.RemoteFreeTransferFail,
                $lastStats.RouteRehomeAttempt,
                $lastStats.RouteRehomeSuccess,
                $lastStats.RouteRehomeFail,
                $lastStats.LifecycleOwnerMismatch,
                $lastStats.LifecycleForeignFreeAttempt,
                $lastStats.LifecycleForeignFreeHandled,
                $lastStats.LifecycleForeignFreeInvalid,
                $lastStats.VisibleFirstAttempt,
                $lastStats.VisibleFirstHit,
                $lastStats.VisibleFirstMiss,
                $lastStats.VisibleFirstVisibleInvalid,
                $lastStats.VisibleFirstLocalFallback,
                $lastStats.VisibleFirstLocalFallbackInvalid,
                $lastStats.VisibleFirstLocalLookupSkipped,
                $lastStats.NegativeFilterAttempt,
                $lastStats.NegativeFilterNotArmed,
                $lastStats.NegativeFilterRehomeBlocked,
                $lastStats.NegativeFilterSkipLocal,
                $lastStats.NegativeFilterMaybeLocal,
                $lastStats.NegativeFilterShadowFalseSkip,
                $lastStats.NegativeFilterShadowLocalValid,
                $lastStats.NegativeFilterShadowLocalInvalid,
                $lastStats.NegativeFilterRangeProbeTotal,
                $lastStats.NegativeFilterRangeProbeMax,
                $lastStats.SharedDirLookup,
                $lastStats.SharedDirHit,
                $lastStats.SharedDirMiss,
                $lastStats.SharedDirStale,
                $lastStats.SharedDirHitLocalAllocator,
                $lastStats.SharedDirHitForeignAllocator,
                $lastStats.SharedDirWouldSkipLocal,
                $lastStats.SharedDirRegister,
                $lastStats.SharedDirUnregister,
                $lastStats.SharedDirProbeTotal,
                $lastStats.SharedDirProbeMax,
                $lastStats.SharedDirFirstAttempt,
                $lastStats.SharedDirFirstHit,
                $lastStats.SharedDirFirstFallback,
                $lastStats.SharedDirFirstInvalid,
                $lastStats.SourceOwnedPrepare,
                $lastStats.SourceOwnedRouteHitLocalOwner,
                $lastStats.SourceOwnedVisibilityHitLocalOwner,
                $lastStats.SourceOwnedVisibilityHitForeignOwner,
                $lastStats.SourceOwnedRemoteFreeAttempt,
                $lastStats.SourceOwnedRelease,
                $lastStats.FrontcacheReuseHit,
                $lastStats.FrontcacheReuseInvalid,
                $lastStats.TransferReuseHit,
                $lastStats.TransferReuseInvalid,
                $lastStats.SourceRefillStarvation,
                $lastStats.SourceRefillSaturation,
                $lastStats.SourceRefillBoost,
                $lastStats.SourceRefillClamp,
                $lastStats.SourceAdmissionOpen,
                $lastStats.SourceAdmissionBoosted,
                $lastStats.SourceAdmissionClamped,
                $lastStats.SourcePrefillAttempt,
                $lastStats.SourcePrefillFilled,
                $lastStats.SourcePrefillFallback,
                $lastStats.AllocFail,
                $lastStats.DescriptorProbeTotal,
                $lastStats.RouteRegisterProbeTotal,
                $lastStats.RouteUnregisterProbeTotal,
                $lastStats.SourceBlockProbeTotal
            )
            $rowCells = @(
                $exe.Name,
                ("{0:N3}M" -f ($medianOps / 1000000.0)),
                ("{0:N0}" -f $medianPeak)
            ) + $metricCells + @(($runTexts -join ", "))
            $Summary.Add(("| " + ($rowCells -join " | ") + " |"))

            if ($exe.Name -like "hz6-*") {
                $Summary.Add((
                    '  prefill: local2p a={0} f={1} fb={2}; midpage a={3} f={4} fb={5}; large a={6} f={7} fb={8}; toy a={9} f={10} fb={11}' -f
                    $lastStats.FrontPrefillAttemptLocal2p,
                    $lastStats.FrontPrefillFilledLocal2p,
                    $lastStats.FrontPrefillFallbackLocal2p,
                    $lastStats.FrontPrefillAttemptMidpage,
                    $lastStats.FrontPrefillFilledMidpage,
                    $lastStats.FrontPrefillFallbackMidpage,
                    $lastStats.FrontPrefillAttemptLarge,
                    $lastStats.FrontPrefillFilledLarge,
                    $lastStats.FrontPrefillFallbackLarge,
                    $lastStats.FrontPrefillAttemptToy,
                    $lastStats.FrontPrefillFilledToy,
                    $lastStats.FrontPrefillFallbackToy))
                $Summary.Add(('  visibility: lookup={0} hit={1} miss={2} probe_total={3} probe_max={4}' -f
                    $lastStats.RouteVisibilityLookup,
                    $lastStats.RouteVisibilityHit,
                    $lastStats.RouteVisibilityMiss,
                    $lastStats.RouteVisibilityProbeTotal,
                    $lastStats.RouteVisibilityProbeMax))
                $Summary.Add(('  visibility owner: local={0} foreign={1}' -f
                    $lastStats.RouteVisibilityHitLocalOwner,
                    $lastStats.RouteVisibilityHitForeignOwner))
                $Summary.Add(('  transfer: current={0} max={1}' -f
                    $lastStats.TransferCurrent,
                    $lastStats.TransferCurrentMax))
                $Summary.Add(('  remote_free: attempt={0} strict_block={1} transfer_fail={2} rehome_attempt={3} rehome_success={4} rehome_fail={5}' -f
                    $lastStats.RemoteFreeAttempt,
                    $lastStats.RemoteFreeStrictOwnerBlock,
                    $lastStats.RemoteFreeTransferFail,
                    $lastStats.RouteRehomeAttempt,
                    $lastStats.RouteRehomeSuccess,
                    $lastStats.RouteRehomeFail))
                $Summary.Add(('  lifecycle: owner_mismatch={0} foreign_attempt={1} foreign_handled={2} foreign_invalid={3}' -f
                    $lastStats.LifecycleOwnerMismatch,
                    $lastStats.LifecycleForeignFreeAttempt,
                    $lastStats.LifecycleForeignFreeHandled,
                    $lastStats.LifecycleForeignFreeInvalid))
                $Summary.Add(('  visible_first: attempt={0} hit={1} miss={2} visible_invalid={3} local_fallback={4} fallback_invalid={5} local_lookup_skipped={6}' -f
                    $lastStats.VisibleFirstAttempt,
                    $lastStats.VisibleFirstHit,
                    $lastStats.VisibleFirstMiss,
                    $lastStats.VisibleFirstVisibleInvalid,
                    $lastStats.VisibleFirstLocalFallback,
                    $lastStats.VisibleFirstLocalFallbackInvalid,
                    $lastStats.VisibleFirstLocalLookupSkipped))
                $Summary.Add(('  negative_filter: attempt={0} not_armed={1} rehome_blocked={2} skip_local={3} maybe_local={4} false_skip={5} shadow_local_valid={6} shadow_local_invalid={7} range_probe_total={8} range_probe_max={9}' -f
                    $lastStats.NegativeFilterAttempt,
                    $lastStats.NegativeFilterNotArmed,
                    $lastStats.NegativeFilterRehomeBlocked,
                    $lastStats.NegativeFilterSkipLocal,
                    $lastStats.NegativeFilterMaybeLocal,
                    $lastStats.NegativeFilterShadowFalseSkip,
                    $lastStats.NegativeFilterShadowLocalValid,
                    $lastStats.NegativeFilterShadowLocalInvalid,
                    $lastStats.NegativeFilterRangeProbeTotal,
                    $lastStats.NegativeFilterRangeProbeMax))
                $Summary.Add(('  shared_dir: lookup={0} hit={1} miss={2} stale={3} local_alloc={4} foreign_alloc={5} would_skip_local={6} register={7} unregister={8} probe_total={9} probe_max={10} first_attempt={11} first_hit={12} first_fallback={13} first_invalid={14}' -f
                    $lastStats.SharedDirLookup,
                    $lastStats.SharedDirHit,
                    $lastStats.SharedDirMiss,
                    $lastStats.SharedDirStale,
                    $lastStats.SharedDirHitLocalAllocator,
                    $lastStats.SharedDirHitForeignAllocator,
                    $lastStats.SharedDirWouldSkipLocal,
                    $lastStats.SharedDirRegister,
                    $lastStats.SharedDirUnregister,
                    $lastStats.SharedDirProbeTotal,
                    $lastStats.SharedDirProbeMax,
                    $lastStats.SharedDirFirstAttempt,
                    $lastStats.SharedDirFirstHit,
                    $lastStats.SharedDirFirstFallback,
                    $lastStats.SharedDirFirstInvalid))
                $Summary.Add(('  frontcache_class: {0}' -f $lastStats.FrontcacheClass))
            }
        }

        $Summary.Add("")
    }
}

Invoke-LarsonSweep -SectionTitle "Larson stress" -ChunksPerThreadValue $ChunksPerThread
if ($IncludeWorkerWarmupControl) {
    $Summary.Add("## Worker-warmup control note")
    $Summary.Add("")
    $Summary.Add("- Worker-warmup mode allocates the initial live set inside each worker thread after allocator thread setup and before the timer starts.")
    $Summary.Add("- This separates cross-owner warmup stress from same-owner small-object source placement.")
    $Summary.Add("")
    Invoke-LarsonSweep -SectionTitle "Larson worker-warmup stress" -ChunksPerThreadValue $ChunksPerThread -WarmupMode 1
}
if ($IncludeCompactControl) {
    $Summary.Add("## Compact control note")
    $Summary.Add("")
    $Summary.Add(("- HZ6-friendly compact control uses `chunks={0}` while keeping the same runtime/size/seed settings." -f $CompactChunksPerThread))
    $Summary.Add("")
    Invoke-LarsonSweep -SectionTitle "Larson compact control" -ChunksPerThreadValue $CompactChunksPerThread
}

$Summary.Add("Artifacts: [out_win_larson](/C:/git/hakozuna-win/out_win_larson)")

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Set-Content -Path $RawLogPath -Value $RawLines -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
