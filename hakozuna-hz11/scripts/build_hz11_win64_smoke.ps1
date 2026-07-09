$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$outExe = Join-Path $root "hz11_thread_cache_smoke_win.exe"

$clang = (Get-Command clang-cl -ErrorAction Stop).Source

$sources = @(
  (Join-Path $root "src\hz11_size_class.c"),
  (Join-Path $root "src\hz11_sys_alloc.c"),
  (Join-Path $root "src\hz11_thread_cache.c"),
  (Join-Path $root "src\hz11_public_entry.c"),
  (Join-Path $root "tests\hz11_thread_cache_smoke.c")
)

& $clang /nologo /W4 /WX /O2 /D_CRT_SECURE_NO_WARNINGS `
  /I (Join-Path $root "include") `
  /I (Join-Path $root "src") `
  /Fe$outExe `
  $sources

if ($LASTEXITCODE -ne 0) {
  throw "clang-cl failed with exit code $LASTEXITCODE"
}

Write-Host "built $outExe"

& $outExe
if ($LASTEXITCODE -ne 0) {
  throw "hz11_thread_cache_smoke_win failed with exit code $LASTEXITCODE"
}
