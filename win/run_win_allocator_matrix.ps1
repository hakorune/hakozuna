param(
    [string]$OutputDir,
    [string[]]$Profiles,
    [string[]]$Allocators,
    [int]$BenchTimeoutSeconds = 0,
    [switch]$ForceBuild,
    [switch]$ContinueOnFailure,
    [switch]$ListOnly
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"
$Hz12BuildScript = Join-Path $RepoRoot "hakozuna-hz12\scripts\build_hz12_windows_broad_controls.ps1"
$Hz12SuiteDir = Join-Path $RepoRoot "hakozuna-hz12\out_win_broad"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\\benchmarks\\windows"
}

if (-not $ListOnly) {
    New-Item -ItemType Directory -Force $OutputDir | Out-Null
}

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_mixed_ws_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz4.exe") },
    @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz5_policy.exe") },
    @{ Name = "hz11-span"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span.exe") },
    @{ Name = "hz11-span-transfer"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_transfer.exe") },
    @{ Name = "hz11-span-diag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_diag.exe") },
    @{ Name = "hz11-span-tlsfast"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_tlsfast.exe") },
    @{ Name = "hz11-span-cache256"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256.exe") },
    @{ Name = "hz11-span-cache256-bumpbatch16"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_bumpbatch16.exe") },
    @{ Name = "hz11-span-cache256-bumpbatch16-diag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_bumpbatch16_diag.exe") },
    @{ Name = "hz11-span-cache256-returnedrange"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_returnedrange.exe") },
    @{ Name = "hz11-span-cache256-returnedrange-diag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_returnedrange_diag.exe") },
    @{ Name = "hz11-span-cache256-returnedrange32"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_returnedrange32.exe") },
    @{ Name = "hz11-span-tlsfast-cache256"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_tlsfast_cache256.exe") },
    @{ Name = "hz11-span-cache256-diag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_diag.exe") },
    @{ Name = "hz11-span-cache256-matrixattrib"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache256_matrixattrib.exe") },
    @{ Name = "hz11-span-cache512"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512.exe") },
    @{ Name = "hz11-span-cache512-diag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_diag.exe") },
    @{ Name = "hz11-span-cache512-matrixattrib"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_matrixattrib.exe") },
    @{ Name = "hz11-span-cache512-classdiag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classdiag.exe") },
    @{ Name = "hz11-span-cache512-classbatch"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch.exe") },
    @{ Name = "hz11-span-cache512-classbatch16"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch16.exe") },
    @{ Name = "hz11-span-cache512-classbatch16-4-7"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch16_4_7.exe") },
    @{ Name = "hz11-span-cache512-classbatch16-matrixattrib"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch16_matrixattrib.exe") },
    @{ Name = "hz11-span-cache512-classbatch16-coldskip"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch16_coldskip.exe") },
    @{ Name = "hz11-span-cache512-classbatch16-coldskip-matrixattrib"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch16_coldskip_matrixattrib.exe") },
    @{ Name = "hz11-span-cache512-classbatch-diag"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz11_span_cache512_classbatch_diag.exe") },
    @{ Name = "hz12-core"; Path = (Join-Path $Hz12SuiteDir "bench_mixed_ws_hz12_core.exe") },
    @{ Name = "hz12-ownermap"; Path = (Join-Path $Hz12SuiteDir "bench_mixed_ws_hz12_ownermap.exe") },
    @{ Name = "hz12-allocmap"; Path = (Join-Path $Hz12SuiteDir "bench_mixed_ws_hz12_allocmap.exe") },
    @{ Name = "hz12-flushroute"; Path = (Join-Path $Hz12SuiteDir "bench_mixed_ws_hz12_flushroute.exe") },
    @{ Name = "hz12-coldspanowner"; Path = (Join-Path $Hz12SuiteDir "bench_mixed_ws_hz12_coldspanowner.exe") },
    @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict.exe") },
    @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed.exe") },
    @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss.exe") },
    @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_broad.exe") },
    @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_broad.exe") },
    @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_broad.exe") },
    @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_route4k.exe") },
    @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_route4k.exe") },
    @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_route4k.exe") },
    @{ Name = "hz6-strict-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_largerlowrss.exe") },
    @{ Name = "hz6-speed-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_largerlowrss.exe") },
    @{ Name = "hz6-rss-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_largerlowrss.exe") },
    @{ Name = "hz6-strict-largedirectretain16m-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_largedirectretain16m_largerlowrss.exe") },
    @{ Name = "hz6-speed-largedirectretain16m-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_largedirectretain16m_largerlowrss.exe") },
    @{ Name = "hz6-rss-largedirectretain16m-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_largedirectretain16m_largerlowrss.exe") },
    @{ Name = "hz6-strict-largedirectretain32m-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_largedirectretain32m_largerlowrss.exe") },
    @{ Name = "hz6-speed-largedirectretain32m-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_largedirectretain32m_largerlowrss.exe") },
    @{ Name = "hz6-rss-largedirectretain32m-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_largedirectretain32m_largerlowrss.exe") },
    @{ Name = "hz6-strict-sameownerfast-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_sameownerfast_largerlowrss.exe") },
    @{ Name = "hz6-speed-sameownerfast-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_sameownerfast_largerlowrss.exe") },
    @{ Name = "hz6-rss-sameownerfast-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_sameownerfast_largerlowrss.exe") },
    @{ Name = "hz6-strict-directlocalsmall8k-sameownerlarge-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_directlocalsmall8k_sameownerlarge_largerlowrss.exe") },
    @{ Name = "hz6-speed-directlocalsmall8k-sameownerlarge-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_directlocalsmall8k_sameownerlarge_largerlowrss.exe") },
    @{ Name = "hz6-rss-directlocalsmall8k-sameownerlarge-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_directlocalsmall8k_sameownerlarge_largerlowrss.exe") },
    @{ Name = "hz6-strict-directlocalfreereuse-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_directlocalfreereuse_largerlowrss.exe") },
    @{ Name = "hz6-speed-directlocalfreereuse-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_directlocalfreereuse_largerlowrss.exe") },
    @{ Name = "hz6-rss-directlocalfreereuse-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_directlocalfreereuse_largerlowrss.exe") },
    @{ Name = "hz6-strict-directlocalfreereuse-small8k-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_directlocalfreereuse_small8k_largerlowrss.exe") },
    @{ Name = "hz6-speed-directlocalfreereuse-small8k-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_directlocalfreereuse_small8k_largerlowrss.exe") },
    @{ Name = "hz6-rss-directlocalfreereuse-small8k-largerlowrss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_directlocalfreereuse_small8k_largerlowrss.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_tcmalloc.exe") }
)

