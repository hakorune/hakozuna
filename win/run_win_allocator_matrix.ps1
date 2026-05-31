param(
    [string]$OutputDir,
    [string[]]$Profiles,
    [switch]$ContinueOnFailure
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDir = Join-Path $RepoRoot "out_win_suite"
$BuildScript = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\\benchmarks\\windows"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$Executables = @(
    @{ Name = "crt"; Path = (Join-Path $SuiteDir "bench_mixed_ws_crt.exe") },
    @{ Name = "hz3"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz3.exe") },
    @{ Name = "hz4"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz4.exe") },
    @{ Name = "hz5-policy"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz5_policy.exe") },
    @{ Name = "hz6-strict"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict.exe") },
    @{ Name = "hz6-speed"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed.exe") },
    @{ Name = "hz6-rss"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss.exe") },
    @{ Name = "hz6-strict-broad"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_broad.exe") },
    @{ Name = "hz6-speed-broad"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_broad.exe") },
    @{ Name = "hz6-rss-broad"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_broad.exe") },
    @{ Name = "hz6-strict-route4k"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_strict_route4k.exe") },
    @{ Name = "hz6-speed-route4k"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_speed_route4k.exe") },
    @{ Name = "hz6-rss-route4k"; Path = (Join-Path $SuiteDir "bench_mixed_ws_hz6_rss_route4k.exe") },
    @{ Name = "mimalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_mimalloc.exe") },
    @{ Name = "tcmalloc"; Path = (Join-Path $SuiteDir "bench_mixed_ws_tcmalloc.exe") }
)

if ($Executables | Where-Object { -not (Test-Path $_.Path) }) {
    & $BuildScript
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

        $peakWorkingSetBytes = [Int64]0
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
            Start-Sleep -Milliseconds 1
        }

        $stdout = $proc.StandardOutput.ReadToEnd()
        $stderr = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()

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
        $rc = $proc.ExitCode
        $peakKb = if ($peakWorkingSetBytes -gt 0) { [string][UInt64]([Math]::Ceiling($peakWorkingSetBytes / 1024.0)) } else { "NA" }
        if ($stdout -match 'peak_kb=([0-9]+)') {
            $peakKb = $Matches[1]
        } elseif ($stderr -match 'peak_kb=([0-9]+)') {
            $peakKb = $Matches[1]
        }
        return [pscustomobject]@{
            Output    = $output
            ExitCode  = $rc
            PeakKb    = $peakKb
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
    @{ Name = "heavy_mixed"; Threads = 8; ItersPerThread = 5000000; WorkingSet = 16384; MinSize = 16; MaxSize = 4096; Note = "heavier mixed run with longer timings" }
)

$ProfileAliases = @{
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
        "large_slice_128k"
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

$ArtifactsPath = Join-Path $RepoRoot "out_win_suite"
$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_allocator_matrix.md")
$Summary = New-Object System.Collections.Generic.List[string]
$Summary.Add("# Windows Allocator Matrix")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("Artifacts: [out_win_suite]($ArtifactsPath)")
$Summary.Add("")
$Summary.Add("Notes:")
$Summary.Add("- `hz5-policy` uses the HZ5 Windows policy/API path in this mixed `malloc/free` runner. It is not the exact 64K/a8192 Local2P microbench lane; use the HZ5 synthetic/Local2P family for that profile.")
$Summary.Add("- `hz6-*-broad` keeps the same HZ6 policy profile but raises descriptor/route/source/front-cache capacities for broad working-set matrix profiles.")
$Summary.Add("- `hz6-*-route4k` keeps the non-route capacities at control values while widening only the route table to 4096.")
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
