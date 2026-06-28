$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$outExe = Join-Path $root "h8_smoke_win.exe"

$clang = (Get-Command clang-cl -ErrorAction Stop).Source

$sources = Get-ChildItem (Join-Path $root "src") -Filter "*.c" |
  ForEach-Object { $_.FullName }
$smoke = Join-Path $root "tests\\h8_smoke.c"

& $clang /nologo /D_CRT_SECURE_NO_WARNINGS `
  /I (Join-Path $root "include") `
  /I (Join-Path $root "src") `
  /Fe$outExe `
  $sources `
  $smoke

if ($LASTEXITCODE -ne 0) {
  throw "clang-cl failed with exit code $LASTEXITCODE"
}

Write-Host "built $outExe"
