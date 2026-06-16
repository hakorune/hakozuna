foreach ($family in $selectedFamilies) {
    $familyExecutables = @($Executables | Where-Object { $_.Family -eq $family })
    $cases = @()
    if ($family -eq "mixed_ws") {
        $cases = Expand-ProfileSelection -AllProfiles $mixedProfiles -Requested $BenchmarkProfiles
    } elseif ($family -eq "random_mixed") {
        $cases = Expand-ProfileSelection -AllProfiles $randomProfiles -Requested $BenchmarkProfiles
    } elseif ($family -eq "larson") {
        $larsonProfiles = @()
        foreach ($threads in $ThreadCounts) {
            $larsonProfiles += @{ Name = "larson_T$threads"; Args = @("10", "8", "1024", "10000", "1", "12345", [string]$threads, "0"); Note = "Larson stress T=$threads, main-warmup, chunks=10000" }
        }
        $larsonProfiles += @(
            @{ Name = "larson_t16_main_10k"; Args = @("10", "8", "1024", "10000", "1", "12345", "16", "0"); Note = "Larson T=16 cross-owner main-warmup, chunks=10000" },
            @{ Name = "larson_t16_worker_10k"; Args = @("10", "8", "1024", "10000", "1", "12345", "16", "1"); Note = "Larson T=16 same-owner worker-warmup, chunks=10000" },
            @{ Name = "larson_t16_main_1k"; Args = @("1", "8", "1024", "1000", "1", "12345", "16", "0"); Note = "Larson T=16 cross-owner main-warmup, chunks=1000" },
            @{ Name = "larson_t16_worker_1k"; Args = @("1", "8", "1024", "1000", "1", "12345", "16", "1"); Note = "Larson T=16 same-owner worker-warmup, chunks=1000" },
            @{ Name = "larson_t16_main_4k"; Args = @("1", "8", "1024", "4000", "1", "12345", "16", "0"); Note = "Larson T=16 cross-owner main-warmup, chunks=4000" },
            @{ Name = "larson_t16_worker_4k"; Args = @("1", "8", "1024", "4000", "1", "12345", "16", "1"); Note = "Larson T=16 same-owner worker-warmup, chunks=4000" }
        )
        $cases = Expand-ProfileSelection -AllProfiles $larsonProfiles -Requested $BenchmarkProfiles
    } elseif ($family -eq "redis") {
        $redisProfiles = @(
            @{ Name = "redis_workload"; Args = @("4", "500", "2000", "16", "256"); Note = "paper-aligned Redis-like workload" },
            @{ Name = "redis_short"; Args = @("2", "100", "200", "16", "256"); Note = "short Redis-like timeout triage row" },
            @{ Name = "redis_medium"; Args = @("2", "200", "500", "16", "256"); Note = "medium Redis-like staged pressure row" },
            @{ Name = "redis_long"; Args = @("4", "300", "1000", "16", "256"); Note = "long Redis-like staged pressure row below paper default" },
            @{ Name = "redis_tiny"; Args = @("1", "50", "100", "16", "256"); Note = "tiny Redis-like smoke row" }
        )
        $requestedRedisProfiles = Split-List -Values $BenchmarkProfiles
        if (-not $requestedRedisProfiles -or $requestedRedisProfiles.Count -eq 0) {
            $cases += ($redisProfiles | Where-Object { $_.Name -eq "redis_workload" })
        } else {
            $cases = Expand-ProfileSelection -AllProfiles $redisProfiles -Requested $BenchmarkProfiles
        }
    }

    foreach ($case in $cases) {
        $residualRows = New-Object System.Collections.Generic.List[object]
        $capacityRows = New-Object System.Collections.Generic.List[object]
        $warmupRows = New-Object System.Collections.Generic.List[object]
        $elasticRows = New-Object System.Collections.Generic.List[object]
        $overflowRows = New-Object System.Collections.Generic.List[object]
        $Summary.Add("## $family / $($case.Name)")
        $Summary.Add("")
        $Summary.Add("- Note: " + $case.Note)
        $Summary.Add(('- Args: `{0}`' -f ($case.Args -join " ")))
        $Summary.Add(('- Runs: `{0}`' -f $Runs))
        $Summary.Add("")
        if ($family -eq "redis") {
            $Summary.Add("| allocator | pattern | median ops | median peak_kb | runs |")
            $Summary.Add("| --- | --- | ---: | ---: | --- |")
        } else {
            $Summary.Add("| allocator | median ops/s | median peak_kb | runs |")
            $Summary.Add("| --- | ---: | ---: | --- |")
        }

        foreach ($exe in $familyExecutables) {
            $opsRuns = New-Object System.Collections.Generic.List[double]
            $peakRuns = New-Object System.Collections.Generic.List[double]
            $runTexts = New-Object System.Collections.Generic.List[string]
            $residualMaps = New-Object System.Collections.Generic.List[object]
            $capacityMaps = New-Object System.Collections.Generic.List[object]
            $warmupMaps = New-Object System.Collections.Generic.List[object]
            $elasticMaps = New-Object System.Collections.Generic.List[object]
            $overflowMaps = New-Object System.Collections.Generic.List[object]
            $redisPatternRuns = @{}
            foreach ($pattern in @("SET", "GET", "LPUSH", "LPOP", "RANDOM")) {
                $redisPatternRuns[$pattern] = New-Object System.Collections.Generic.List[double]
            }

            for ($run = 1; $run -le $Runs; $run++) {
                $result = Invoke-CapturedProcess -FilePath $exe.Path -Arguments $case.Args -TimeoutSeconds $TimeoutSeconds
                $output = $result.Lines
                $raw = (($output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()

                $RawLogWriter.WriteLine("=== $family / $($case.Name) / $($exe.Name) / run $run ===")
                $RawLogWriter.WriteLine("cmd: $($exe.Path) " + ($case.Args -join " "))
                $RawLogWriter.WriteLine("rc: " + $result.ExitCode)
                foreach ($line in $output) {
                    $RawLogWriter.WriteLine($line.ToString())
                }
                $RawLogWriter.WriteLine("")

                if ($result.ExitCode -ne 0) {
                    if ($ContinueOnFailure) {
                        $runTexts.Add(("failed:rc{0}" -f $result.ExitCode))
                        continue
                    }
                    throw "$family runner allocator $($exe.Name) failed with exit code $($result.ExitCode)"
                }

                if ($family -eq "larson") {
                    if ($raw -match "Throughput = ([0-9.]+) operations per second") {
                        $opsRuns.Add([double]$Matches[1])
                    } elseif ($ContinueOnFailure) {
                        $runTexts.Add("failed:parse")
                        continue
                    } else {
                        throw "Could not parse Larson throughput for $($exe.Name)"
                    }
                } elseif ($family -eq "redis") {
                    $currentPattern = $null
                    foreach ($line in $output) {
                        $text = $line.ToString().Trim()
                        if ($text -match "^Pattern:\s+(\S+)$") {
                            $currentPattern = $Matches[1]
                            continue
                        }
                        if ($text -match "^Throughput:\s+([0-9.]+)\s+M ops/sec$") {
                            if ($redisPatternRuns.ContainsKey($currentPattern)) {
                                $redisPatternRuns[$currentPattern].Add([double]$Matches[1])
                            }
                        }
                    }
                } else {
                    if ($raw -match "ops/s=([0-9.]+)") {
                        $opsRuns.Add([double]$Matches[1])
                    } elseif ($ContinueOnFailure) {
                        $runTexts.Add("failed:parse")
                        continue
                    } else {
                        throw "Could not parse ops/s for $family $($exe.Name)"
                    }
                }

                if ($result.PeakKb -match '^[0-9]+$') {
                    $peakRuns.Add([double]$result.PeakKb)
                }
                if ($DiagnosticHz6Probes -and $raw -match '\[HZ6_RSS_RESIDUAL\]\s+([^[]+)') {
                    $residualMaps.Add((Get-KeyValueMap -Line $Matches[1]))
                }
                if ($DiagnosticHz6Probes -and $raw -match '\[HZ6_CAPACITY_UTIL\]\s+([^[]+)') {
                    $capacityMaps.Add((Get-KeyValueMap -Line $Matches[1]))
                }
                if ($DiagnosticHz6Probes -and $raw -match '\[HZ6_MAIN_WARMUP_CAPACITY\]\s+([^[]+)') {
                    $warmupMaps.Add((Get-KeyValueMap -Line $Matches[1]))
                }
                if ($DiagnosticHz6Probes -and $raw -match '\[HZ6_ELASTIC_PROJECTION\]\s+([^[]+)') {
                    $elasticMaps.Add((Get-KeyValueMap -Line $Matches[1]))
                }
                if ($DiagnosticHz6Probes -and $raw -match '\[HZ6_ELASTIC_OVERFLOW_PROJECTION\]\s+([^[]+)') {
                    $overflowMaps.Add((Get-KeyValueMap -Line $Matches[1]))
                }
                if ($family -eq "redis") {
                    $runTexts.Add(("run{0}:ok" -f $run))
                } else {
                    $runTexts.Add($raw)
                }
            }

            $medianPeak = if ($peakRuns.Count -gt 0) { "{0:N0}" -f (Get-Median -Values $peakRuns.ToArray()) } else { "n/a" }
            if ($family -eq "redis") {
                foreach ($pattern in @("SET", "GET", "LPUSH", "LPOP", "RANDOM")) {
                    $values = $redisPatternRuns[$pattern]
                    $medianOps = if ($values.Count -gt 0) { "{0:N2}" -f (Get-Median -Values $values.ToArray()) } else { "failed" }
                    $Summary.Add(('| {0} | {1} | {2} | {3} | `{4}` |' -f $exe.Name, $pattern, $medianOps, $medianPeak, ($runTexts -join " ; ")))
                }
            } else {
                $medianOps = if ($opsRuns.Count -gt 0) { "{0:N3}M" -f ((Get-Median -Values $opsRuns.ToArray()) / 1000000.0) } else { "failed" }
                $Summary.Add(('| {0} | {1} | {2} | `{3}` |' -f $exe.Name, $medianOps, $medianPeak, ($runTexts -join " ; ")))
            }

            if ($DiagnosticHz6Probes -and $residualMaps.Count -gt 0) {
                $residualRows.Add([pscustomobject]@{
                    Allocator = $exe.Name
                    StaticTable = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "static_table_bytes"
                    StaticPlusPayload = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "static_plus_payload_bytes"
                    Descriptor = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "descriptor_table_bytes"
                    Route = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "route_table_bytes"
                    SharedDir = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "shared_route_directory_bytes"
                    OwnerIndex = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "owner_locality_index_bytes"
                    SourceBlock = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "source_block_table_bytes"
                    Frontcache = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "frontcache_table_bytes"
                    Transfer = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "transfer_table_bytes"
                    Payload = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "source_block_committed_estimate"
                    ActiveSourceBlocks = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "active_source_blocks"
                    RouteActive = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "route_active_current"
                    FrontcacheTotal = Get-MedianFromMaps -Maps $residualMaps.ToArray() -Key "frontcache_total"
                })
            }
            if ($DiagnosticHz6Probes -and $capacityMaps.Count -gt 0) {
                $capacityRows.Add([pscustomobject]@{
                    Allocator = $exe.Name
                    AllocatorCount = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "allocator_count"
                    DescriptorCapacity = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "descriptor_capacity"
                    DescriptorUsed = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "descriptor_used"
                    DescriptorUnused = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "descriptor_unused"
                    RouteCapacity = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_capacity"
                    RouteUsed = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_used"
                    RouteOccupied = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_occupied"
                    RouteUnused = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_unused"
                    SourceBlockCapacity = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "source_block_capacity"
                    SourceBlockUsed = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "source_block_used"
                    SourceBlockUnused = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "source_block_unused"
                    FrontcacheCapacity = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "frontcache_capacity"
                    FrontcacheUsed = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "frontcache_used"
                    FrontcacheUnused = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "frontcache_unused"
                    TransferCapacity = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "transfer_capacity"
                    TransferUsed = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "transfer_used"
                    TransferUnused = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "transfer_unused"
                    DescriptorLiveMax = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "descriptor_live_max"
                    SourceBlockActiveMax = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "source_block_active_max"
                    FrontcacheTotalMax = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "frontcache_total_max"
                    DescriptorMaxAllocator = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "descriptor_used_max_allocator"
                    DescriptorLocalCap2x = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "descriptor_local_cap_2x"
                    RouteMaxAllocator = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_used_max_allocator"
                    RouteOccupiedMaxAllocator = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_occupied_max_allocator"
                    RouteLocalCap2x = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "route_local_cap_2x"
                    SourceBlockMaxAllocator = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "source_block_used_max_allocator"
                    SourceBlockLocalCap2x = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "source_block_local_cap_2x"
                    FrontcacheMaxAllocator = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "frontcache_used_max_allocator"
                    FrontcacheLocalCap2x = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "frontcache_local_cap_2x"
                    TransferMaxAllocator = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "transfer_used_max_allocator"
                    TransferLocalCap2x = Get-MedianFromMaps -Maps $capacityMaps.ToArray() -Key "transfer_local_cap_2x"
                })
            }
            if ($DiagnosticHz6Probes -and $warmupMaps.Count -gt 0) {
                $warmupRows.Add([pscustomobject]@{
                    Allocator = $exe.Name
                    DescriptorUsed = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "descriptor_used"
                    DescriptorLiveMax = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "descriptor_live_max"
                    RouteUsed = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "route_used"
                    RouteOccupied = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "route_occupied"
                    RouteActiveMax = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "route_active_max"
                    SourceBlockUsed = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "source_block_used"
                    SourceBlockActiveMax = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "source_block_active_max"
                    FrontcacheUsed = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "frontcache_used"
                    FrontcacheTotalMax = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "frontcache_total_max"
                    TransferCurrent = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "transfer_current"
                    TransferCurrentMax = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "transfer_current_max"
                    DescriptorExhausted = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "descriptor_exhausted"
                    ElasticDepotRunMetaInit = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_init"
                    ElasticDepotRunMetaMark = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_mark"
                    ElasticDepotRunMetaClear = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_clear"
                    ElasticDepotRunMetaClassMismatch = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_class_mismatch"
                    ElasticDepotRunMetaSlotMisaligned = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_slot_misaligned"
                    ElasticDepotRunMetaTooManySlots = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_too_many_slots"
                    ElasticDepotRunMetaUsedCountMismatch = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "elastic_depot_run_meta_used_count_mismatch"
                    RouteRegisterFail = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "route_register_fail"
                    SourceBlockExhausted = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "source_block_exhausted"
                    AllocFail = Get-MedianFromMaps -Maps $warmupMaps.ToArray() -Key "alloc_fail"
                })
            }
            if ($DiagnosticHz6Probes -and $elasticMaps.Count -gt 0) {
                $elasticRows.Add([pscustomobject]@{
                    Allocator = $exe.Name
                    AllocatorCount = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "allocator_count"
                    DescriptorCapacity = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "descriptor_projected_capacity"
                    DescriptorBytes = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "descriptor_projected_table_bytes"
                    RouteCapacity = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "route_projected_capacity"
                    RouteBytes = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "route_projected_table_bytes"
                    SourceBlockCapacity = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "source_block_projected_capacity"
                    SourceBlockBytes = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "source_block_projected_table_bytes"
                    FrontcacheCapacity = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "frontcache_projected_capacity"
                    FrontcacheBytes = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "frontcache_projected_table_bytes"
                    TransferCapacity = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "transfer_projected_capacity"
                    TransferBytes = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "transfer_projected_table_bytes"
                    ProjectedStatic = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "projected_static_table_bytes"
                    ProjectedStaticPlusPayload = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "projected_static_plus_payload_bytes"
                    StaticSavings = Get-MedianFromMaps -Maps $elasticMaps.ToArray() -Key "projected_static_savings_bytes"
                })
            }
            if ($DiagnosticHz6Probes -and $overflowMaps.Count -gt 0) {
                $overflowRows.Add([pscustomobject]@{
                    Allocator = $exe.Name
                    DescriptorLocalCap = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "descriptor_local_cap"
                    DescriptorOverflow = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "descriptor_overflow"
                    DescriptorOverflowBytes = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "descriptor_overflow_bytes"
                    RouteLocalCap = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "route_local_cap"
                    RouteOverflow = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "route_overflow"
                    RouteOverflowBytes = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "route_overflow_bytes"
                    SourceBlockLocalCap = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "source_block_local_cap"
                    SourceBlockOverflow = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "source_block_overflow"
                    SourceBlockOverflowBytes = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "source_block_overflow_bytes"
                    FrontcacheLocalCap = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "frontcache_local_cap"
                    FrontcacheOverflow = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "frontcache_overflow"
                    FrontcacheOverflowBytes = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "frontcache_overflow_bytes"
                    TransferLocalCap = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "transfer_local_cap"
                    TransferOverflow = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "transfer_overflow"
                    TransferOverflowBytes = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "transfer_overflow_bytes"
                    OverflowTotalBytes = Get-MedianFromMaps -Maps $overflowMaps.ToArray() -Key "overflow_total_bytes"
                })
            }
        }
        if ($DiagnosticHz6Probes -and $residualRows.Count -gt 0) {
            $Summary.Add("")
            $Summary.Add("### HZ6 RSS residual audit")
            $Summary.Add("")
            $Summary.Add("| allocator | static KiB | static+payload KiB | descriptor KiB | route KiB | shared dir KiB | owner index KiB | source block KiB | frontcache KiB | transfer KiB | payload KiB | active source blocks | route active | frontcache total |")
            $Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
            foreach ($row in $residualRows) {
                $fmtKb = {
                    param($value)
                    if ($null -eq $value) { return "n/a" }
                    return "{0:N0}" -f ([double]$value / 1024.0)
                }
                $fmtCount = {
                    param($value)
                    if ($null -eq $value) { return "n/a" }
                    return "{0:N0}" -f ([double]$value)
                }
                $Summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} | {10} | {11} | {12} | {13} |' -f `
                    $row.Allocator,
                    (& $fmtKb $row.StaticTable),
                    (& $fmtKb $row.StaticPlusPayload),
                    (& $fmtKb $row.Descriptor),
                    (& $fmtKb $row.Route),
                    (& $fmtKb $row.SharedDir),
                    (& $fmtKb $row.OwnerIndex),
                    (& $fmtKb $row.SourceBlock),
                    (& $fmtKb $row.Frontcache),
                    (& $fmtKb $row.Transfer),
                    (& $fmtKb $row.Payload),
                    (& $fmtCount $row.ActiveSourceBlocks),
                    (& $fmtCount $row.RouteActive),
                    (& $fmtCount $row.FrontcacheTotal)))
            }
        }
        if ($DiagnosticHz6Probes -and $capacityRows.Count -gt 0) {
            $Summary.Add("")
            $Summary.Add("### HZ6 capacity utilization audit")
            $Summary.Add("")
            $Summary.Add("| allocator | allocators | descriptor used/cap | descriptor util | descriptor peak | descriptor max/cap2x | route active/cap | route util | route max/cap2x | source blocks used/cap | source util | source peak | source max/cap2x | frontcache used/cap | frontcache util | frontcache peak | frontcache max/cap2x | transfer current/cap | transfer util | transfer max/cap2x |")
            $Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
            $fmtCount = {
                param($value)
                if ($null -eq $value) { return "n/a" }
                return "{0:N0}" -f ([double]$value)
            }
            $fmtPair = {
                param($used, $capacity)
                if ($null -eq $used -or $null -eq $capacity) { return "n/a" }
                return ("{0:N0}/{1:N0}" -f ([double]$used), ([double]$capacity))
            }
            $fmtPct = {
                param($used, $capacity)
                if ($null -eq $used -or $null -eq $capacity -or [double]$capacity -le 0.0) { return "n/a" }
                return "{0:N2}%" -f (([double]$used * 100.0) / [double]$capacity)
            }
            foreach ($row in $capacityRows) {
                $Summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} | {10} | {11} | {12} | {13} | {14} | {15} | {16} | {17} | {18} | {19} |' -f `
                    $row.Allocator,
                    (& $fmtCount $row.AllocatorCount),
                    (& $fmtPair $row.DescriptorUsed $row.DescriptorCapacity),
                    (& $fmtPct $row.DescriptorUsed $row.DescriptorCapacity),
                    (& $fmtCount $row.DescriptorLiveMax),
                    (& $fmtPair $row.DescriptorMaxAllocator $row.DescriptorLocalCap2x),
                    (& $fmtPair $row.RouteUsed $row.RouteCapacity),
                    (& $fmtPct $row.RouteUsed $row.RouteCapacity),
                    (& $fmtPair $row.RouteOccupiedMaxAllocator $row.RouteLocalCap2x),
                    (& $fmtPair $row.SourceBlockUsed $row.SourceBlockCapacity),
                    (& $fmtPct $row.SourceBlockUsed $row.SourceBlockCapacity),
                    (& $fmtCount $row.SourceBlockActiveMax),
                    (& $fmtPair $row.SourceBlockMaxAllocator $row.SourceBlockLocalCap2x),
                    (& $fmtPair $row.FrontcacheUsed $row.FrontcacheCapacity),
                    (& $fmtPct $row.FrontcacheUsed $row.FrontcacheCapacity),
                    (& $fmtCount $row.FrontcacheTotalMax),
                    (& $fmtPair $row.FrontcacheMaxAllocator $row.FrontcacheLocalCap2x),
                    (& $fmtPair $row.TransferUsed $row.TransferCapacity),
                    (& $fmtPct $row.TransferUsed $row.TransferCapacity),
                    (& $fmtPair $row.TransferMaxAllocator $row.TransferLocalCap2x)))
            }
        }
        if ($DiagnosticHz6Probes -and $warmupRows.Count -gt 0) {
            $Summary.Add("")
            $Summary.Add("### HZ6 main-warmup capacity audit")
            $Summary.Add("")
            $Summary.Add("| allocator | descriptor used/peak | route active/occupied/peak | source blocks used/peak | frontcache used/peak | transfer current/peak | depot run meta init/mark/clear | depot run meta bad class/align/slots/count | failures descriptor/route/source/alloc |")
            $Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
            $fmtPair2 = {
                param($a, $b)
                if ($null -eq $a -or $null -eq $b) { return "n/a" }
                return ("{0:N0}/{1:N0}" -f ([double]$a), ([double]$b))
            }
            $fmtTriple = {
                param($a, $b, $c)
                if ($null -eq $a -or $null -eq $b -or $null -eq $c) { return "n/a" }
                return ("{0:N0}/{1:N0}/{2:N0}" -f ([double]$a), ([double]$b), ([double]$c))
            }
            $fmtQuad = {
                param($a, $b, $c, $d)
                if ($null -eq $a -or $null -eq $b -or $null -eq $c -or $null -eq $d) { return "n/a" }
                return ("{0:N0}/{1:N0}/{2:N0}/{3:N0}" -f ([double]$a), ([double]$b), ([double]$c), ([double]$d))
            }
            foreach ($row in $warmupRows) {
                $Summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} |' -f `
                    $row.Allocator,
                    (& $fmtPair2 $row.DescriptorUsed $row.DescriptorLiveMax),
                    (& $fmtTriple $row.RouteUsed $row.RouteOccupied $row.RouteActiveMax),
                    (& $fmtPair2 $row.SourceBlockUsed $row.SourceBlockActiveMax),
                    (& $fmtPair2 $row.FrontcacheUsed $row.FrontcacheTotalMax),
                    (& $fmtPair2 $row.TransferCurrent $row.TransferCurrentMax),
                    (& $fmtTriple $row.ElasticDepotRunMetaInit $row.ElasticDepotRunMetaMark $row.ElasticDepotRunMetaClear),
                    (& $fmtQuad $row.ElasticDepotRunMetaClassMismatch $row.ElasticDepotRunMetaSlotMisaligned $row.ElasticDepotRunMetaTooManySlots $row.ElasticDepotRunMetaUsedCountMismatch),
                    (& $fmtQuad $row.DescriptorExhausted $row.RouteRegisterFail $row.SourceBlockExhausted $row.AllocFail)))
            }
        }
        if ($DiagnosticHz6Probes -and $elasticRows.Count -gt 0) {
            $Summary.Add("")
            $Summary.Add("### HZ6 elastic capacity projection")
            $Summary.Add("")
            $Summary.Add("| allocator | allocators | projected static KiB | projected static+payload KiB | static savings KiB | descriptor cap/KiB | route cap/KiB | source cap/KiB | frontcache cap/KiB | transfer cap/KiB |")
            $Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
            $fmtKb = {
                param($value)
                if ($null -eq $value) { return "n/a" }
                return "{0:N0}" -f ([double]$value / 1024.0)
            }
            $fmtCount = {
                param($value)
                if ($null -eq $value) { return "n/a" }
                return "{0:N0}" -f ([double]$value)
            }
            $fmtCapKb = {
                param($capacity, $bytes)
                if ($null -eq $capacity -or $null -eq $bytes) { return "n/a" }
                return ("{0:N0}/{1:N0}" -f ([double]$capacity), ([double]$bytes / 1024.0))
            }
            foreach ($row in $elasticRows) {
                $Summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} |' -f `
                    $row.Allocator,
                    (& $fmtCount $row.AllocatorCount),
                    (& $fmtKb $row.ProjectedStatic),
                    (& $fmtKb $row.ProjectedStaticPlusPayload),
                    (& $fmtKb $row.StaticSavings),
                    (& $fmtCapKb $row.DescriptorCapacity $row.DescriptorBytes),
                    (& $fmtCapKb $row.RouteCapacity $row.RouteBytes),
                    (& $fmtCapKb $row.SourceBlockCapacity $row.SourceBlockBytes),
                    (& $fmtCapKb $row.FrontcacheCapacity $row.FrontcacheBytes),
                    (& $fmtCapKb $row.TransferCapacity $row.TransferBytes)))
            }
        }
        if ($DiagnosticHz6Probes -and $overflowRows.Count -gt 0) {
            $Summary.Add("")
            $Summary.Add("### HZ6 elastic overflow projection")
            $Summary.Add("")
            $Summary.Add("| allocator | descriptor local/overflow/KiB | route local/overflow/KiB | source local/overflow/KiB | frontcache local/overflow/KiB | transfer local/overflow/KiB | overflow total KiB |")
            $Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
            $fmtTripleKb = {
                param($localCap, $overflow, $bytes)
                if ($null -eq $localCap -or $null -eq $overflow -or $null -eq $bytes) { return "n/a" }
                return ("{0:N0}/{1:N0}/{2:N0}" -f ([double]$localCap), ([double]$overflow), ([double]$bytes / 1024.0))
            }
            $fmtKb = {
                param($value)
                if ($null -eq $value) { return "n/a" }
                return "{0:N0}" -f ([double]$value / 1024.0)
            }
            foreach ($row in $overflowRows) {
                $Summary.Add(('| {0} | {1} | {2} | {3} | {4} | {5} | {6} |' -f `
                    $row.Allocator,
                    (& $fmtTripleKb $row.DescriptorLocalCap $row.DescriptorOverflow $row.DescriptorOverflowBytes),
                    (& $fmtTripleKb $row.RouteLocalCap $row.RouteOverflow $row.RouteOverflowBytes),
                    (& $fmtTripleKb $row.SourceBlockLocalCap $row.SourceBlockOverflow $row.SourceBlockOverflowBytes),
                    (& $fmtTripleKb $row.FrontcacheLocalCap $row.FrontcacheOverflow $row.FrontcacheOverflowBytes),
                    (& $fmtTripleKb $row.TransferLocalCap $row.TransferOverflow $row.TransferOverflowBytes),
                    (& $fmtKb $row.OverflowTotalBytes)))
            }
        }
        $Summary.Add("")
    }
}

if ($RawLogWriter) {
    $RawLogWriter.Flush()
    $RawLogWriter.Dispose()
}

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