if ($Allocators -and $Allocators.Count -gt 0) {
    $AllocatorNames = @()
    foreach ($name in $Allocators) {
        foreach ($part in @($name -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })) {
            $AllocatorNames += $part
        }
    }
    $SelectedExecutables = @()
    foreach ($name in $AllocatorNames) {
        $match = $Executables | Where-Object { $_.Name -eq $name }
        if (-not $match) {
            throw "Unknown allocator: $name"
        }
        $SelectedExecutables += $match
    }
    $Executables = $SelectedExecutables
}

if (-not $ListOnly -and ($ForceBuild -or ($Executables | Where-Object { -not (Test-Path $_.Path) }))) {
    $OnlyHz11Selected = (($Executables.Count -gt 0) -and
        (($Executables | Where-Object { $_.Name -notlike "hz11-*" }).Count -eq 0))
    $OnlyHz12Selected = (($Executables.Count -gt 0) -and
        (($Executables | Where-Object { $_.Name -notlike "hz12-*" }).Count -eq 0))
    if ($OnlyHz12Selected) {
        & $Hz12BuildScript
    } elseif ($OnlyHz11Selected) {
        & $BuildScript -OnlyHz11
    } else {
        & $BuildScript
    }
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

function Invoke-BenchProcess {
    param(
        [string]$Path,
        [string[]]$BenchArgs
    )

    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $Path
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.Arguments = ($BenchArgs | ForEach-Object {
            if ($_ -match '[\s"]') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ }
        }) -join ' '

        $proc = New-Object System.Diagnostics.Process
        $proc.StartInfo = $psi
        [void]$proc.Start()
        $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
        $stderrTask = $proc.StandardError.ReadToEndAsync()

        $peakWorkingSetBytes = [Int64]0
        $timedOut = $false
        $deadline = if ($BenchTimeoutSeconds -gt 0) {
            [DateTime]::UtcNow.AddSeconds($BenchTimeoutSeconds)
        } else {
            [DateTime]::MaxValue
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

        while (-not $proc.HasExited) {
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
            if ([DateTime]::UtcNow -ge $deadline) {
                $timedOut = $true
                try {
                    $proc.Kill()
                } catch {
                }
                break
            }
            Start-Sleep -Milliseconds 1
        }

        if ($timedOut) {
            try {
                $proc.WaitForExit(5000) | Out-Null
            } catch {
            }
        }
        $proc.WaitForExit()
        $stdout = $stdoutTask.Result
        $stderr = $stderrTask.Result

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

        $output = @()
        if ($stdout) {
            $output += $stdout
        }
        if ($stderr) {
            $output += $stderr
        }
        if ($timedOut) {
            $output += "timeout_seconds=$BenchTimeoutSeconds"
        }
        $rc = if ($timedOut) { -999 } else { $proc.ExitCode }
        $peakKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
        if ($stdout -match 'peak_kb=([0-9]+)') {
            $peakKb = $Matches[1]
        } elseif ($stderr -match 'peak_kb=([0-9]+)') {
            $peakKb = $Matches[1]
        }
        return [pscustomobject]@{
            Output   = $output
            ExitCode = $rc
            PeakKb   = $peakKb
            TimedOut = $timedOut
        }
    } finally {
        $ErrorActionPreference = $prevEap
    }
}

