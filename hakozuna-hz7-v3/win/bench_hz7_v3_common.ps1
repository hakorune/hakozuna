function Get-H7Median {
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

function Format-H7Rate {
    param([double]$Value)
    if ([double]::IsNaN($Value)) {
        return "NaN"
    }
    if ($Value -ge 1000000.0) {
        return ("{0:N3}M" -f ($Value / 1000000.0))
    }
    if ($Value -ge 1000.0) {
        return ("{0:N3}K" -f ($Value / 1000.0))
    }
    return ("{0:N2}" -f $Value)
}

function Invoke-H7CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$Arguments
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
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($chunk in @($stdout, $stderr)) {
        if (-not [string]::IsNullOrEmpty($chunk)) {
            foreach ($line in ($chunk -split "`r?`n")) {
                if ($line -ne "") {
                    $lines.Add($line)
                }
            }
        }
    }

    return @{ ExitCode = $proc.ExitCode; Lines = $lines }
}

function New-H7BenchmarkSummaryLines {
    param(
        [string]$Title,
        [string]$Benchmark,
        [string]$Allocator,
        [int]$Runs,
        [int]$Iters,
        [string]$Note,
        [int]$DirectRetainCap = 0,
        [int]$SpanClassMax = 0
    )

    $lines = New-Object System.Collections.Generic.List[string]
    [void]$lines.Add("# $Title")
    [void]$lines.Add("")
    [void]$lines.Add("Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')")
    [void]$lines.Add("")
    [void]$lines.Add("- benchmark: ``$Benchmark``")
    [void]$lines.Add("- allocator: ``$Allocator``")
    [void]$lines.Add("- runs: $Runs")
    [void]$lines.Add("- iters_per_run: $Iters")
    if ($DirectRetainCap -gt 0) {
        [void]$lines.Add("- direct_retain_cap: $DirectRetainCap")
    }
    if ($SpanClassMax -gt 0) {
        [void]$lines.Add("- span_class_max: $SpanClassMax")
    }
    [void]$lines.Add("- note: $Note")
    [void]$lines.Add("")
    return ,$lines
}

function Add-H7BenchmarkSummaryTable {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [hashtable]$Rows,
        [string[]]$OrderedKeys = $null
    )

    $keys = $OrderedKeys
    if ($null -eq $keys) {
        $keys = @($Rows.Keys | Sort-Object)
    }

    [void]$Lines.Add("| op | label | size | median rate | rate unit | median peak_kb |")
    [void]$Lines.Add("| --- | --- | ---: | ---: | --- | ---: |")

    foreach ($key in $keys) {
        if (-not $Rows.ContainsKey($key)) {
            continue
        }
        $row = $Rows[$key]
        $medianRate = Get-H7Median $row.Rates.ToArray()
        $medianRss = Get-H7Median $row.Rss.ToArray()
        [void]$Lines.Add("| $($row.Op) | $($row.Label) | $($row.Size) | $(Format-H7Rate $medianRate) | $($row.RateName) | $([int]$medianRss) |")
    }
}
