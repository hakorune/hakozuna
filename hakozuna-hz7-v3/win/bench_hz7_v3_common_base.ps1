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
        $unit = ""
        if ($row.ContainsKey("RateName")) {
            $unit = $row.RateName
        } elseif ($row.ContainsKey("Unit")) {
            $unit = $row.Unit
        }
        [void]$Lines.Add("| $($row.Op) | $($row.Label) | $($row.Size) | $(Format-H7Rate $medianRate) | $unit | $([int]$medianRss) |")
    }
}

function Add-H7HotpathRowsFromLines {
    param(
        [hashtable]$Rows,
        [System.Collections.Generic.List[string]]$Lines,
        [string]$LinePrefix = '^hz7_hotpath:'
    )

    foreach ($line in $Lines) {
        if ($line -notmatch $LinePrefix) {
            continue
        }
        $fields = @{}
        foreach ($part in ($line -split '\s+')) {
            if ($part -match '^([^=]+)=(.*)$') {
                $fields[$Matches[1]] = $Matches[2]
            }
        }
        if (-not $fields.ContainsKey("op") -or -not $fields.ContainsKey("label")) {
            continue
        }
        $key = "$($fields["op"]):$($fields["label"])"
        if (-not $Rows.ContainsKey($key)) {
            $Rows[$key] = @{
                Op = $fields["op"]
                Label = $fields["label"]
                Size = $fields["size"]
                RateName = if ($fields.ContainsKey("pairs/s")) { "pairs/s" } else { "ops/s" }
                Rates = New-Object System.Collections.Generic.List[double]
                Rss = New-Object System.Collections.Generic.List[double]
            }
        }
        $rateKey = $Rows[$key].RateName
        if ($fields.ContainsKey($rateKey)) {
            $Rows[$key].Rates.Add([double]$fields[$rateKey])
        }
        if ($fields.ContainsKey("peak_kb")) {
            $Rows[$key].Rss.Add([double]$fields["peak_kb"])
        }
    }
}

function Add-H7SummaryRowsFromMarkdown {
    param(
        [hashtable]$Rows,
        [System.Collections.Generic.List[string]]$Lines
    )

    foreach ($line in $Lines) {
        if ($line -notmatch '^\|\s*(?<op>[^|]+)\s*\|\s*(?<label>[^|]+)\s*\|\s*(?<size>[^|]+)\s*\|\s*(?<rate>[^|]+)\s*\|\s*(?<unit>[^|]+)\s*\|\s*(?<rss>[^|]+)\s*\|$') {
            continue
        }
        $op = $Matches.op.Trim()
        $label = $Matches.label.Trim()
        $size = $Matches.size.Trim()
        $rate = $Matches.rate.Trim()
        $unit = $Matches.unit.Trim()
        $rss = $Matches.rss.Trim()
        $key = "${op}:${label}"
        if (-not $Rows.ContainsKey($key)) {
            $Rows[$key] = @{
                Op = $op
                Label = $label
                Size = $size
                Unit = $unit
                Rates = New-Object System.Collections.Generic.List[double]
                Rss = New-Object System.Collections.Generic.List[double]
            }
        }
        if ($rate -match '([0-9.]+)([MK]?)') {
            $rateValue = [double]$Matches[1]
            switch ($Matches[2]) {
                'M' { $rateValue *= 1000000.0 }
                'K' { $rateValue *= 1000.0 }
            }
            $Rows[$key].Rates.Add($rateValue)
        }
        if ($rss -match '([0-9.]+)') {
            $Rows[$key].Rss.Add([double]$Matches[1])
        }
    }
}

function Find-H7SummaryPathFromLines {
    param([System.Collections.Generic.List[string]]$Lines)

    foreach ($line in $Lines) {
        if ($line -match '^Wrote summary:\s+(.*)$') {
            return $Matches[1].Trim()
        }
    }
    return $null
}

function Get-H7BenchmarkRowsFromMarkdownPath {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "summary not found: $Path"
    }
    $rows = @{}
    Add-H7SummaryRowsFromMarkdown -Rows $rows -Lines (Get-Content $Path)
    return $rows
}

function Get-H7SpanAuditOrderedKeys {
    return @(
        'malloc_free:span4k',
        'malloc_free:span8k',
        'malloc_free:span16k',
        'route_invariant:span4k',
        'route_invariant:span8k',
        'route_invariant:span16k',
        'route_valid:span4k',
        'route_valid:span8k',
        'route_valid:span16k',
        'route_invalid:span4k',
        'route_invalid:span8k',
        'route_invalid:span16k',
        'malloc_batch:span4k',
        'malloc_batch:span8k',
        'malloc_batch:span16k',
        'free_batch:span4k',
        'free_batch:span8k',
        'free_batch:span16k',
        'free_retained_loop:span4k',
        'free_retained_loop:span8k',
        'free_retained_loop:span16k'
    )
}