$AllProfiles = @(
    @{ Name = "smoke"; Threads = 2; ItersPerThread = 10000; WorkingSet = 256; MinSize = 16; MaxSize = 128; Note = "quick sanity and regression check" },
    @{ Name = "balanced"; Threads = 8; ItersPerThread = 250000; WorkingSet = 4096; MinSize = 16; MaxSize = 2048; Note = "larger mixed run for first Windows compare" },
    @{ Name = "wide_ws"; Threads = 8; ItersPerThread = 200000; WorkingSet = 16384; MinSize = 16; MaxSize = 1024; Note = "wider working-set pressure" },
    @{ Name = "larger_sizes"; Threads = 4; ItersPerThread = 150000; WorkingSet = 4096; MinSize = 256; MaxSize = 8192; Note = "shift toward larger allocations" },
    @{ Name = "large_slice_256"; Threads = 4; ItersPerThread = 150000; WorkingSet = 4096; MinSize = 256; MaxSize = 256; Note = "fixed-size large slice: 256 bytes" },
    @{ Name = "large_slice_512"; Threads = 4; ItersPerThread = 150000; WorkingSet = 4096; MinSize = 512; MaxSize = 512; Note = "fixed-size large slice: 512 bytes" },
    @{ Name = "large_slice_1k"; Threads = 4; ItersPerThread = 150000; WorkingSet = 4096; MinSize = 1024; MaxSize = 1024; Note = "fixed-size large slice: 1 KiB" },
    @{ Name = "large_slice_2k"; Threads = 4; ItersPerThread = 150000; WorkingSet = 4096; MinSize = 2048; MaxSize = 2048; Note = "fixed-size large slice: 2 KiB" },
    @{ Name = "large_slice_4k"; Threads = 4; ItersPerThread = 120000; WorkingSet = 2048; MinSize = 4096; MaxSize = 4096; Note = "fixed-size large slice: 4 KiB" },
    @{ Name = "large_slice_8k"; Threads = 4; ItersPerThread = 100000; WorkingSet = 1024; MinSize = 8192; MaxSize = 8192; Note = "fixed-size large slice: 8 KiB" },
    @{ Name = "large_slice_16k"; Threads = 4; ItersPerThread = 80000; WorkingSet = 512; MinSize = 16384; MaxSize = 16384; Note = "fixed-size large slice: 16 KiB" },
    @{ Name = "large_slice_32k"; Threads = 4; ItersPerThread = 60000; WorkingSet = 256; MinSize = 32768; MaxSize = 32768; Note = "fixed-size large slice: 32 KiB" },
    @{ Name = "large_slice_64k"; Threads = 4; ItersPerThread = 50000; WorkingSet = 128; MinSize = 65536; MaxSize = 65536; Note = "fixed-size large slice: 64 KiB" },
    @{ Name = "large_slice_128k"; Threads = 4; ItersPerThread = 40000; WorkingSet = 64; MinSize = 131072; MaxSize = 131072; Note = "fixed-size large slice: 128 KiB" },
    @{ Name = "large_slice_256k"; Threads = 4; ItersPerThread = 30000; WorkingSet = 32; MinSize = 262144; MaxSize = 262144; Note = "fixed-size large slice: 256 KiB" },
    @{ Name = "large_slice_512k"; Threads = 4; ItersPerThread = 20000; WorkingSet = 16; MinSize = 524288; MaxSize = 524288; Note = "fixed-size large slice: 512 KiB" },
    @{ Name = "large_slice_1m"; Threads = 4; ItersPerThread = 12000; WorkingSet = 8; MinSize = 1048576; MaxSize = 1048576; Note = "fixed-size large slice: 1 MiB" },
    @{ Name = "large_direct_slice_2m"; Threads = 4; ItersPerThread = 8000; WorkingSet = 4; MinSize = 2097152; MaxSize = 2097152; Note = "fixed-size direct large slice: 2 MiB" },
    @{ Name = "large_direct_slice_4m"; Threads = 4; ItersPerThread = 5000; WorkingSet = 3; MinSize = 4194304; MaxSize = 4194304; Note = "fixed-size direct large slice: 4 MiB" },
    @{ Name = "large_direct_slice_8m"; Threads = 4; ItersPerThread = 3000; WorkingSet = 2; MinSize = 8388608; MaxSize = 8388608; Note = "fixed-size direct large slice: 8 MiB" },
    @{ Name = "heavy_mixed"; Threads = 8; ItersPerThread = 5000000; WorkingSet = 16384; MinSize = 16; MaxSize = 4096; Note = "heavier mixed run with longer timings" }
)

