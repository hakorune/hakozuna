param(
    [string]$OutputDir,
    [int]$Runs = 2,
    [int]$TimeoutSeconds = 18
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper\hz6_frontcache_packed_l1\direct_closeout_20260605"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$cases = @(
    @{
        Name = "baseline_l2"
        Exe = "out_win_hz6_capacity\bench_larson_hz6_speed_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_s16_r192_run512.exe"
    },
    @{
        Name = "frontcachepacked"
        Exe = "out_win_hz6_capacity\bench_larson_hz6_speed_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_s16_r192_run512.exe"
    }
)

$benchArgs = @("10", "8", "1024", "10000", "1", "12345", "16", "0")
$rows = @()

foreach ($case in $cases) {
    $exe = Join-Path $RepoRoot $case.Exe
    if (-not (Test-Path $exe)) {
        throw "missing benchmark exe: $exe"
    }

    foreach ($run in 1..$Runs) {
        $stdout = Join-Path $OutputDir "$($case.Name)_run${run}.out.txt"
        $stderr = Join-Path $OutputDir "$($case.Name)_run${run}.err.txt"
        $proc = Start-Process -FilePath $exe `
            -ArgumentList $benchArgs `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
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
            Start-Sleep -Milliseconds 10
        }

        $timedOut = $false
        if (-not $proc.HasExited) {
            $timedOut = $true
            try {
                $proc.Kill($true)
            } catch {
                try { $proc.Kill() } catch {}
            }
            $proc.WaitForExit()
        }

        $text = ""
        if (Test-Path $stdout) {
            $text += [System.IO.File]::ReadAllText($stdout)
        }
        if (Test-Path $stderr) {
            $text += [System.IO.File]::ReadAllText($stderr)
        }

        $ops = $null
        if ($text -match "Throughput = ([0-9]+) operations per second") {
            $ops = [double]$Matches[1]
        }
        $peakKb = if ($peakWorkingSetBytes -gt 0) {
            [UInt64][Math]::Ceiling($peakWorkingSetBytes / 1024.0)
        } else {
            $null
        }

        $safety = @{}
        foreach ($key in @("route_invalid", "route_miss", "remote_free_transfer_fail", "alloc_fail")) {
            if ($text -match "$key=([0-9]+)") {
                $safety[$key] = [UInt64]$Matches[1]
            } else {
                $safety[$key] = $null
            }
        }

        $row = [pscustomobject]@{
            lane = $case.Name
            run = $run
            timed_out = $timedOut
            ops_s = $ops
            peak_kb = $peakKb
            route_invalid = $safety["route_invalid"]
            route_miss = $safety["route_miss"]
            remote_free_transfer_fail = $safety["remote_free_transfer_fail"]
            alloc_fail = $safety["alloc_fail"]
        }
        $rows += $row
        Write-Host ($row | ConvertTo-Json -Compress)
    }
}

$rows | Export-Csv -NoTypeInformation -Path (Join-Path $OutputDir "direct_closeout_rows.csv")

function Get-Median {
    param([double[]]$Values)
    if (-not $Values -or $Values.Count -eq 0) {
        return $null
    }
    $sorted = [double[]]$Values.Clone()
    [Array]::Sort($sorted)
    $mid = [int][Math]::Floor($sorted.Count / 2.0)
    if (($sorted.Count % 2) -eq 1) {
        return $sorted[$mid]
    }
    return ($sorted[$mid - 1] + $sorted[$mid]) / 2.0
}

$summary = foreach ($lane in ($rows | Select-Object -ExpandProperty lane -Unique)) {
    $laneRows = @($rows | Where-Object { $_.lane -eq $lane })
    [pscustomobject]@{
        lane = $lane
        median_ops_s = Get-Median -Values ([double[]]@($laneRows | ForEach-Object { [double]$_.ops_s }))
        median_peak_kb = Get-Median -Values ([double[]]@($laneRows | ForEach-Object { [double]$_.peak_kb }))
        safety_clean = -not ($laneRows | Where-Object {
            $_.route_invalid -ne 0 -or
            $_.route_miss -ne 0 -or
            $_.remote_free_transfer_fail -ne 0 -or
            $_.alloc_fail -ne 0
        })
    }
}

$lines = @(
    "# HZ6 FrontCachePackedMeta-L1 Direct Closeout",
    "",
    "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')",
    "",
    "Args: ``$($benchArgs -join ' ')``",
    "",
    "## Median Summary",
    "",
    "| lane | median ops/s | median peak_kb | safety_clean |",
    "| --- | ---: | ---: | --- |"
)
foreach ($row in $summary) {
    $lines += "| $($row.lane) | $('{0:N0}' -f $row.median_ops_s) | $($row.median_peak_kb) | $($row.safety_clean) |"
}

$lines += @(
    "",
    "## Runs",
    "",
    "| lane | run | timed_out | stopped_after_result | ops/s | peak_kb | route_invalid | route_miss | remote_free_transfer_fail | alloc_fail |",
    "| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |"
)
foreach ($row in $rows) {
    $opsText = if ($null -ne $row.ops_s) { "{0:N0}" -f $row.ops_s } else { "NA" }
    $lines += "| $($row.lane) | $($row.run) | $($row.timed_out) | n/a | $opsText | $($row.peak_kb) | $($row.route_invalid) | $($row.route_miss) | $($row.remote_free_transfer_fail) | $($row.alloc_fail) |"
}

$lines | Set-Content -Path (Join-Path $OutputDir "direct_closeout_20260605.md") -Encoding ASCII
Write-Host "wrote $OutputDir"
