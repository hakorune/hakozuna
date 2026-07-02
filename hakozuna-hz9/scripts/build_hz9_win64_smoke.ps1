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
  /DH8_ENABLE_DEBUG_STATS `
  /Fe$outExe `
  $sources `
  $smoke

if ($LASTEXITCODE -ne 0) {
  throw "clang-cl failed with exit code $LASTEXITCODE"
}

Write-Host "built $outExe"

$env:H8_SMOKE_REGULAR_ADOPTION = '0'
& $outExe
if ($LASTEXITCODE -ne 0) {
  throw "h8_smoke_win default run failed with exit code $LASTEXITCODE"
}

$env:H8_SMOKE_REGULAR_ADOPTION = '1'
& $outExe
if ($LASTEXITCODE -ne 0) {
  throw "h8_smoke_win adoption run failed with exit code $LASTEXITCODE"
}

Remove-Item Env:H8_SMOKE_REGULAR_ADOPTION -ErrorAction SilentlyContinue
