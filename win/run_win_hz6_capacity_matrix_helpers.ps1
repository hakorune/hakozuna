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
            if ($line -match '^(\[BENCH_ARGS\]|\[RSS\]|\[OPS\]|\[HZ6_STATS\]|\[HZ6_SOURCE_BLOCK_ROUTE\]|\[HZ6_MEMORY_ATTR\]|\[HZ6_RSS_RESIDUAL\]|\[HZ6_CAPACITY_UTIL\]|\[HZ6_MAIN_WARMUP_CAPACITY\]|\[HZ6_ELASTIC_PROJECTION\]|\[HZ6_ELASTIC_OVERFLOW_PROJECTION\]|\[HZ6_METADATA_SLIM\]|\[HZ6_FRONT_ALLOC_PATH\]|\[HZ6_FRONTCACHE_CLASS\]|\[HZ6_ROUTE_PROBE_SHAPE\]|\[HZ6_REDIS_STATS\]|bench_larson_compare: unhandled exception|threads=|bench_[^:]+:.*ops/s=|Pattern:|Throughput:|Throughput = |Ops:|---)') {
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

function Test-LogCaptureHasResultLine {
    param(
        [Parameter(Mandatory = $true)]$Lines
    )
    foreach ($line in @($Lines)) {
        $text = $line.ToString()
        if ($text -match '(ops/s=|Throughput = |Throughput:|Pattern:)') {
            return $true
        }
    }
    return $false
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
    try {
        # Give redirected stdout/stderr handles a short chance to flush after
        # process exit. Some long HZ6 Larson rows can otherwise return rc=0
        # with an empty capture file, which makes the matrix parser report a
        # false parse failure.
        [void]$proc.WaitForExit(1000)
    } catch {
    }

    $lines = New-Object System.Collections.Generic.List[string]
    $captured = Get-LogCapture -Paths @($stdoutPath, $stderrPath) -TailLimit 200
    $captureRetry = 0
    while (($captured.Count -eq 0 -or -not (Test-LogCaptureHasResultLine -Lines $captured)) -and $captureRetry -lt 20) {
        Start-Sleep -Milliseconds 100
        $captured = Get-LogCapture -Paths @($stdoutPath, $stderrPath) -TailLimit 200
        $captureRetry++
    }
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
            foreach ($slice in @("large_slice_256", "large_slice_512", "large_slice_1k", "large_slice_2k", "large_slice_4k", "large_slice_8k", "large_slice_16k", "large_slice_32k", "large_slice_64k", "large_slice_128k", "large_slice_256k", "large_slice_512k", "large_slice_1m", "large_direct_slice_2m", "large_direct_slice_4m", "large_direct_slice_8m")) {
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