$ProfileAliases = @{
    "selected_small_slices" = @(
        "large_slice_256",
        "large_slice_512",
        "large_slice_1k",
        "large_slice_2k",
        "large_slice_4k",
        "large_slice_8k",
        "large_slice_16k"
    )
    "large_slices" = @(
        "large_slice_256",
        "large_slice_512",
        "large_slice_1k",
        "large_slice_2k",
        "large_slice_4k",
        "large_slice_8k",
        "large_slice_16k",
        "large_slice_32k",
        "large_slice_64k",
        "large_slice_128k",
        "large_slice_256k",
        "large_slice_512k",
        "large_slice_1m",
        "large_direct_slice_2m",
        "large_direct_slice_4m",
        "large_direct_slice_8m"
    )
}

if ($Profiles -and $Profiles.Count -gt 0) {
    $ProfileNames = @()
    foreach ($name in $Profiles) {
        foreach ($part in @($name -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })) {
            if ($ProfileAliases.ContainsKey($part)) {
                $ProfileNames += $ProfileAliases[$part]
            } else {
                $ProfileNames += $part
            }
        }
    }
    $Selected = @()
    foreach ($name in $ProfileNames) {
        $match = $AllProfiles | Where-Object { $_.Name -eq $name }
        if (-not $match) {
            throw "Unknown profile: $name"
        }
        $Selected += $match
    }
} else {
    $Selected = $AllProfiles
}

