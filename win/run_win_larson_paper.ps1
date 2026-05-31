param(
    [string]$OutputDir,
    [int]$Runs = 5,
    [int[]]$ThreadCounts,
    [int]$TimeoutSeconds = 120,
    [switch]$IncludeCompactControl,
    [switch]$IncludeWorkerWarmupControl,
    [int]$CompactChunksPerThread = 400,
    [switch]$IncludeHz6CapacityControls,
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
    @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_appcap.exe") },
    @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_appcap.exe") },
    @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_appcap.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_larson_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_larson_tcmalloc.exe") }
)

if ($IncludeHz6CapacityControls) {
    $Executables = @(
        @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_larson_crt.exe") },
        @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_larson_hz3.exe") },
        @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_larson_hz4.exe") },
        @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_larson_hz5_policy.exe") },
        @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict.exe") },
        @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed.exe") },
        @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss.exe") },
        @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_broad.exe") },
        @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_broad.exe") },
        @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_broad.exe") },
        @{ Name = "hz6-strict-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_strict_appcap.exe") },
        @{ Name = "hz6-speed-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_speed_appcap.exe") },
        @{ Name = "hz6-rss-appcap"; Path = (Join-Path $SuiteDir "bench_larson_hz6_rss_appcap.exe") },
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

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $FilePath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Arguments = ($Arguments | ForEach-Object {
        if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
    }) -join ' '

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    [void]$proc.Start()
    if (-not $proc.WaitForExit($TimeoutSeconds * 1000)) {
        try {
            $proc.Kill($true)
        } catch {
            try { $proc.Kill() } catch {}
        }
        $proc.WaitForExit()
        return @{
            ExitCode = 124
            Lines = @("[TIMEOUT] exceeded ${TimeoutSeconds}s")
        }
    }
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()

    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($chunk in @($stdout, $stderr)) {
        if (-not [string]::IsNullOrEmpty($chunk)) {
            foreach ($line in ($chunk -split "`r?`n")) {
                if ($line -ne "") { $lines.Add($line) }
            }
        }
    }

    return @{ ExitCode = $proc.ExitCode; Lines = $lines }
}

function Parse-Hz6Stats {
    param([string[]]$Lines)

    $result = @{
        RouteMiss = "NA"
        SourceAlloc = "NA"
        TransferPush = "NA"
        TransferPop = "NA"
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
    }

    foreach ($line in $Lines) {
        if ($line.StartsWith("[HZ6_STATS]")) {
            foreach ($part in $line.Split(" ")) {
                switch -Regex ($part) {
                    '^route_miss=(.*)$' { $result.RouteMiss = $Matches[1]; continue }
                    '^source_alloc=(.*)$' { $result.SourceAlloc = $Matches[1]; continue }
                    '^transfer_push=(.*)$' { $result.TransferPush = $Matches[1]; continue }
                    '^transfer_pop=(.*)$' { $result.TransferPop = $Matches[1]; continue }
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
$Summary.Add('- thread sweep: `1, 4, 8, 16`')
$Summary.Add(('- runs: `{0}`' -f $Runs))
$Summary.Add(('- timeout: `{0}s` per allocator row' -f $TimeoutSeconds))
$Summary.Add('- statistic: `median alloc/s` from the benchmark''s `Throughput = ...` line')
$Summary.Add('- note: paper originally reports `system / hz3 / mimalloc / tcmalloc`; this runner also records `hz4`')
$Summary.Add("")

function Invoke-LarsonSweep {
    param(
        [string]$SectionTitle,
        [int]$ChunksPerThreadValue,
        [int]$WarmupMode = 0
    )

    foreach ($threads in $ThreadCounts) {
        $Summary.Add("## " + $SectionTitle + " T=" + $threads)
        $Summary.Add("")
        $Summary.Add("| allocator | median ops/s | route_miss | source_alloc | local2p_source_alloc | midpage_source_alloc | large_source_alloc | toy_source_alloc | front_source_ops_alloc | front_source_slot_alloc | front_source_prefill_alloc | toy_source_prefill_call | front_path_local2p | front_path_midpage | front_path_large | front_path_toy | transfer_push | transfer_pop | frontcache_reuse_hit | frontcache_reuse_invalid | transfer_reuse_hit | transfer_reuse_invalid | source_refill_starvation | source_refill_saturation | source_refill_boost | source_refill_clamp | source_admission_open | source_admission_boosted | source_admission_clamped | source_prefill_attempt | source_prefill_filled | source_prefill_fallback | alloc_fail | desc_probe | reg_probe | unreg_probe | srcblk_probe | runs |")
        $Summary.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |")

        foreach ($exe in $Executables) {
            $opsRuns = New-Object System.Collections.Generic.List[double]
            $runTexts = New-Object System.Collections.Generic.List[string]
            $lastStats = @{
                RouteMiss = "NA"
                SourceAlloc = "NA"
                TransferPush = "NA"
                TransferPop = "NA"
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
                $runTexts.Add(("{0:N3}M" -f ($ops / 1000000.0)))
                $parsedStats = Parse-Hz6Stats -Lines $output
                if ($exe.Name -like "hz6-*") {
                    $lastStats = $parsedStats
                }
            }

            if ($opsRuns.Count -eq 0) {
                $failedMetrics = ((@('NA') * 35) -join ' | ')
                $Summary.Add(('| {0} | failed | {1} | `{2}` |' -f $exe.Name, $failedMetrics, ($runTexts -join ", ")))
                continue
            }

            $medianOps = Get-Median -Values $opsRuns.ToArray()
                $Summary.Add((
                '| {0} | {1:N3}M | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} | {10} | {11} | {12} | {13} | {14} | {15} | {16} | {17} | {18} | {19} | {20} | {21} | {22} | {23} | {24} | {25} | {26} | {27} | {28} | {29} | {30} | {31} | {32} | {33} | {34} | {35} | `{36}` |' -f
                $exe.Name,
                ($medianOps / 1000000.0),
                $lastStats.RouteMiss,
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
                $lastStats.SourceBlockProbeTotal,
                ($runTexts -join ", ")))

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
