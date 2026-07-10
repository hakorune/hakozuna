param(
    [int]$MaxLinesPerFile = 999
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$Roots = @(
    (Join-Path $Hz12Root "src"),
    (Join-Path $Hz12Root "include")
)
$Extensions = @(".c", ".h", ".inc")
$Rows = @()

foreach ($root in $Roots) {
    Get-ChildItem $root -File | Where-Object {
        $Extensions -contains $_.Extension
    } | ForEach-Object {
        $Rows += [pscustomobject]@{
            Lines = (Get-Content $_.FullName | Measure-Object -Line).Lines
            File = $_.FullName.Substring($Hz12Root.Length + 1)
        }
    }
}

$Rows = @($Rows | Sort-Object Lines -Descending)
$Rows | Format-Table -AutoSize
$Violations = @($Rows | Where-Object { $_.Lines -gt $MaxLinesPerFile })
if ($Violations.Count -ne 0) {
    throw "HZ12 source layout exceeded $MaxLinesPerFile lines per file."
}

Write-Host "HZ12 source layout OK: $($Rows.Count) files, max $($Rows[0].Lines) lines."