if ($ListOnly) {
    Write-Host "Selected allocator matrix rows:"
    foreach ($profile in $Selected) {
        foreach ($exe in $Executables) {
            Write-Host ("{0}`t{1}`t{2}" -f $profile.Name, $exe.Name, $exe.Path)
        }
    }
    return
}

$ArtifactsPath = Join-Path $RepoRoot "out_win_suite"
$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_allocator_matrix.md")
$Summary = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Windows Allocator Matrix")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("Artifacts: [out_win_suite]($ArtifactsPath)")
$Summary.Add("Allocators: " + (($Executables | ForEach-Object { $_.Name }) -join ", "))
$Summary.Add("")
$Summary.Add("Notes:")
$Summary.Add("- `hz5-policy` uses the HZ5 Windows policy/API path in this mixed `malloc/free` runner. It is not the exact 64K/a8192 Local2P microbench lane; use the HZ5 synthetic/Local2P family for that profile.")
$Summary.Add("- `hz11-span-cache256` is the selected Windows HZ11 bring-up row: HZ11_CLASSIFY_SPAN=1 plus HZ11_CACHE_CAP=256 with a VirtualAlloc span arena. `hz11-span` remains the L1 control row. These are Windows matrix connectivity rows, not Linux fine128 parity and not default allocator claims.")
$Summary.Add("- `hz11-span-transfer` adds HZ11_TRANSFER_CENTRAL_SPAN=1 to the Windows span row as an opt-in L1 pressure lane. It is not the selected Windows row unless a follow-up gate promotes it.")
$Summary.Add("- `hz11-span-diag` is a diagnostic-only sibling of `hz11-span` with HZ11_ENABLE_HOT_COUNTERS=1, HZ11_SPAN_RETURNED_DIAG=1, and HZ11 summary printing. Do not use it for speed ranking.")
$Summary.Add("- `hz11-span-tlsfast` and `hz11-span-tlsfast-cache256` are Windows A/B rows for the matrix balanced pressure signal. `hz11-span-cache256-diag` is the diagnostic sibling of the selected cache256 row.")
$Summary.Add("- `hz11-span-cache512` is an experimental Windows cap-pressure row used to test whether matrix balanced/wide_ws weakness is caused by returned-object/refill pressure. `hz11-span-cache512-diag` is its diagnostic sibling and should not be used for speed ranking.")
$Summary.Add("- `hz11-span-cache512-classdiag` adds HZ11_CLASS_DIAG=1 to print per-class malloc/hit/refill/overflow/returned-pop attribution. It is diagnostic-only and must not be used for speed ranking.")
$Summary.Add("- `hz11-span-cache512-classbatch` is a Windows L1 behavior probe: for class >= 4 it pops returned objects in a small batch and seeds the front cache. `classbatch-diag` adds class attribution and must not be used for speed ranking.")
$Summary.Add("- `hz11-span-cache512-classbatch16` and `classbatch16-4-7` are narrower L2 probes. `classbatch16` is candidate-watch / matrix helper; `classbatch16-4-7` is balanced/wide_ws specialist evidence. Pressure-gated and 4-6 variants are documented no-go/evidence rows and are not kept in the default runner list.")
$Summary.Add("- `hz11-span-*-matrixattrib` rows enable HZ11_MATRIX_ATTRIB_DIAG=1 and are diagnostic-only. They split refill into returned-one, returned-batch, current-span, span-new, and sys-fallback paths.")
$Summary.Add("- `hz11-span-cache512-classbatch16-coldskip` is a Windows L2 returned-empty-lock probe. After a returned-pop miss it briefly skips returned-sink lock attempts when the current span has slots. It is evidence/candidate-watch, not the selected row.")
$Summary.Add("- `hz6-*-broad` keeps the same HZ6 policy profile but raises descriptor/route/source/front-cache capacities for broad working-set matrix profiles.")
$Summary.Add("- `hz6-*-route4k` keeps the non-route capacities at control values while widening only the route table to 4096.")
$Summary.Add("- `hz6-*-largerlowrss` uses the selected 4K..16K/LargerSizes low-RSS lane: front8k + SourceRunReuse + desc8k + route8k.")
$Summary.Add("- `hz6-*-largedirectretain16m-largerlowrss` and `hz6-*-largedirectretain32m-largerlowrss` add >1MiB LargeDirectRetain controls to the LargerLowRSS lane for `large_slices` follow-up rows.")
$Summary.Add("- `hz6-*-sameownerfast-largerlowrss` adds SameOwnerFast-L1 to the LargerLowRSS lane for same-owner small/mid fixed-size checks.")
$Summary.Add("- `hz6-*-directlocalfreereuse-largerlowrss` decomposes the SameOwnerFast win into direct local free + alloc + reuse on the LargerLowRSS lane.")
$Summary.Add("- `hz6-*-directlocalfreereuse-small8k-largerlowrss` limits DirectLocalFreeReuse to class <= 4 so 16K/32K MidPage rows fall back to the LargerLowRSS path.")
$Summary.Add("- The unqualified `hz6-*` rows keep the small default R1 capacities as controls.")
$Summary.Add("")

