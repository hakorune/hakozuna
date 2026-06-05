param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [string[]]$Families,
    [string[]]$BenchmarkProfiles,
    [string[]]$Hz6Profiles,
    [string[]]$CapacityLanes,
    [int[]]$ThreadCounts,
    [int]$TimeoutSeconds = 120,
    [switch]$SkipBuild,
    [switch]$ForceBuild,
    [switch]$DiagnosticHz6Probes,
    [switch]$ContinueOnFailure,
    [switch]$ListOnly
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDirName = if ($DiagnosticHz6Probes) { "out_win_hz6_capacity_diag" } else { "out_win_hz6_capacity" }
$SuiteDir = Join-Path $RepoRoot $SuiteDirName
$BuildScript = Join-Path $PSScriptRoot "build_win_hz6_capacity_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

function Split-List {
    param([string[]]$Values)
    $items = @()
    foreach ($value in @($Values)) {
        foreach ($part in @($value -split ',' | ForEach-Object { $_.Trim().ToLowerInvariant() } | Where-Object { $_ -ne "" })) {
            $items += $part
        }
    }
    $items
}

function Get-Median {
    param([double[]]$Values)
    if ($null -eq $Values -or $Values.Length -eq 0) {
        return [double]::NaN
    }
    $sorted = [double[]]$Values.Clone()
    [Array]::Sort($sorted)
    $mid = [int][Math]::Floor($sorted.Count / 2.0)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$mid]
    }
    return ([double]$sorted[$mid - 1] + [double]$sorted[$mid]) / 2.0
}

function Get-KeyValueMap {
    param([string]$Line)
    $map = @{}
    foreach ($match in [regex]::Matches($Line, '([A-Za-z0-9_]+)=([0-9]+)')) {
        $map[$match.Groups[1].Value] = [double]$match.Groups[2].Value
    }
    return $map
}

function Get-MedianFromMaps {
    param(
        [array]$Maps,
        [string]$Key
    )
    $values = New-Object System.Collections.Generic.List[double]
    foreach ($map in @($Maps)) {
        if ($null -ne $map -and $map.ContainsKey($Key)) {
            $values.Add([double]$map[$Key])
        }
    }
    if ($values.Count -eq 0) {
        return $null
    }
    return Get-Median -Values $values.ToArray()
}

function Get-LogCapture {
    param(
        [Parameter(Mandatory = $true)][string[]]$Paths,
        [int]$TailLimit = 200,
        [switch]$IncludeStatsTail
    )

    $captured = New-Object System.Collections.Generic.List[string]
    $tail = New-Object System.Collections.Generic.Queue[string]
    $statsTail = New-Object System.Collections.Generic.Queue[string]

    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }
        foreach ($line in [System.IO.File]::ReadLines($path)) {
            if ($null -eq $line) { continue }
            if ($line -ne "") {
                if ($tail.Count -ge $TailLimit) {
                    [void]$tail.Dequeue()
                }
                [void]$tail.Enqueue($line)
            }
            if ($line -match '^(\[BENCH_ARGS\]|\[RSS\]|\[OPS\]|\[HZ6_STATS\]|\[HZ6_MEMORY_ATTR\]|\[HZ6_RSS_RESIDUAL\]|\[HZ6_CAPACITY_UTIL\]|\[HZ6_MAIN_WARMUP_CAPACITY\]|\[HZ6_ELASTIC_PROJECTION\]|\[HZ6_ELASTIC_OVERFLOW_PROJECTION\]|\[HZ6_METADATA_SLIM\]|\[HZ6_FRONT_ALLOC_PATH\]|\[HZ6_FRONTCACHE_CLASS\]|\[HZ6_ROUTE_PROBE_SHAPE\]|\[HZ6_REDIS_STATS\]|threads=.*ops/s=|bench_[^:]+:.*ops/s=|Pattern:|Throughput:|Throughput = |Ops:|---)') {
                [void]$captured.Add($line)
            } elseif ($IncludeStatsTail -and $line -match '^\[HZ6_STATS\]\s+label=redis_alloc_string_fail') {
                if ($statsTail.Count -ge $TailLimit) {
                    [void]$statsTail.Dequeue()
                }
                [void]$statsTail.Enqueue($line)
            }
        }
    }

    if ($captured.Count -eq 0 -and $tail.Count -gt 0) {
        foreach ($line in $tail) {
            [void]$captured.Add($line)
        }
    }

    if ($IncludeStatsTail -and $statsTail.Count -gt 0) {
        [void]$captured.Add("[TIMEOUT_CAPTURED_STATS_TAIL] last $($statsTail.Count) redis_alloc_string_fail stats lines")
        foreach ($line in $statsTail) {
            [void]$captured.Add($line)
        }
    }

    return ,$captured.ToArray()
}

