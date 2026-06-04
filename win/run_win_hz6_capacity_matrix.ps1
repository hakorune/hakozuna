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
            if ($line -match '^(\[BENCH_ARGS\]|\[RSS\]|\[OPS\]|\[HZ6_STATS\]|\[HZ6_MEMORY_ATTR\]|\[HZ6_METADATA_SLIM\]|\[HZ6_FRONT_ALLOC_PATH\]|\[HZ6_FRONTCACHE_CLASS\]|\[HZ6_ROUTE_PROBE_SHAPE\]|\[HZ6_REDIS_STATS\]|threads=.*ops/s=|bench_[^:]+:.*ops/s=|Pattern:|Throughput:|Throughput = |Ops:|---)') {
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