foreach ($profile in $Selected) {
    $Summary.Add("## " + $profile.Name)
    $Summary.Add("")
    $Summary.Add("- Note: " + $profile.Note)
    $Summary.Add("- Args: threads=$($profile.Threads) iters=$($profile.ItersPerThread) ws=$($profile.WorkingSet) size=$($profile.MinSize)..$($profile.MaxSize)")
    $Summary.Add("")
$Summary.Add("| allocator | ops/s | peak RSS KB | raw |")
$Summary.Add("| --- | ---: | ---: | --- |")

    $ProfileLog = Join-Path $OutputDir ($Stamp + "_" + $profile.Name + ".log")
    $LogLines = New-Object System.Collections.Generic.List[string]

    foreach ($exe in $Executables) {
        $benchArgs = @(
            [string]$profile.Threads,
            [string]$profile.ItersPerThread,
            [string]$profile.WorkingSet,
            [string]$profile.MinSize,
            [string]$profile.MaxSize
        )
        $result = Invoke-BenchProcess -Path $exe.Path -BenchArgs $benchArgs
        $raw = (($result.Output | ForEach-Object { $_.ToString().Trim() }) -join " ").Trim()
        $raw = ($raw -replace '[\r\n]+', ' ')
        if (-not $raw) {
            $raw = "(no output)"
        }
        $LogLines.Add("=== " + $profile.Name + " / " + $exe.Name + " ===")
        $LogLines.Add("cmd: " + $exe.Path + " " + ($benchArgs -join " "))
        $LogLines.Add("rc: " + $result.ExitCode)
        $LogLines.Add($raw)
        $LogLines.Add("")
        if ($result.ExitCode -ne 0) {
            $Summary.Add(('| {0} | failed:{1} | {2} | `{3}` |' -f $exe.Name, $result.ExitCode, $result.PeakKb, $raw))
            if (-not $ContinueOnFailure) {
                throw "Profile $($profile.Name) allocator $($exe.Name) failed with exit code $($result.ExitCode)"
            }
            continue
        }

        $ops = ""
        if ($raw -match "ops/s=([0-9.]+)") {
            $ops = $Matches[1]
        } else {
            $ops = "n/a"
        }
        $Summary.Add(('| {0} | {1} | {2} | `{3}` |' -f $exe.Name, $ops, $result.PeakKb, $raw))
    }

    Set-Content -Path $ProfileLog -Value $LogLines -Encoding UTF8
    $Summary.Add("")
}

Set-Content -Path $SummaryPath -Value $Summary -Encoding UTF8
Write-Host "Wrote summary: $SummaryPath"