function Invoke-CapturedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [int]$TimeoutSeconds = 120
    )

    $stdoutPath = [System.IO.Path]::GetTempFileName()
    $stderrPath = [System.IO.Path]::GetTempFileName()
    $argumentList = ($Arguments | ForEach-Object { [string]$_ })
    $proc = Start-Process -FilePath $FilePath `
        -ArgumentList $argumentList `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath `
        -NoNewWindow `
        -PassThru
    $peakWorkingSetBytes = [Int64]0
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)

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
        Start-Sleep -Milliseconds 1
    }

    if (-not $proc.HasExited) {
        try {
            $proc.Kill($true)
        } catch {
            try { $proc.Kill() } catch {}
        }
        $proc.WaitForExit()
        $lines = New-Object System.Collections.Generic.List[string]
        $lines.Add("[TIMEOUT] exceeded ${TimeoutSeconds}s")
        $captured = Get-LogCapture -Paths @($stdoutPath, $stderrPath) -TailLimit 200 -IncludeStatsTail
        foreach ($line in $captured) {
            $lines.Add($line)
        }
        Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue
        $peakKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
        return @{
            ExitCode = 124
            Lines = $lines
            PeakKb = $peakKb
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

    $proc.WaitForExit()

    $lines = New-Object System.Collections.Generic.List[string]
    $captured = Get-LogCapture -Paths @($stdoutPath, $stderrPath) -TailLimit 200
    foreach ($line in $captured) {
        if ($line -ne "") { $lines.Add($line) }
    }
    Remove-Item -LiteralPath $stdoutPath, $stderrPath -Force -ErrorAction SilentlyContinue

    $peakKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
    $text = ($lines -join " ")
    if ($text -match 'peak_kb=([0-9]+)') {
        $peakKb = $Matches[1]
    }

    $exitCode = $proc.ExitCode
    if ($null -eq $exitCode -or "$exitCode" -eq "") {
        $exitCode = 0
    }
    return @{ ExitCode = $exitCode; Lines = $lines; PeakKb = $peakKb }
}

function Expand-ProfileSelection {
    param(
        [Parameter(Mandatory = $true)][array]$AllProfiles,
        [string[]]$Requested
    )
    $names = Split-List -Values $Requested
    if (-not $names -or $names.Count -eq 0) {
        return $AllProfiles
    }
    $selected = @()
    foreach ($name in $names) {
        if ($name -eq "large_slices") {
            foreach ($slice in @("large_slice_256", "large_slice_512", "large_slice_1k", "large_slice_2k", "large_slice_4k", "large_slice_8k", "large_slice_16k", "large_slice_32k", "large_slice_64k", "large_slice_128k")) {
                $selected += ($AllProfiles | Where-Object { $_.Name -eq $slice })
            }
            continue
        }
        $match = $AllProfiles | Where-Object { $_.Name -eq $name }
        if (-not $match) {
            throw "Unknown benchmark profile: $name"
        }
        $selected += $match
    }
    $selected
}

$selectedFamilies = Split-List -Values $Families
if (-not $selectedFamilies -or $selectedFamilies.Count -eq 0) {
    $selectedFamilies = @("mixed_ws", "random_mixed", "larson", "redis")
}

$selectedHz6Profiles = Split-List -Values $Hz6Profiles
if (-not $selectedHz6Profiles -or $selectedHz6Profiles.Count -eq 0) {
    $selectedHz6Profiles = @("strict", "speed", "rss")
}

$selectedLanes = Split-List -Values $CapacityLanes
if (-not $selectedLanes -or $selectedLanes.Count -eq 0) {
    $selectedLanes = @("control", "route4k", "appcap")
}

if (-not $ThreadCounts -or $ThreadCounts.Count -eq 0) {
    $ThreadCounts = @(1, 4, 8, 16)
}

$laneSuffix = @{
    "default" = ""
    "broad" = "_broad"
    "control" = "_control"
    "route4k" = "_route4k"
    "noboost-route4k" = "_noboost_route4k"
    "localexactfree-noboost-route4k" = "_localexactfree_noboost_route4k"
    "directlocalfree-noboost-route4k" = "_directlocalfree_noboost_route4k"
    "descavail-noboost-route4k" = "_descavail_noboost_route4k"
    "directlocalfree-descavail-noboost-route4k" = "_directlocalfree_descavail_noboost_route4k"
    "directlocalalloc-descavail-noboost-route4k" = "_directlocalalloc_descavail_noboost_route4k"
    "directlocalreuse-descavail-noboost-route4k" = "_directlocalreuse_descavail_noboost_route4k"
    "directlocalfreealloc-descavail-noboost-route4k" = "_directlocalfreealloc_descavail_noboost_route4k"
    "directlocalfreereuse-descavail-noboost-route4k" = "_directlocalfreereuse_descavail_noboost_route4k"
    "sameownerfast-descavail-noboost-route4k" = "_sameownerfast_descavail_noboost_route4k"
    "spill-route4k" = "_spill_route4k"
    "borrow-route4k" = "_borrow_route4k"
    "cap-route4k" = "_cap_route4k"
    "sourcerun-route4k" = "_sourcerun_route4k"
    "sourcerun-reclaim-route4k" = "_sourcerun_reclaim_route4k"
    "sourcerun-sameclass-route4k" = "_sourcerun_sameclass_route4k"
    "descriptorless-route4k" = "_descriptorless_route4k"
    "descriptorreserve-route4k" = "_descriptorreserve_route4k"
    "descriptorcold-route4k" = "_descriptorcold_route4k"
    "descriptorcoldgov-route4k" = "_descriptorcoldgov_route4k"
    "descriptorcoldgov-widews-route4k" = "_descriptorcoldgov_widews_route4k"
    "desc4k-route4k" = "_desc4k_route4k"
    "source512-route4k" = "_source512_route4k"
    "desc4k-source512-route4k" = "_desc4k_source512_route4k"
    "redislowrss-route4k" = "_redislowrss_route4k"
    "redislowrss-sourcerun-route4k" = "_redislowrss_sourcerun_route4k"
    "redislowrss-sourcerun-route8k" = "_redislowrss_sourcerun_route8k"
    "redislowrss-sourcerun-desc8k-route8k" = "_redislowrss_sourcerun_desc8k_route8k"
    "largerlowrss-sourcerun-desc8k-route8k" = "_largerlowrss_sourcerun_desc8k_route8k"
    "redislowrss-sourcerun-desc8k-route8k-tombcompact" = "_redislowrss_sourcerun_desc8k_route8k_tombcompact"
    "redislowrss-sourcerun-desc8k-route8k-retainedoverflow" = "_redislowrss_sourcerun_desc8k_route8k_retainedoverflow"
    "redislowrss-sourcerun-desc8k-route8k-slotlookup" = "_redislowrss_sourcerun_desc8k_route8k_slotlookup"
    "largerlowrss-sourcerun-desc8k-route64k" = "_largerlowrss_sourcerun_desc8k_route64k"
    "largerlowrss-sourcerun-desc8k-source2k-route64k" = "_largerlowrss_sourcerun_desc8k_source2k_route64k"
    "largerlowrss-front8k-sourcerun-desc8k-route8k" = "_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "largerlowrss-front6k-sourcerun-desc8k-route8k" = "_largerlowrss_front6k_sourcerun_desc8k_route8k"
    "largerlowrss-front4k-sourcerun-desc8k-route8k" = "_largerlowrss_front4k_sourcerun_desc8k_route8k"
    "mixedclean-front8k-sourcerun-desc16k-source2k-route16k" = "_mixedclean_front8k_sourcerun_desc16k_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc16k-source2k-route16k" = "_mixedclean_front16k_sourcerun_desc16k_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc16k-transfer2304-source2k-route16k" = "_mixedclean_front16k_sourcerun_desc16k_transfer2304_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc16k-transfer2560-source2k-route16k" = "_mixedclean_front16k_sourcerun_desc16k_transfer2560_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc24k-source2k-route24k" = "_mixedclean_front16k_sourcerun_desc24k_source2k_route24k"
    "mixedclean-front16k-sourcerun-desc22k-source2k-route22k" = "_mixedclean_front16k_sourcerun_desc22k_source2k_route22k"
    "mixedclean-front16k-sourcerun-desc20k-source2k-route20k" = "_mixedclean_front16k_sourcerun_desc20k_source2k_route20k"
    "mixedclean-front16k-sourcerun-desc19k-source2k-route19k" = "_mixedclean_front16k_sourcerun_desc19k_source2k_route19k"
    "mixedclean-front16k-sourcerun-desc18k-source2k-route18k" = "_mixedclean_front16k_sourcerun_desc18k_source2k_route18k"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route17k" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k_linearwrap"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k_linearwrap_loopcarry"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-doublehash" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_doublehash"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-hashxor" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_hashxor"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-linearwrap" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_linearwrap"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route20k" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route20k"
    "mixedclean-front16k-sourcerun-desc32k-source4k-route32k" = "_mixedclean_front16k_sourcerun_desc32k_source4k_route32k"
    "mixedclean-front16k-sourcerun-desc32k-source3k-route32k" = "_mixedclean_front16k_sourcerun_desc32k_source3k_route32k"
    "mixedclean-front8k-sourcerun-desc32k-source4k-route32k" = "_mixedclean_front8k_sourcerun_desc32k_source4k_route32k"
    "mixedclean-front12k-sourcerun-desc32k-source4k-route32k" = "_mixedclean_front12k_sourcerun_desc32k_source4k_route32k"
    "mixedclean-front16k-sourcerun-desc32k-source2k-route32k" = "_mixedclean_front16k_sourcerun_desc32k_source2k_route32k"
    "redislowrss-slim-route4k" = "_redislowrss_slim_route4k"
    "front1k-desc4k-source512-route4k" = "_front1k_desc4k_source512_route4k"
    "appcap" = "_appcap"
    "visiblefirst-appcap" = "_visiblefirst_appcap"
    "negativefilter-appcap" = "_negativefilter_appcap"
    "sharedir-appcap" = "_sharedir_appcap"
    "sharedirfirst-appcap" = "_sharedirfirst_appcap"
    "ownerlocality-appcap" = "_ownerlocality_appcap"
    "ownerlocalityfast-appcap" = "_ownerlocalityfast_appcap"
    "ownerlocalityfast-rsscap-1" = "_ownerlocalityfast_rsscap_1"
    "ownerlocalityfast-rsscap-2" = "_ownerlocalityfast_rsscap_2"
    "ownerlocalityfast-rsscap-2-desc192k" = "_ownerlocalityfast_rsscap_2_desc192k"
    "ownerlocalityfast-rsscap-2-desc160k" = "_ownerlocalityfast_rsscap_2_desc160k"
    "ownerlocalityfast-rsscap-2-desc160k-route128k" = "_ownerlocalityfast_rsscap_2_desc160k_route128k"
    "ownerlocalityfast-rsscap-2-desc160k-source2k" = "_ownerlocalityfast_rsscap_2_desc160k_source2k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source12k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source14k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route128k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route224k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run2048"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run1024"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir192k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_dir192k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_dir192k_routepacked_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_osdry_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_sbp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source2k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s2_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source8k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s8_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source12k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s12_r192_run512"
    "ownerlocalityfast-rsscap-2-elasticproj-local1k-route16k-source64-front1k-packed" = "_ownerloc_rss2_elproj_l1k_r16k_s64_f1k_packed"
    "ownerlocalityfast-rsscap-2-elasticproj-live2k-route16k-source128-front1k-packed" = "_ownerloc_rss2_elproj_l2k_r16k_s128_f1k_packed"
    "ownerlocalityfast-rsscap-2-elasticroute-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512" = "_ownerloc_rss2_elroute_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescroute-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512" = "_ownerloc_rss2_eldescroute_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-localizedryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_locdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-runlocalitydryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_runlocdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_depotrunmeta_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_slotownerdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownersparse-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_slotownersparse_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2dryrun-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2dry_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_so16_nrb_d192_rp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir128k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir128k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir96k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir96k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_sideowner16_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc158k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc156k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc152k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc148k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc144k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc128k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route160k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route160k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route128k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route96k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route96k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route64k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route64k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source32k"
    "ownerlocalityfast-rsscap-2-desc144k" = "_ownerlocalityfast_rsscap_2_desc144k"
    "ownerlocalityfast-rsscap-3" = "_ownerlocalityfast_rsscap_3"
    "ownerlocalityfast-rsscap-4" = "_ownerlocalityfast_rsscap_4"
    "directlocalfree-ownerlocalityfast-rsscap-4" = "_directlocalfree_ownerlocalityfast_rsscap_4"
    "ownerlocalityfast-widecap-1" = "_ownerlocalityfast_widecap_1"
    "ownerlocalityfast-widecap-2" = "_ownerlocalityfast_widecap_2"
    "ownerlocalityfast-widecap-3" = "_ownerlocalityfast_widecap_3"
    "ownerlocalityfast-widecap-4" = "_ownerlocalityfast_widecap_4"
    "directlocalfree-ownerlocalityfast-widecap-4" = "_directlocalfree_ownerlocalityfast_widecap_4"
}

$familyPrefixes = @{
    "mixed_ws" = "bench_mixed_ws"
    "random_mixed" = "bench_random_mixed"
    "larson" = "bench_larson"
    "redis" = "bench_redis_workload"
}

$Executables = @()
foreach ($family in $selectedFamilies) {
    if (-not $familyPrefixes.ContainsKey($family)) {
        throw "Unknown benchmark family: $family"
    }
    foreach ($profile in $selectedHz6Profiles) {
        foreach ($lane in $selectedLanes) {
            if (-not $laneSuffix.ContainsKey($lane)) {
                throw "Unknown HZ6 capacity lane: $lane"
            }
            $name = "hz6-$profile"
            if ($lane -ne "default") {
                $name += "-$lane"
            }
            $Executables += @{
                Family = $family
                Name = $name
                Path = (Join-Path $SuiteDir ("{0}_hz6_{1}{2}.exe" -f $familyPrefixes[$family], $profile, $laneSuffix[$lane]))
            }
        }
    }
}

if ($ListOnly) {
    $Executables | ForEach-Object { "{0}`t{1}`t{2}" -f $_.Family, $_.Name, $_.Path }
    return
}

if (-not $SkipBuild) {
    $missing = @($Executables | Where-Object { -not (Test-Path $_.Path) })
    if ($ForceBuild -or $missing.Count -gt 0) {
        & $BuildScript `
            -Families $selectedFamilies `
            -Hz6Profiles $selectedHz6Profiles `
            -CapacityLanes $selectedLanes `
            -OutDirName $SuiteDirName `
            -DiagnosticHz6Probes:$DiagnosticHz6Probes
        if ($LASTEXITCODE -ne 0) {
            throw "build_win_hz6_capacity_suite.ps1 failed with exit code $LASTEXITCODE"
        }
    }
}

foreach ($exe in $Executables) {
    if (-not (Test-Path $exe.Path)) {
        throw "Missing HZ6 executable: $($exe.Path)"
    }
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null
$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz6_capacity_matrix_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_hz6_capacity_matrix_windows.log")
$Summary = New-Object System.Collections.Generic.List[string]
$Utf8BomEncoding = New-Object System.Text.UTF8Encoding $true
$RawLogWriter = New-Object System.IO.StreamWriter($RawLogPath, $false, $Utf8BomEncoding)
$RawLogWriter.AutoFlush = $true

$Summary.Add("# Windows HZ6 Capacity Matrix")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("- artifacts: [$SuiteDirName]($SuiteDir)")
$Summary.Add(('- families: `{0}`' -f ($selectedFamilies -join ", ")))
$Summary.Add(('- HZ6 profiles: `{0}`' -f ($selectedHz6Profiles -join ", ")))
$Summary.Add(('- capacity lanes: `{0}`' -f ($selectedLanes -join ", ")))
$Summary.Add(('- diagnostic probes: `{0}`' -f ([string]$DiagnosticHz6Probes)))
$Summary.Add("")

$mixedProfiles = @(
    @{ Name = "smoke"; Args = @("2", "10000", "256", "16", "128"); Note = "quick sanity and regression check" },
    @{ Name = "balanced"; Args = @("8", "250000", "4096", "16", "2048"); Note = "larger mixed run for first Windows compare" },
    @{ Name = "wide_ws"; Args = @("8", "200000", "16384", "16", "1024"); Note = "wider working-set pressure" },
    @{ Name = "larger_sizes"; Args = @("4", "150000", "4096", "256", "8192"); Note = "shift toward larger allocations" },
    @{ Name = "large_slice_256"; Args = @("4", "150000", "4096", "256", "256"); Note = "fixed-size large slice: 256 bytes" },
    @{ Name = "large_slice_512"; Args = @("4", "150000", "4096", "512", "512"); Note = "fixed-size large slice: 512 bytes" },
    @{ Name = "large_slice_1k"; Args = @("4", "150000", "4096", "1024", "1024"); Note = "fixed-size large slice: 1 KiB" },
    @{ Name = "large_slice_2k"; Args = @("4", "150000", "4096", "2048", "2048"); Note = "fixed-size large slice: 2 KiB" },
    @{ Name = "large_slice_4k"; Args = @("4", "120000", "2048", "4096", "4096"); Note = "fixed-size large slice: 4 KiB" },
    @{ Name = "large_slice_8k"; Args = @("4", "100000", "1024", "8192", "8192"); Note = "fixed-size large slice: 8 KiB" },
    @{ Name = "large_slice_16k"; Args = @("4", "80000", "512", "16384", "16384"); Note = "fixed-size large slice: 16 KiB" },
    @{ Name = "large_slice_32k"; Args = @("4", "60000", "256", "32768", "32768"); Note = "fixed-size large slice: 32 KiB" },
    @{ Name = "large_slice_64k"; Args = @("4", "50000", "128", "65536", "65536"); Note = "fixed-size large slice: 64 KiB" },
    @{ Name = "large_slice_128k"; Args = @("4", "40000", "64", "131072", "131072"); Note = "fixed-size large slice: 128 KiB" },
    @{ Name = "heavy_mixed"; Args = @("8", "5000000", "16384", "16", "4096"); Note = "heavier mixed run with longer timings" }
)
$randomProfiles = @(
    @{ Name = "small"; Args = @("20000000", "400", "16", "2048"); Note = "paper SSOT small range" },
    @{ Name = "medium"; Args = @("20000000", "400", "4096", "32768"); Note = "paper SSOT medium range" },
    @{ Name = "mixed"; Args = @("20000000", "400", "16", "32768"); Note = "paper SSOT mixed range" }
)

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
